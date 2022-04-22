import serial,os,time

f = open('testfile.bin', 'rb')
s = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

time.sleep(3)

s.write(b'\x20')

time.sleep(1)

read_ct = 0

while True:

    in_data = s.read(1)

    if in_data == b'\xFF':
        i = 0
        while True:

            d = f.read(1)
            i+=1

            s.write(d)
            print(f'{round((read_ct/0xFFFF)*100,1)}% done - data: {d.hex().upper()}', end='\r')
            read_ct+=1

            if i > 50 or d == b'':
                break

    elif in_data == b'\xFD':
        print('\ncomplete')
        break
