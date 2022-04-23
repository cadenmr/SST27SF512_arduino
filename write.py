import os,sys,time,serial

# This script writes a 64 kbyte binary to a clean EEPROM.
# It is recommended to verify the chip is clean before using this.

# usage: python3 write.py [serial port] [input filename]

# expected file size
correct_filesize = 65536

if __name__ == '__main__':

    # open the serial port
    try:
        ser = serial.Serial(sys.argv[1], 115200, timeout=1)
    except serial.SerialException:
        print('serial port error')
        quit(1)

    # open the file
    try:
        binfile = open(sys.argv[2], 'rb')
    except IndexError:
        print('bad filename')
        quit(1)
    except FileNotFoundError:
        print('bad filename')
        quit(1)

    # check the input file size
    if os.path.getsize(sys.argv[2]) != correct_filesize:
        print('incorrect file size')
        quit(1)

    time.sleep(1)

    ser.write(b'\xFF')  # ask the arduino if it's working
    if ser.read(1) != b'\xFE':
        print('unable to communicate with arduino!')
        quit(1)
    else:
        print('connected!')

    if input('type "flash" to flash >> ') != "flash":  # prompt for user confirmation
        print('exiting...')
        quit()

    ser.write(b'\x20')  # tell arduino to start

    addr = 0  # address counter
    while True:

        rx_data = ser.read(1)  # grab the next byte from the serial port

        if rx_data == b'\xFF':  # if the arduino is ready for more data
            i = 0  # start index counter at zero
            while True:

                tx_data = binfile.read(1)  # pull the next byte from the file
                ser.write(tx_data)  # send the data out
                addr+=1

                # break out if we've sent enough data or we don't have any left
                if i > 50 or tx_data == b'':
                    break

                print(f'flashing: {round((addr/correct_filesize)*100, 1)}% complete...', end='\r')

                i+=1

        elif rx_data == b'\xFD':
            print('done                                ')
            break

    binfile.close()
    ser.close()
