#!/usr/bin/env python3
"""An example HTTP server with GET and POST endpoints.

Taken from https://gist.github.com/nitaku/10d0662536f37a087e1b

`python mock_server.py`

curl --data "{\"uris\":\"ayon://foo/bar/bar_010_0010?subset=model?version=v017\"}" --header "Content-Type: application/json" http://localhost:8001
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
from http import HTTPStatus
import json
import pathlib


USD_ASSETS = pathlib.Path(__file__).parent.resolve() / "test" / "assets"

KNOWN_URIS = {
    "ayon://foo/bar/bar_010_0010?subset=model?version=v017": str(USD_ASSETS / "bar_010_0010.usd"),
    "ayon://foo/bar/bar_020_0010?subset=model?version=v013": str(USD_ASSETS / "bar_020_0010.usd")
}


class _RequestHandler(BaseHTTPRequestHandler):
    # Borrowing from https://gist.github.com/nitaku/10d0662536f37a087e1b
    def _set_headers(self):
        self.send_response(HTTPStatus.OK.value)
        self.send_header('Content-type', 'application/json')
        # Allow requests from any origin, so CORS policies don't
        # prevent local development.
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

    def do_POST(self):
        content_length = int(self.headers.get('content-length'))
        message = json.loads(
            self.rfile.read(content_length).decode('utf8').replace("'", '"')
        )

        paths = [
            {uri: KNOWN_URIS.get(uri)}
            for uri in message.get("uris", [])
            if KNOWN_URIS.get(uri)
        ]

        self._set_headers()
        self.wfile.write(json.dumps({"paths": paths, "time": 0}).encode('utf-8'))

    def do_OPTIONS(self):
        # Send allow-origin header for preflight POST XHRs.
        self.send_response(HTTPStatus.NO_CONTENT.value)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST')
        self.send_header('Access-Control-Allow-Headers', 'content-type')
        self.end_headers()


def run_server():
    server_address = ('', 8001)
    httpd = HTTPServer(server_address, _RequestHandler)
    print('serving at %s:%d' % server_address)
    httpd.serve_forever()


if __name__ == '__main__':
    run_server()


