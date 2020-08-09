
TaskHandle_t frame_send_task;


int running_mode = 0;
// 0 - idle
// 1 - sine wave

float cur_step = 0;
float cur_speed = 0.005;

int cur_position = 0;
int cur_force = 1023;

int last_send;

int empty_ticks = 0;


void setup() {
  Serial.begin(115200);
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
  
  if (last_send != millis()){ //1khz ish
    last_send = millis();
    sendFrame();

    switch (running_mode) {
      case 1:
        stepModeWave();
        break;
      default:
        stepModeIdle();
        break;
    }
    
  } else {
    empty_ticks++;
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
  Serial.print(cur_position, DEC);
  Serial.print(",");
  Serial.println(empty_ticks, DEC);
  empty_ticks = 0;
}

void sendFrameTask() {
  }

void startModeIdle() {
  running_mode = 0;
}

void startModeWave() {
  running_mode = 1;
}

void stepModeIdle() {
}

void stepModeWave() {
    cur_step = cur_step + cur_speed;
    if (cur_step == 1) {
      cur_step = 0;
    }
    cur_position = cos(cur_step) * 1000;
}
