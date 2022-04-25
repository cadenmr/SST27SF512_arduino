import sys,serial,time

# This script wipes the EEPROM

# usage: python3 wipe.py [serial port]

if __name__ == '__main__':

    # open the serial port
    try:
        ser = serial.Serial(sys.argv[1], 115200, timeout=2)
    except serial.SerialException:
        print('serial port error')
        quit(1)

    time.sleep(1)

    ser.write(b'\xFF')  # ask the arduino if it's working
    if ser.read(1) != b'\xFE':
        print('unable to communicate with arduino!')
        quit(1)
    else:
        print('connected!')

    if input('type "wipe" to wipe >> ') != "wipe":  # prompt for user confirmation
        print('exiting...')
        quit()

    ser.write(b'\x30')  # tell the arduino to wipe

    # check for proper response from arduino
    rx_data = ser.read(0x01)
    if rx_data != b'\xFD':
        print('wiping failed')
        quit(1)
    else:
        print('wipe successful')
        ser.close()
