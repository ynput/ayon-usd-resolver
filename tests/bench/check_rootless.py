"""Dump a few memcached values and report whether they are rootless.

The cache contract is: value = "{root[work]}/..." so a client with different roots can
re-root it on read. An absolute value resolves fine on the machine that wrote it and
breaks on every other one -- and a single client sharing one cache never sees that, so
it has to be checked explicitly rather than inferred from a working run.
"""

import json
import pathlib
import socket
import sys

DEFAULT_SHOW_LIMIT = 5
ARG_LIMIT = 3  # argv index at which the optional show-limit appears
MAX_MEMCACHED_KEY_LEN = 250

server, pins_path = sys.argv[1], sys.argv[2]
limit = int(sys.argv[3]) if len(sys.argv) > ARG_LIMIT else DEFAULT_SHOW_LIMIT
host, port = server.split(":")
with pathlib.Path(pins_path).open(encoding="utf-8") as handle:
    keys = list(json.load(handle)["ayon_resolver_pinning_data"])[:200]


class Reader:
    """Buffered reader over the memcached text protocol."""

    def __init__(self, sock: socket.socket) -> None:
        self.sock = sock
        self.buf = b""

    def line(self) -> bytes:
        """Read one CRLF-terminated protocol line.

        Returns:
            The line, without its terminator.
        """
        while b"\r\n" not in self.buf:
            self.buf += self.sock.recv(65536)
        out, self.buf = self.buf.split(b"\r\n", 1)
        return out

    def body(self, size: int) -> str:
        """Read a value body of ``size`` bytes plus its trailing CRLF.

        Returns:
            The decoded value.
        """
        while len(self.buf) < size + 2:
            self.buf += self.sock.recv(65536)
        val, self.buf = self.buf[:size].decode(), self.buf[size + 2 :]
        return val


s = socket.create_connection((host, int(port)), timeout=5)
reader = Reader(s)


shown = rootless = absolute = empty = 0
for k in keys:
    if len(k.encode()) > MAX_MEMCACHED_KEY_LEN or " " in k:
        continue
    s.sendall(f"get {k}\r\n".encode())
    hdr = reader.line()
    if not hdr.startswith(b"VALUE"):
        continue  # END -> not cached
    val = reader.body(int(hdr.split()[-1]))
    reader.line()  # trailing END
    if not val:
        # A stored empty value is a different failure from an absolute one: nothing
        # resolved, and every later reader takes the empty as a hit.
        empty += 1
    elif val.startswith("{root["):
        rootless += 1
    else:
        absolute += 1
    if shown < limit:
        print(f"  key   {k[:88]}")
        print(f"  value {val}")
        if not val:
            verdict = "*** EMPTY (negative cache entry)"
        elif val.startswith("{root["):
            verdict = "ROOTLESS ok"
        else:
            verdict = "*** ABSOLUTE (contract broken)"
        print(f"  -> {verdict}\n")
        shown += 1
s.close()
print(f"sampled: {rootless} rootless, {absolute} absolute, {empty} empty")
sys.exit(1 if absolute or empty else 0)
