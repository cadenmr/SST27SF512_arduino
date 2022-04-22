// pin assignments
// control signals
#define ce_pin 22
#define oe_pin 24
// address (A0, A1 ... A15)
unsigned char addr_pins[16] = {23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53};
// data (D0, D1 ... D7)
unsigned char data_pins[8] = {38, 40, 42, 44, 46, 48, 50, 52};

// constants
const unsigned char comm_rdy =  0xFF;
const unsigned char comm_ack = 0xFE;
const unsigned char comm_pause = 0xFD;
const unsigned int chip_size = 65535;
const byte chip_size_buf[] = {0xFF, 0xFF};

// variables
unsigned char state = 0;
bool running = false;

void setup() {
  Serial.begin(115200);  // begin serial communication @ 115200 baud
  Serial.setTimeout(2000); // 2 sec timeout
  while (!Serial) {  // wait for serial port to connect
    ;
  }
}

void loop() {

	switch(state) {  // what state are we in?
  
    case 0:  // STATE: connection validation & mode select
      {
        if (Serial.available() > 0) {  // wait for incoming data
          switch(Serial.read()){  // what did we get?
            case comm_rdy:  // rx RDY?
              Serial.write(comm_ack);  // tx comm_ack
              break;
  
            case 0x10:  // rx READ
              state = 1;
              break;
  
            case 0x20:  // rx WRITE
              state = 2;
              break;
          }
        }
        break;
      }

    case 1:  // STATE: chip read
      {
        if (!running) {
          // send the chip size (0xFFFF)
          Serial.write(chip_size_buf, 2);
  
          // check for a valid response after sending chip size
          byte in_data[1] = {0x00};
          if (Serial.readBytes(in_data, 1) == 0) { // timed out
            state = 0;
          } else if (in_data[0] != comm_ack) {  // check if we got a good ack
            state = 0;
          } else {  // we passed all checks. break out
            running = true;
            delay(25);  // 25 ms delay before continue
          }
        }

        // set up our pins
        read_init();

        // we have to use an infinite while loop here because we can't detect 
        // an overflow with a for loop
        unsigned int addr = 0;
        while (true){
          unsigned char data = read_byte(addr);
          Serial.write(data);  // send the data off
          
          addr++;  // increment the address counter
          
          if (addr == 0) {break;}  // if we've overflowed, we're done
        }
        
        state = 0;
        running = false;
        break;
      }
      
    case 2:  // STATE: chip write
      {

        write_init();  // initialize for writing

        unsigned int addr = 0;
        bool sent_ready = false;
        while (true) {

          if (Serial.available() > 0) {  // if we've recieved data

            sent_ready = false;

            unsigned char rx_byte = Serial.read();  // read one byte

            write_byte(addr, rx_byte);
            addr++;

            if (addr == 0) {Serial.println("DONE!"); break;} // we've overflowed, we're done.

          } else {
            if (!sent_ready) {
              Serial.write(comm_rdy);
              sent_ready = true;
            }
        }
      }
      state = 0;
      break;
	}
  }
}

// sets up the pins for reading the chip
void read_init() {

  pinMode(ce_pin, OUTPUT);
  pinMode(oe_pin, OUTPUT);

  // set up control pins while we finish setup
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);

  // set up address pins as outputs
  for (char i = 0; i < 16; i++) {
    pinMode(addr_pins[i], OUTPUT);
  }

  // set up data pins as inputs
  for (char i = 0; i < 8; i++) {
    pinMode(data_pins[i], INPUT);
  }

}

// sets up the pins for writing the chip
void write_init() {

  pinMode(ce_pin, OUTPUT);
  pinMode(oe_pin, OUTPUT);

  // set up control pins while we finish setup
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);

  // set up address pins as outputs
  for (char i = 0; i < 16; i++) {
    pinMode(addr_pins[i], OUTPUT);
  }

  // set up data pins as outputs
  for (char i = 0; i < 8; i++) {
    pinMode(data_pins[i], OUTPUT);
  }

}

byte read_byte(unsigned int address) {

  byte out_data = 0x00;

  // de-assert the control signals
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);

  // set up the address lines
  unsigned int addr_pin_sel = 0b0000000000000001;
  for (char i = 0; i < 16; i++) {  // go through each pin starting at index 0 (D0) to 15 (D15)

    bool pin_state = false;
    // check if we want this pin on or off
    // shift the pin select variable by the index and bitwise AND with the address variable
    // if the result is not zero, we want the pin on
    if (((addr_pin_sel << i) & address) != 0) {
      pin_state = true;
    }

    digitalWrite(addr_pins[i], pin_state);

  }

  // assert chip enable and output enable
  digitalWrite(ce_pin, LOW);
  digitalWrite(oe_pin, LOW);

  // wait the required time
  delay(0.001);

  // we should have valid data now. read the pins and store in array
  unsigned char read_pins[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (unsigned char i = 0; i < 8; i++) {
    read_pins[i] = digitalRead(data_pins[i]);
  }

  // make our data byte from the array of bits
  // bitwise OR the out data byte with the bit state, shifted left by the index
  for (char i = 0; i < 8; i++) {
    out_data = out_data | (read_pins[i] << i);
  }

  // de-assert the control signals
  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);
  
  return out_data;
  
}

void write_byte(unsigned int address, unsigned char data) {

  digitalWrite(ce_pin, HIGH);
  digitalWrite(oe_pin, HIGH);

  // set up the address lines
  const unsigned int addr_pin_sel = 0b0000000000000001;
  for (unsigned char i = 0; i < 16; i++) {  // go through each pin starting at index 0 (D0) to 15 (D15)

    bool pin_state = false;
    // check if we want this pin on or off
    // shift the pin select variable by the index and bitwise AND with the address variable
    // if the result is not zero, we want the pin on
    if (((addr_pin_sel << i) & address) != 0) {
      pin_state = true;
    }

    digitalWrite(addr_pins[i], pin_state);

  }

  const unsigned char data_pin_sel = 0b00000001;
  for (unsigned char i = 0; i < 8; i++) {
    
    bool pin_state = false;
    // check if we want this pin on or off
    // shift the pin select variable by the index and bitwise AND with the address variable
    // if the result is not zero, we want the pin on
    if (((data_pin_sel >> i) & address) != 0) {
      pin_state = true;
    }

    digitalWrite(data_pins[i], pin_state);
    
  }

  delay(0.02);  // wait for the required time
  digitalWrite(ce_pin, LOW);  // assert control signals
  digitalWrite(oe_pin, LOW);
  delay(0.02);  // wait for the required time
  digitalWrite(ce_pin, HIGH);  // de-assert control signals
  digitalWrite(oe_pin, HIGH);
  
}
