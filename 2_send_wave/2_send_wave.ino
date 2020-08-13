int running_mode = 0;
// 0 - idle
// 1 - sine wave

float cur_step = 0;
float cur_speed = 0.005;

bool frame_ack = false;
int frame_position = 0;
int frame_force = 1023;

int last_send;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  if (Serial.available()) {
    int inByte = Serial.read();
    if (inByte == 120) {
      startModeIdle();
    } else if (inByte == 119) {
      startModeWave();
    }
  }
  
  sendFrame();

  switch (running_mode) {
    case 1:
      stepModeWave();
      break;
    default:
      stepModeIdle();
      break;
  }
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[128];
  
  sprintf(format, "%%.%dX", precision);
  
  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void sendFrame() {
  byte state_code[7];

  // status byte
  byte status_byte = 0x80;

  if (frame_ack) {
    status_byte = status_byte | 0x01;
  }

  state_code[0] = status_byte;

  // position bytes
  int f_value = frame_position;
  bool f_negative = false;

  if (f_value < 0) {
    // if negative invert and mark negative
    f_value = f_value * -1;
    f_negative = true;
  }

  byte f_l = f_value & 0xFF;
  byte f_h = f_value >> 8 & 0x07;

  if (f_negative) {
    f_h = f_h | 0x08;
  }
  
  state_code[1] = f_l;
  state_code[2] = f_h;
  

  // position bytes
  f_value = frame_force;
  f_negative = false;

  if (f_value < 0) {
    // if negative invert and mark negative
    f_value = f_value * -1;
    f_negative = true;
  }

  f_l = f_value & 0xFF;
  f_h = f_value >> 8 & 0x07;

  if (f_negative) {
    f_h = f_h | 0x08;
  }
  
  state_code[3] = f_l;
  state_code[4] = f_h;

  // checksum
  int checksum = 0;
  
  for (int i = 0; i < 5; i++) {
    checksum = checksum + state_code[i];
  }
  
  f_l = checksum & 0xFF;
  f_h = checksum >> 8 & 0xFF;

  state_code[5] = f_l;
  state_code[6] = f_h;

  for (int i = 0; i < 7; i++) {
    printHex(state_code[i], 2);
  }

  Serial.println();
  Serial1.write(state_code, 7);
  delay(1);
}

void startModeIdle() {
  running_mode = 0;
  frame_ack = false;
}

void startModeWave() {
  running_mode = 1;
  frame_ack = true;
}

void stepModeIdle() {
  // return to zero if not currently there
  if (frame_position != 0) {
    if ((cur_step > 0 && cur_step < cur_speed) || (cur_step < 0 && cur_step > (cur_speed * -1))) {
      frame_position = 0;
    } else {
      float next_speed = cur_speed;
      if (cur_step < 0.5) {
        // invert to go backwards towards zero
        next_speed = next_speed * -1;
      }

      cur_step = cur_step + next_speed;
      frame_position = sin(cur_step) * 1000;
    }
  }
}

void stepModeWave() {
    cur_step = cur_step + cur_speed;
    if (cur_step == 1) {
      cur_step = 0;
    }
    frame_position = sin(cur_step) * 1000;
}
