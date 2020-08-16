// Config
bool serial_output_plotter = true;

// Globals
int running_mode = 0; // 0 - idle,  1 - sine wave
float running_hz = 1.0;

float cur_step = 0;

String serial_buffer = "";

int last_send;


// Frame Values
bool frame_ack = false;
int frame_position = 0;
int frame_force = 1023;

// Config
int wave_max = 750;
int wave_min = 750;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  if (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      processSerialBuffer();
    } else {
      serial_buffer += inChar;
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

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[128];
  
  sprintf(format, "%%.%dX", precision);
  
  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void processSerialBuffer() {
  String command = getValue(inputString, ',', 0);
  if (command == "LEVEL") {
    String channel = getValue(inputString, ',', 1);
    
    switch (channel.toInt()) {
      case 0:
        ledcWrite(nipple0Channel, level.toInt());   
        break;
      case 1:
        ledcWrite(nipple1Channel, level.toInt());   
        break;
      
    }
  }

  inputString = "";
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
  byte f_h = f_value >> 8 & 0x03;

  if (f_negative) {
    f_h = f_h | 0x04;
  }
  
  state_code[1] = f_l;
  state_code[2] = f_h;
  
  // force bytes
  f_value = frame_force;
  f_negative = false;

  if (f_value < 0) {
    // if negative invert and mark negative
    f_value = f_value * -1;
    f_negative = true;
  }

  f_l = f_value & 0xFF;
  f_h = f_value >> 8 & 0x03;

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

  if (serial_output_plotter) {
    Serial.print(cur_step, DEC);
    Serial.print(",");
    Serial.print(frame_position, DEC);
    Serial.println();
  } else {
    for (int i = 0; i < 7; i++) {
      printHex(state_code[i], 2);
    }
    Serial.println();
  }

  Serial1.write(state_code, 7);
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
    if ((cur_step > 0 && cur_step <= 1) || (cur_step < 0 && cur_step >= -1) || (cur_step < 180 && cur_step >= 179) || (cur_step > 180 && cur_step <= 181)) {
      // if the step is less than the next delta go to zero to prevent wabling
      cur_step = 0;
      frame_position = 0;
    } else {
      float next_speed = 1;
      if (cur_step <= 90 || (180 <= cur_step && cur_step <= 270)){
        // invert to go backwards towards zero
        next_speed = -1;
      }

      cur_step = cur_step + next_speed;
      if (cur_step >= 360) {cur_step = 0;}
      frame_position = sin(radians(cur_step)) * wave_max;
    }
  }
}

void stepModeWave() {
  int r_millis = running_hz * 1000;
  int m_millis = millis() % r_millis;
  float t_pos = float(m_millis) / r_millis;

  cur_step = t_pos * 360;
  
  frame_position = sin(radians(cur_step)) * wave_max;
}
