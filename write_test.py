import serial,os,time

f = open('testfile.bin', 'rb')
s = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

time.sleep(3)

s.write(b'\x20')

time.sleep(1)

read_ct = 0

while True:

    in_data = s.read(1)
    print(in_data)

    if in_data == b'\xFF':
        i = 0
        while True:

            d = f.read(1)
            i+=1

            read_ct+=1
            print(read_ct)

            if d == b'' or i > 50:
                break
            else:
                s.write(d)
