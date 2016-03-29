#!/usr/bin/env python
import sys
while True:
	b = sys.stdin.read(1)
	if b != "":
		sys.stdout.write("0x%02x,"%ord(b))
	else:
		break
