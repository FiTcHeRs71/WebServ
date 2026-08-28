#!/usr/bin/env python3
import sys, time
time.sleep(int(sys.argv[1]) if len(sys.argv) > 1 else 5)
print("Content-Type: text/plain\r\n\r\nSlow CGI done")
