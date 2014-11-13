import serial
import math

ser = serial.Serial(4,9600,timeout = None)

fn = input("Please input G-Code file's filename:")
f = open(fn)
for l in f:
	print(l.strip())
	ser.write((l.strip() + "\n").encode("ASCII"))
	ser.read(1)
