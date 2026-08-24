#!/usr/bin/env python3
import sys, os
n = int(os.environ.get("CONTENT_LENGTH") or 0)
data = sys.stdin.buffer.read(n)
print("Content-Type: text/plain\r\n")
sys.stdout.buffer.write(data)
