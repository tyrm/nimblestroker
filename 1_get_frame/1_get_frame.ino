
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
  for (int i = 0; i < 7; i++) {
    printHex(state_code[i], 2);
  } 

  Serial.println();
}


void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}
