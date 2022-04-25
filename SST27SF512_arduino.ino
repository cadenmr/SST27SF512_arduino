// pin assignments
// control signals
#define ce_pin 2
#define oe_pin 3
#define oe_12v 6
// address (A0, A1 ... A15)
unsigned char addr_pins[16] = {23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53};
// data (D0, D1 ... D7)
unsigned char data_pins[8] = {36, 34, 32, 30, 28, 26, 24, 22};

// constants
const unsigned char comm_rdy =  0xFF;
const unsigned char comm_ack = 0xFE;
const unsigned char comm_done = 0xFD;
const unsigned int chip_size = 65535;
const byte chip_size_buf[] = {0xFF, 0xFF};

// variables
unsigned char state = 0;
bool running = false;

void setup() {

  Serial.begin(115200);  // begin serial communication @ 115200 baud
  Serial.setTimeout(2000); // 2 sec timeout

  while (!Serial) {;}  // wait for serial port to connect

}

void loop() {

	switch(state) {  // what state are we in?
  
      case 0:  // STATE: connection validation & mode select
        {
          if (Serial.available() > 0) {  // wait for incoming data

            switch(Serial.read()) {  // what did we get?

              case comm_rdy:  // recived comm_rdy
                Serial.write(comm_ack);  // recived comm_ack
                break;

              case 0x10:  // recived read mode selection
                state = 1;
                break;

              case 0x20:  // recived write mode selection
                state = 2;
                break;

              case 0x30:  // recieved erase mode selection
                state = 3;
                break;

            }
          }
          break;
        }

      case 1:  // chip reading routine
        {
          if (!running) {

            Serial.write(chip_size_buf, 2);  // send the chip size (0xFFFF)

            // check for a valid response after sending chip size
            byte in_data[1] = {0x00};
            if (Serial.readBytes(in_data, 1) == 0) { // timed out
              state = 0;
            } else if (in_data[0] != comm_ack) {  // check if we got a good ack
              state = 0;
            } else {  // we passed all checks. go time.
              running = true;
            }

          } else {

            read_init();  // initialize for reading

            // read each byte and send it out
            unsigned int addr = 0x00;  // address counter starts at zero
            while (true){
              unsigned char data = read_byte(addr);
              Serial.write(data);  // send the data off

              addr++;  // increment the address counter

              if (addr == 0x00) {break;}  // if we've overflowed, we're done
            }

            running = false;  // stop running
            state = 0;  // go back to the mode selection

          }
          break;
        }

      case 2:  // chip writing routine
        {

          write_init();  // initialize for writing
          digitalWrite(oe_12v, HIGH);  // switch the OE pin relay to 12v mode
          delay(50);  // 50ms delay for relay to switch

          unsigned int addr = 0x00;  // start the address counter at zero
          bool sent_ready = false;  // remember if we've sent the "ready for more data" signal or not
          running = true;  // start running
          while (true) {

            if (Serial.available() > 0 && running) {  // if we've recieved data and we're running

              sent_ready = false;  // we're going to need to send "ready" again. unset the var

              unsigned char rx_byte = Serial.read();  // read one byte from serial input
              write_byte(addr, rx_byte);  // write the byte to the chip

              addr++;  // increment the address

              if (addr == 0x00) {  // we've overflowed back to zero, we're done.
                running = false;  // we're no longer running. unset the var
                break;  // we're done
              }

            } else if (Serial.available() <= 0 && running) {  // if our buffer is empty
              if (!sent_ready && running) {  // if we haven't sent the ready message yet and we're running
                Serial.write(comm_rdy);  // send the ready message
                sent_ready = true;  // set the var so we don't send it again
              }
          }
        }

        // finish write sequence as per datasheet
        digitalWrite(oe_pin, LOW);
        digitalWrite(oe_12v, LOW);  // switch the OE pin relay back to 5v control
        delay(50);  // 50ms delay for relay to switch
        digitalWrite(ce_pin, LOW);

        Serial.write(comm_done);  // send the "done" signal
        state = 0;  // go back to mode select
        break;
      }

    case 3:
    {

      write_init();  // initialize for write

      // erase chip following datasheet timing diagram
      digitalWrite(ce_pin, HIGH);
      digitalWrite(oe_pin, LOW);
      digitalWrite(oe_12v, HIGH);
      delay(50);
      digitalWrite(ce_pin, LOW);
      delay(200);
      digitalWrite(ce_pin, HIGH);
      delayMicroseconds(4);
      digitalWrite(oe_12v, LOW);
      delay(60);
      digitalWrite(ce_pin, LOW);

      Serial.write(comm_done);  // tell computer we're done

      state = 0;  // go back to mode sel
      break;

    }
  }
}

// sets up the pins for reading the chip
void read_init() {

  pinMode(ce_pin, OUTPUT);
  pinMode(oe_pin, OUTPUT);
  pinMode(oe_12v, OUTPUT);

  // set up control pins while we finish
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);
  digitalWrite(oe_12v, LOW);

  // go through address pin array and set all as outputs
  for (unsigned char i = 0; i < 16; i++) {
    pinMode(addr_pins[i], OUTPUT);
  }

  // go through data pin array and set all as outputs
  for (unsigned char i = 0; i < 8; i++) {
    pinMode(data_pins[i], INPUT);
  }

}

// sets up the pins for writing the chip
void write_init() {

  pinMode(ce_pin, OUTPUT);
  pinMode(oe_pin, OUTPUT);
  pinMode(oe_12v, OUTPUT);

  // set up control pins while we finish
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, LOW);
  digitalWrite(oe_12v, LOW);

  // go through address pin array and set all as outputs
  for (unsigned char i = 0; i < 16; i++) {
    pinMode(addr_pins[i], OUTPUT);
  }

  // go through data pin array and set all as outputs
  for (unsigned char i = 0; i < 8; i++) {
    pinMode(data_pins[i], OUTPUT);
  }

}

byte read_byte(unsigned int address) {

  // de-assert the control signals
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);

  // set up the address lines
  unsigned int addr_pin_sel = 0b0000000000000001;  // used to select which pin we want
  for (unsigned char i = 0; i < 16; i++) {  // go through each address pin (starting at A0) and set it appropriately

    bool pin_state = false;
    // shift the pin select variable by the index, then bitwise AND with the address variable
    // if the result is not zero, we want the pin on
    if (((addr_pin_sel << i) & address) != 0) {
      pin_state = true;
    }

    digitalWrite(addr_pins[i], pin_state);

  }

  // assert control signals
  digitalWrite(ce_pin, LOW);
  digitalWrite(oe_pin, LOW);

  delayMicroseconds(3);  // wait minumum accurate time to ensure a valid read

  // we should have valid data now. read the pins and store in array
  unsigned char read_pins[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (unsigned char i = 0; i < 8; i++) {
    read_pins[i] = digitalRead(data_pins[i]);
  }

  // make our data byte from the array of bits
  // shift the desired pin reading left by its index, then bitwise OR the data byte by the result
  byte out_data = 0x00;
  for (unsigned char i = 0; i < 8; i++) {
    out_data = out_data | (read_pins[i] << i);
  }

  // de-assert the control signals
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);
  
  return out_data;
  
}

void write_byte(unsigned int address, unsigned char data) {

  digitalWrite(ce_pin, HIGH);  // de-assert control signal

  // set up the address lines
  const unsigned int addr_pin_sel = 0b0000000000000001;  // used to select which pin we want
  for (unsigned char i = 0; i < 16; i++) {  // go through each address pin (starting at A0) and set it appropriately

    bool pin_state = false;
    // shift the pin select variable by the index, then bitwise AND with the address variable
    // if the result is not zero, we want the pin on
    if (((addr_pin_sel << i) & address) != 0) {
      pin_state = true;
    }

    digitalWrite(addr_pins[i], pin_state);  // write it to the pin

  }

  // set up the data lines
  const unsigned char data_pin_sel = 0b00000001;  // used to select which pin we want
  for (unsigned char i = 0; i < 8; i++) {  // go through each data pin (starting at D0) and set it appropriately
    
    bool pin_state = false;
    // shift the pin select variable by the index, then bitwise AND with the data variable
    // if the result is not zero, we want the pin on
    if (((data_pin_sel << i) & data) != 0) {
      pin_state = true;
    }

    digitalWrite(data_pins[i], pin_state);  // write it to the pin
    
  }

  delayMicroseconds(3);  // wait minimum accurate time to ensure a valid write
  digitalWrite(ce_pin, LOW);  // assert control signals
  delayMicroseconds(18);  // wait for the required time, accounting for slow digitalWrite
  digitalWrite(ce_pin, HIGH);  // de-assert control signals
  
}
