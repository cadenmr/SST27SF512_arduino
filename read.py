import sys,serial,time

# This script reads the EEPROM from hex 0000-FFFF.

# usage: python3 read.py [serial port] [outptut filename]

# eeprom size
correct_filesize = 0xFFFF

if __name__ == '__main__':

    # open the serial port
    try:
        ser = serial.Serial(sys.argv[1], 115200, timeout=2)
    except serial.SerialException:
        print('serial port error')
        quit(1)

    # open the file
    try:
        binfile = open(sys.argv[2], 'wb')
    except IndexError:
        print('bad filename')
        quit(1)

    time.sleep(1)

    ser.write(b'\xFF')  # ask the arduino if it's working
    if ser.read(1) != b'\xFE':
        print('unable to communicate with arduino!')
        quit(1)
    else:
        print('connected!')

    if input('type "read" to read >> ') != "read":  # prompt for user confirmation
        print('exiting...')
        quit()

    ser.write(b'\x10')  # tell the arduino to start reading

    # check for proper response from arduino
    rx_data = ser.read(0x02)
    if rx_data != b'\xFF\xFF':
        print(f'got bad response from arduino: got 0x{rx_data.hex().upper()}, should be 0xFFFF')
        quit(1)

    ser.write(b'\xFF')  # tell arduino computer is ready

    i = 0  # start index var at zero
    while True:
        rx_data = ser.read(512)  # read 512 bytes from arduino

        if rx_data == b'':  # check if the data we read was null
            print('done                        ')
            break
        else:  # if we have data, update the status and write it out
            print(f'reading: {round((i/correct_filesize)*100, 1)}% complete...', end='\r')
            binfile.write(rx_data)

        i+=512  # increase by read size

    binfile.close()
    ser.close()
