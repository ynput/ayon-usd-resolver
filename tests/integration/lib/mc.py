"""Minimal memcached text-protocol client for inspecting what the resolver stored.

The resolver uses libmemcached's default text protocol, so every backend under test must
speak it. ``stats`` returns only numeric counters; a backend that reports none (Dragonfly
reports little beyond its version) yields an empty dict, so callers can skip counter
assertions instead of failing on them.
"""

from __future__ import annotations

import socket
from dataclasses import dataclass


@dataclass(frozen=True)
class Backend:
    name: str
    host: str
    port: int

    @property
    def address(self) -> str:
        """Value for AYON_MEMCACHED_SERVERS."""
        return f"{self.host}:{self.port}"

    def client(self) -> TextClient:
        return TextClient(self.host, self.port)


def memcached_knobs(backend: Backend, **extra: str) -> dict[str, str]:
    """Resolver environment that enables memcached on ``backend``."""
    return {
        "AYON_MEMCACHED_ENABLED": "true",
        "AYON_MEMCACHED_SERVERS": backend.address,
        "AYON_MEMCACHED_TIMEOUT_MS": "500",
        **extra,
    }


def parse_backends(spec: str) -> dict[str, Backend]:
    """``name=host:port,...`` -> {name: Backend}."""
    backends = {}
    for item in filter(None, (s.strip() for s in spec.split(","))):
        name, address = item.split("=", 1)
        host, _, port = address.rpartition(":")
        backends[name] = Backend(name, host, int(port))
    return backends


class _Conn:
    def __init__(self, host: str, port: int, timeout: float = 3.0) -> None:
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.buf = b""

    def close(self) -> None:
        self.sock.close()

    def __enter__(self) -> _Conn:  # ruff: ignore[non-self-return-type] - typing.Self needs Python 3.11, the runner has 3.9
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    def _fill(self) -> None:
        data = self.sock.recv(65536)
        if not data:
            raise ConnectionError("memcached closed the connection")
        self.buf += data

    def read_exact(self, n: int) -> bytes:
        while len(self.buf) < n:
            self._fill()
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def read_line(self) -> bytes:
        while b"\r\n" not in self.buf:
            self._fill()
        line, self.buf = self.buf.split(b"\r\n", 1)
        return line


class TextClient:
    def __init__(self, host: str, port: int) -> None:
        self.host, self.port = host, port

    def get(self, key: str) -> bytes | None:
        with _Conn(self.host, self.port) as c:
            c.sock.sendall(b"get " + key.encode() + b"\r\n")
            value = None
            while True:
                line = c.read_line()
                if line == b"END":
                    return value
                if not line.startswith(b"VALUE "):
                    raise RuntimeError(f"unexpected reply to get: {line!r}")
                size = int(line.split()[3])
                value = c.read_exact(size + 2)[:-2]

    def flush(self) -> None:
        with _Conn(self.host, self.port) as c:
            c.sock.sendall(b"flush_all\r\n")
            if c.read_line() != b"OK":
                raise RuntimeError("flush_all failed")

    def stats(self) -> dict[str, int]:
        out: dict[str, int] = {}
        with _Conn(self.host, self.port) as c:
            c.sock.sendall(b"stats\r\n")
            while (line := c.read_line()) != b"END":
                _, name, value = line.decode().split(" ", 2)
                if value.isdigit():
                    out[name] = int(value)
        return out
