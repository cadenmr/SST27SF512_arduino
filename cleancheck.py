import sys,serial,time

# This script verifies that the EEPROM has been properly reset

# usage: python3 cleancheck.py [serial port]

# eeprom size
correct_filesize = 0xFFFF

if __name__ == '__main__':

    # open the serial port
    try:
        ser = serial.Serial(sys.argv[1], 115200, timeout=2)
    except serial.SerialException:
        print('serial port error')
        quit(1)

    time.sleep(2)

    ser.write(b'\xFF')  # ask the arduino if it's working
    if ser.read(1) != b'\xFE':
        print('unable to communicate with arduino!')
        quit(1)
    else:
        print('connected!')

    if input('type "check" to check >> ') != "check":  # prompt for user confirmation
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
            print('PASS                        ')
            break
        else:  # if we have data, check it

            a = 0
            for x in rx_data:  # go thru all bytes in recived data
                if x != 255:
                    print(f'FAILED: addr {hex(i+a)}, data {hex(x)}')
                    ser.close()
                    quit(1)
                a+=1

            print(f'checking: {round((i/correct_filesize)*100, 1)}% complete...', end='\r')

        i+=512  # increase by read size

    ser.close()
