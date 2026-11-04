#!/bin/bash
# Локальный HTTP-сервер для тестирования прокси без интернета
# Запуск: ./local_test_server.sh &
# Остановка: kill $(pgrep -f local_test_server)

python3 -c "
import http.server, sys

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        body = b'Hello from local test server!'
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    
    def log_message(self, format, *args):
        pass

server = http.server.HTTPServer(('127.0.0.1', 9999), Handler)
print('Local test server running on port 9999', flush=True)
server.serve_forever()
"