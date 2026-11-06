"""TCP proxy between the resolver and a memcached backend.

memcached's own ``stats`` counts keys, not requests, so it cannot tell one multi-key ``get``
from many single-key ones. This proxy parses the text protocol the resolver sends, counts
commands per kind and keys per ``get``, and can add latency or drop the connection on a
multi-key ``get``, and can hold the reads of one key, or the replies carrying its value, to keep
a race open. It only reads the client's stream; replies pass through unchanged.
"""

from __future__ import annotations

import contextlib
import socket
import threading
import time
from collections import Counter
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from .mc import Backend

STORAGE_COMMANDS = {b"set", b"add", b"replace", b"append", b"prepend", b"cas"}


class McProxy:
    def __init__(
        self,
        target: Backend,
        *,
        rtt_ms: float = 0,
        fail_multi_get: bool = False,
        hold_get: tuple[str, float] | None = None,
        hold_value: tuple[str, float] | None = None,
        hold_end: tuple[str, float] | None = None,
    ) -> None:
        self.target = target
        self.rtt_ms = rtt_ms
        self.fail_multi_get = fail_multi_get
        self.hold_get = hold_get  # (key, milliseconds): delay every get of that key
        # (key, milliseconds): delay every reply carrying that key's value, after the server has sent it
        self.hold_value = hold_value
        self.held_values = threading.Event()  # set once a reply is being held
        # (key, milliseconds): send that key's value at once, and only the END after it late
        self.hold_end = hold_end
        self.held_ends = 0  # replies split by hold_end
        self.get_sizes: Counter[int] = Counter()  # keys in one get -> number of such gets
        self.commands: Counter[str] = Counter()
        self.failed_gets = 0
        self._lock = threading.Lock()
        self._server = socket.create_server(("127.0.0.1", 0))
        self._closing = False
        self._sockets: list[socket.socket] = []
        self._thread = threading.Thread(target=self._accept, daemon=True)
        self._thread.start()

    @property
    def address(self) -> str:
        """Value for AYON_MEMCACHED_SERVERS."""
        return f"127.0.0.1:{self._server.getsockname()[1]}"

    def reset(self) -> None:
        with self._lock:
            self.get_sizes.clear()
            self.commands.clear()
            self.failed_gets = 0

    def metrics(self) -> dict[str, int]:
        """Counters since the last reset, in the shape published with a session record."""
        with self._lock:
            return {
                "mc_get_commands": sum(self.get_sizes.values()),
                "mc_get_keys": sum(size * count for size, count in self.get_sizes.items()),
                "mc_get_max_keys": max(self.get_sizes, default=0),
                "mc_multi_get_commands": sum(count for size, count in self.get_sizes.items() if size > 1),
                "mc_set_commands": self.commands["set"],
                "mc_add_commands": self.commands["add"],
                "mc_delete_commands": self.commands["delete"],
                "mc_flush_commands": self.commands["flush_all"],
                "mc_failed_gets": self.failed_gets,
            }

    def close(self) -> None:
        """Stop listening and drop every open connection, like a server that died."""
        self._closing = True
        # close() alone leaves a blocked accept() listening; shutdown() ends it.
        with contextlib.suppress(OSError):
            self._server.shutdown(socket.SHUT_RDWR)
        self._server.close()
        with self._lock:
            sockets = list(self._sockets)
        for sock in sockets:
            with contextlib.suppress(OSError):
                sock.shutdown(socket.SHUT_RDWR)

    def _accept(self) -> None:
        while not self._closing:
            try:
                client, _ = self._server.accept()
            except OSError:
                return
            threading.Thread(target=self._serve, args=(client,), daemon=True).start()

    def _serve(self, client: socket.socket) -> None:
        upstream = socket.create_connection((self.target.host, self.target.port))
        with self._lock:
            self._sockets += [client, upstream]
        for sock in (client, upstream):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        threading.Thread(target=self._pump_replies, args=(upstream, client), daemon=True).start()
        buffer = b""
        try:  # ruff: ignore[too-many-statements-in-try-clause] - any socket error ends this one connection
            while data := client.recv(65536):
                buffer += data
                complete, buffer, drop = self._parse(buffer)
                if drop:
                    return
                if complete:
                    delay_ms = self.rtt_ms + self._hold_ms(complete)
                    if delay_ms:
                        time.sleep(delay_ms / 1000)
                    upstream.sendall(complete)
        except OSError:
            pass
        finally:
            client.close()
            upstream.close()

    def _pump_replies(self, upstream: socket.socket, client: socket.socket) -> None:
        try:
            while data := upstream.recv(65536):
                if self.hold_value is not None and f"VALUE {self.hold_value[0]} ".encode() in data:
                    self.held_values.set()
                    time.sleep(self.hold_value[1] / 1000)
                if (
                    self.hold_end is not None
                    and f"VALUE {self.hold_end[0]} ".encode() in data
                    and data.endswith(b"END\r\n")
                ):
                    client.sendall(data[:-5])
                    self.held_ends += 1
                    time.sleep(self.hold_end[1] / 1000)
                    data = data[-5:]
                client.sendall(data)
        except OSError:
            pass
        finally:
            client.close()

    def _hold_ms(self, commands: bytes) -> float:
        if self.hold_get is None:
            return 0
        key, hold_ms = self.hold_get
        for line in commands.split(b"\r\n"):
            parts = line.split()
            if parts and parts[0] in {b"get", b"gets"} and key.encode() in parts[1:]:
                return hold_ms
        return 0

    def _parse(self, buffer: bytes) -> tuple[bytes, bytes, bool]:
        """Count the complete commands in ``buffer``. Returns (complete, remainder, drop)."""
        position = 0
        while (end := buffer.find(b"\r\n", position)) != -1:
            parts = buffer[position:end].split()
            next_position = end + 2
            name = parts[0] if parts else b""
            if name in STORAGE_COMMANDS and len(parts) >= 5:
                next_position += int(parts[4]) + 2  # data block and its line ending
                if next_position > len(buffer):
                    break
            if self._count(name, parts):
                return b"", b"", True
            position = next_position
        return buffer[:position], buffer[position:], False

    def _count(self, name: bytes, parts: list[Any]) -> bool:
        """Record one command; True when the connection must be dropped instead of forwarding it."""
        with self._lock:
            self.commands[name.decode(errors="replace")] += 1
            if name in {b"get", b"gets"}:
                keys = len(parts) - 1
                self.get_sizes[keys] += 1
                if self.fail_multi_get and keys > 1:
                    self.failed_gets += 1
                    return True
        return False
