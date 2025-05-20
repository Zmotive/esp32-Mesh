import socketserver
import threading
from datetime import datetime

HOST = ''  # Listen on all interfaces
PORT = 9000  # You can change this port if needed

class LogHandler(socketserver.BaseRequestHandler):
    def handle(self):
        client_ip = self.client_address[0]
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        print(f"[{now}][{client_ip}]: Connection from {client_ip}")
        while True:
            data = self.request.recv(1024)
            if not data:
                break
            now = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            print(f"[{now}][{client_ip}]: {data.decode(errors='replace').rstrip()}")
        print(f"[{now}][{client_ip}]: Connection from {client_ip} closed.")

def run_server():
    with socketserver.ThreadingTCPServer((HOST, PORT), LogHandler) as server:
        print(f"-------- Log server listening on port {PORT} --------")
        server.serve_forever()

if __name__ == "__main__":
    run_server()
