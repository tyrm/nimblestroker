
byte state_code[7];
int state_code_cur = 0;
bool state_code_reading = false;

void setup() {
  // initialize both serial ports:
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  if (Serial1.available()) {
    int inByte = Serial1.read();

    if (state_code_reading) {
      // Store byte and 
      state_code[state_code_cur] = inByte;
      state_code_cur++;

      // buffer is full, process
      if (state_code_cur == 7) {
        processBuffer();
        state_code_cur = 0;
        state_code_reading = false;
      }
    } else {
      byte system_type = inByte & 0xE0;
      if (system_type == 0x80) {
        state_code_reading = true;
        state_code[state_code_cur] = inByte;
        state_code_cur++;
      }
    }
  }
}


void processBuffer() {
  int checksum = word(state_code[6], state_code[5]);
  int calc_checksum = 0;
  for (int i = 0; i < 5; i++) {
    calc_checksum = calc_checksum + state_code[i];
  }

  if (calc_checksum == checksum) {
    byte t_high = state_code[2] & 0x03;
    byte t_low = state_code[1];

    byte t_sign = state_code[2] & 0x04;

    int pos = word(t_high, t_low);

    if (t_sign == 0x04) {
      pos = pos * -1;
    }

    Serial.print(pos, DEC);
    
    //for (int i = 0; i < 7; i++) {
    //  printHex(state_code[i], 2);
    //}
    Serial.println();
  }
  
}


void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}
