import socketserver
import threading
from datetime import datetime
import base64
import struct
import re

HOST = ''  # Listen on all interfaces
PORT = 9000  # You can change this port if needed

# Map struct names to their Python struct format strings and field names
STRUCT_FORMATS = {
    "UBXNavPVT": {
        "format": "<IH6BIi4B4i2I5i2I2HIihH",
        "fields": [
            "iTOW", "year", "month", "day", "hour", "min", "sec", "valid", "tAcc", "nano", "fixType", "flags", "flags2", "numSV", "lon", "lat", "height", "hMSL", "hAcc", "vAcc", "velN", "velE", "velD", "gSpeed", "headMot", "sAcc", "headAcc", "pDOP", "flags3", "reserved0", "headVeh", "magDec", "magAcc"
        ]
    },
    "UBXNavSVIN": {
        "format": "<B3x2I3i3bx2I2B2x",
        "fields": [
            "version", "iTOW", "dur", "meanX", "meanY", "meanZ", "meanXHP", "meanYHP", "meanZHP", "meanAcc", "obs", "valid", "active"
        ]
    },
    # Add more as needed
}

def parse_and_print_struct(struct_name, fmt, fields, b64data):
    try:
        bin_data = base64.b64decode(b64data)
        print(f" Binary data length: {len(bin_data)}")
        values = struct.unpack(fmt, bin_data)
        print(f"  Decoded {struct_name}:")
        for name, v in zip(fields, values):
            print(f"    {name}: {v}")
        if len(values) > len(fields):
            for i, v in enumerate(values[len(fields):]):
                print(f"    [extra {i}]: {v}")
    except Exception as e:
        print(f"  [ERROR] Could not decode/parse struct: {e}")

class LogHandler(socketserver.BaseRequestHandler):
    def handle(self):
        client_ip = self.client_address[0]
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        print(f"[{now}][{client_ip}]: Connection from {client_ip}")
        buffer = b''
        while True:
            data = self.request.recv(1024)
            if not data:
                break
            buffer += data
            # Split into lines and process each
            lines = buffer.split(b'\n')
            buffer = lines[-1]  # Save incomplete line for next recv
            for line_bytes in lines[:-1]:
                line = line_bytes.decode(errors='replace').rstrip()
                now = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                print(f"[{now}][{client_ip}]: {line}")
                # Look for DATA_TAG and parse
                if "DATA_TAG:" in line:
                    m = re.search(r'NAME:(\w+)\s+FORMAT:([^\s]+)\s+LEN:(\d+)\s+BASE64:([A-Za-z0-9+/=]+)', line)
                    if m:
                        struct_name, fmt, length, b64data = m.groups()
                        print(f"  [DATA_TAG] Struct: {struct_name}, Format: {fmt}, Length: {length}")
                        struct_info = STRUCT_FORMATS.get(struct_name)
                        if struct_info and struct_info["format"] == fmt:
                            parse_and_print_struct(struct_name, struct_info["format"], struct_info["fields"], b64data)
                        else:
                            print("  [DATA_TAG] Unknown struct or format mismatch, raw decode:")
                            parse_and_print_struct(struct_name, fmt, [f"field_{i}" for i in range(int(length))], b64data)
                    else:
                        print("  [DATA_TAG] Could not parse struct log line.")
        print(f"[{now}][{client_ip}]: Connection from {client_ip} closed.")

def run_server():
    with socketserver.ThreadingTCPServer((HOST, PORT), LogHandler) as server:
        print(f"-------- Log server listening on port {PORT} --------")
        server.serve_forever()

if __name__ == "__main__":
    run_server()
