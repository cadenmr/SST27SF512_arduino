import os,sys,time,serial

# This script writes a 64 kbyte binary to a clean EEPROM.
# It is recommended to verify the chip is clean before using this.

# usage: python3 write.py [serial port] [input filename]

# expected file size
correct_filesize = 65536
# arduino data chunk size in bytes
chunk_size = 63

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

    time.sleep(1)  # wait for arduino to reset after serial connection

    # ask the arduino if it's working
    ser.write(b'\xFF')
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

        rx_data = ser.read()  # grab the next byte from the serial port

        if rx_data == b'\xFF':  # if we got the ready byte

            tx_data = binfile.read(chunk_size)  # pull the next byte from the file

            ser.write(tx_data)  # send the data out
            addr+=len(tx_data)  # increment by lenth of data to account for a read of less than chunk size

            # update status
            print(f'flashing: {round((addr/correct_filesize)*100, 1)}% complete...', end='\r')

        elif rx_data == b'\xFD':  # if we got the stop byte
            print('done                                ')
            break

        elif rx_data == b'':
            raise RuntimeError('timed out')

    binfile.close()
    ser.close()
