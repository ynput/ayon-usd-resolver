"""TCP proxy between the resolver and a memcached backend.

memcached's own ``stats`` counts keys, not requests, so it cannot tell one multi-key ``get``
from many single-key ones. This proxy parses the text protocol the resolver sends, counts
commands per kind and keys per ``get``, and can add latency or drop the connection on a
multi-key ``get``. It only reads the client's stream; replies pass through unchanged.
"""

from __future__ import annotations

import socket
import threading
import time
from collections import Counter
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from .mc import Backend

STORAGE_COMMANDS = {b"set", b"add", b"replace", b"append", b"prepend", b"cas"}


class McProxy:
    def __init__(self, target: Backend, *, rtt_ms: float = 0, fail_multi_get: bool = False) -> None:
        self.target = target
        self.rtt_ms = rtt_ms
        self.fail_multi_get = fail_multi_get
        self.get_sizes: Counter[int] = Counter()  # keys in one get -> number of such gets
        self.commands: Counter[str] = Counter()
        self.failed_gets = 0
        self._lock = threading.Lock()
        self._server = socket.create_server(("127.0.0.1", 0))
        self._closing = False
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
        self._closing = True
        self._server.close()

    def _accept(self) -> None:
        while not self._closing:
            try:
                client, _ = self._server.accept()
            except OSError:
                return
            threading.Thread(target=self._serve, args=(client,), daemon=True).start()

    def _serve(self, client: socket.socket) -> None:
        upstream = socket.create_connection((self.target.host, self.target.port))
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
                    if self.rtt_ms:
                        time.sleep(self.rtt_ms / 1000)
                    upstream.sendall(complete)
        except OSError:
            pass
        finally:
            client.close()
            upstream.close()

    @staticmethod
    def _pump_replies(upstream: socket.socket, client: socket.socket) -> None:
        try:
            while data := upstream.recv(65536):
                client.sendall(data)
        except OSError:
            pass
        finally:
            client.close()

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
