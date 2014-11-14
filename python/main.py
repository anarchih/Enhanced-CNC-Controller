import serial
import getopt
import sys

ser = None
flag = False


def displayhelp():
    print("List of commands:")
    print("help -- display help")
    print("exit -- quit controller")
    print("shell -- interactive G-code shell")
    print("start -- open serial port to communicate")
    print("file -- start send code from file")


def shell():
    print("Entering Interactive G-code Shell...")
    print("Type return to return to controller")
    while True:
        code = input("G>")
        if(code == "return"):
            print("returning to normal shell")
            return
        elif(code == "help"):
            print("Commands:")
            print("G00 [X(steps)] [Y(steps)] [Z(steps)] [F(feedrate)]; - linear move")
            print("G01 [X(steps)] [Y(steps)] [Z(steps)] [F(feedrate)]; - linear move")
            print("G04 P[seconds]; - delay")
            print("G90; - absolute mode")
            print("G91; - relative mode")
            print("G92 [X(steps)] [Y(steps)]; - change logical position")
            print("M18; - disable motors")
            print("M100; - this help message")
            print("M114; - report position and feedrate")
        else:
            ser.write((code.strip() + "\n").encode("ASCII"))
            ser.readline();

def send():
    fn = input("Please input G-Code file's filename:")
    f = open(fn)
    for l in f:
        print(l.strip())
        ser.write((l.strip() + "\n").encode("ASCII"))
        ser.read(1);





optlist, args = getopt.getopt(sys.argv[1:], 'f:p:')
if optlist:
    f = open(optlist[0][1])
    ser = serial.Serial(optlist[1][1], 9600, timeout=None)
    for l in f:
        print(l.strip())
        ser.write((l.strip() + "\n").encode("ASCII"))
        ser.read(1);

print("Welcome to CNC430 Contorl Program!")
print("type in commands to operate; help to list commands")

while True:
    comm = input(">")
    if(comm == "exit"):
        print()
        print("bye bye")
        exit()
    elif(comm == "help"):
        displayhelp()
    elif(comm == "shell"):
        if(flag):
            shell()
        else:
            print("Please open a serial port first")
    elif(comm == "start"):
        portnum = input("Input Serial port number:")
        ser = serial.Serial(portnum, 9600, timeout=None)
        flag = True
    elif(comm == "file"):
        if(flag):
            send()
        else:
            print("Please open a serial port first")
    else:
        print("I can't get you ;(")
        send

