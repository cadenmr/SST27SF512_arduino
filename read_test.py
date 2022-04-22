import serial,time

s = serial.Serial('/dev/ttyACM0', 115200)
f = open('out2.bin', 'wb')

time.sleep(3)

s.write(b'\x10')
s.flush()
print(f'chip size reported: {s.read(0x02).hex().upper()}')
s.write(b'\xFF')
f.write(s.read(0x10000))

f.close()
