# -*- coding: utf-8 -*-
# tested on python 3.4 ,python of lower version  has different module organization.
# from https://gist.github.com/HaiyangXu/ec88cbdce3cdbac7b8d5
import http.server
from http.server import HTTPServer, BaseHTTPRequestHandler
import socketserver

PORT = 8080

Handler = http.server.SimpleHTTPRequestHandler

Handler.extensions_map = {
  '.manifest': 'text/cache-manifest',
  '.html': 'text/html',
  '.png': 'image/png',
  '.jpg': 'image/jpg',
  '.svg': 'image/svg+xml',
  '.wasm': 'application/wasm',
  '.css': 'text/css',
  '.js': 'application/x-javascript',
  '': 'application/octet-stream',  # Default
}

httpd = socketserver.TCPServer(("", PORT), Handler)

print("serving at port", PORT)
httpd.serve_forever()
