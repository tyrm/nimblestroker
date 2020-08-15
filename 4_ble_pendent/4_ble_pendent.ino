#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID                "00000000-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_MODE    "00000001-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_AIR     "00000002-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_STROKE  "00000003-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_SPEED   "00000004-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_TEXTURE "00000005-e1c6-4512-bad0-16cdcf3369ab"
#define CHARACTERISTIC_UUID_NATURE  "00000006-e1c6-4512-bad0-16cdcf3369ab"

// Config
const bool idle_on_client_disconnect = true;
const int idle_return_speed = 3;
const bool serial_output_plotter = true;

// Globals
BLECharacteristic *modeCharacteristic;
BLECharacteristic *airCharacteristic;
BLECharacteristic *strokeCharacteristic;
BLECharacteristic *speedCharacteristic;
BLECharacteristic *textureCharacteristic;
BLECharacteristic *natureCharacteristic;

uint8_t ns_mode = 0;      // 0 - idle, 1 - wave
uint16_t ns_stroke = 750; // maximum magnitude of stroke (primary wave magnitude will be ns_stroke - ns_texture)
float ns_speed = 1.0;     // speed in hz of the primary wave
uint16_t ns_texture = 0;  // magnitude of the secondary wave
float ns_nature = 10;     // speed in hz of the secondary wave

bool device_connected = false;

int wave1_degrees = 0;
int wave2_degrees = 0;

bool frame_ack = false;
bool frame_air_in = false;
bool frame_air_out = false;
int frame_position = 0;
int frame_force = 1023;

// Functions
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
  if (frame_air_in) {
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

void stepModeIdle() {
  // return to zero
  if ((idle_return_speed * -1) < frame_position && frame_position < 0) {
    frame_position = 0;
  }
  else if (0 < frame_position && frame_position < idle_return_speed) {
    frame_position = 0;
  }
  else if (frame_position > 0) {
    frame_position = frame_position - idle_return_speed;
  }
  else if (frame_position < 0) {  
    frame_position = frame_position + idle_return_speed;
  }
}

void stepModeWave() {
  int speed_millis = 1000 / ns_speed;
  int mod_millis = millis() % speed_millis;
  float temp_pos = float(mod_millis) / speed_millis;
  wave1_degrees = temp_pos * 360;

  speed_millis = 1000 / ns_nature;
  mod_millis = millis() % speed_millis;
  temp_pos = float(mod_millis) / speed_millis;
  wave2_degrees = temp_pos * 360;

  int wave1_max = ns_stroke - ns_texture;
  int wave1_postition = sin(radians(wave1_degrees)) * wave1_max;

  int wave2_postition = sin(radians(wave2_degrees)) * ns_texture;
  
  frame_position = wave1_postition + wave2_postition;
}

// Callback Classes
class NSServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    device_connected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    device_connected = false;

    if (idle_on_client_disconnect) {
      ns_mode = 0; // Go idle on client disconnect
    }
  }
};

class ModeCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx_value = pCharacteristic->getValue();

    // Validate
    if (rx_value.length() == 1) {
      if (0 <= rx_value[0] && rx_value[0] <= 1) {
        ns_mode = rx_value[0];
      }
      else {
        // Invalid input. Reset to previous value. 
        pCharacteristic->setValue(&ns_mode, 1);
      }
    }
    else {
      // Invalid input. Reset to previous value. 
      pCharacteristic->setValue(&ns_mode, 1);
    }
  }
};

class StrokeCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx_value = pCharacteristic->getValue();

    // Validate
    if (rx_value.length() == 2) {
      uint16_t new_stroke = word(rx_value[1], rx_value[0]);
      
      if (0 <= new_stroke && new_stroke <= 1000) {
        ns_stroke = new_stroke;
      }
      else {
        // Invalid input. Reset to previous value. 
        pCharacteristic->setValue(ns_stroke);
      }
    }
    else {
      // Invalid input. Reset to previous value. 
      pCharacteristic->setValue(ns_stroke);
    }
  }
};

class SpeedCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx_value = pCharacteristic->getValue();

    if (rx_value.length() == 4) {
      union u_tag {
         byte b[4];
         float fval;
      } bytesToFloat;
      
      for (byte i = 0;i < 4; i++) {
        bytesToFloat.b[i] = rx_value[i];
      }
      
      float new_speed = bytesToFloat.fval;
      ns_speed = new_speed;
    }
    else {
      // Invalid input. Reset to previous value. 
      pCharacteristic->setValue(ns_speed);
    }
  }
};

class TextureCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx_value = pCharacteristic->getValue();

    // Validate
    if (rx_value.length() == 2) {
      uint16_t new_texture = word(rx_value[1], rx_value[0]);
      
      if (0 <= new_texture && new_texture <= 1000) {
        ns_texture = new_texture;
      }
      else {
        // Invalid input. Reset to previous value. 
        pCharacteristic->setValue(ns_texture);
      }
    }
    else {
      // Invalid input. Reset to previous value. 
      pCharacteristic->setValue(ns_texture);
    }
  }
};

class NatureCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx_value = pCharacteristic->getValue();

    if (rx_value.length() == 4) {
      union u_tag {
         byte b[4];
         float fval;
      } bytesToFloat;
      
      for (byte i = 0;i < 4; i++) {
        bytesToFloat.b[i] = rx_value[i];
      }
      
      float new_nature = bytesToFloat.fval;
      ns_nature = new_nature;
    }
    else {
      // Invalid input. Reset to previous value. 
      pCharacteristic->setValue(ns_nature);
    }
  }
};

// Application
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  // Create the BLE Device
  BLEDevice::init("NIMBSTROKE-1");
  
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new NSServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create Mode Characteristic 
  modeCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID_MODE,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_WRITE
                       );

  
  modeCharacteristic->setValue(&ns_mode, 1);
  modeCharacteristic->setCallbacks(new ModeCallbacks());
  
  // Create Stroke Characteristic 
  strokeCharacteristic = pService->createCharacteristic(
                           CHARACTERISTIC_UUID_STROKE,
                           BLECharacteristic::PROPERTY_READ |
                           BLECharacteristic::PROPERTY_WRITE
                         );

  
  strokeCharacteristic->setValue(ns_stroke);
  strokeCharacteristic->setCallbacks(new StrokeCallbacks());
  
  // Create Speed Characteristic 
  speedCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID_SPEED,
                          BLECharacteristic::PROPERTY_READ |
                          BLECharacteristic::PROPERTY_WRITE
                        );

  
  speedCharacteristic->setValue(ns_speed);
  speedCharacteristic->setCallbacks(new SpeedCallbacks());
  
  // Create Texture Characteristic 
  textureCharacteristic = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_TEXTURE,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE
                          );

  
  textureCharacteristic->setValue(ns_texture);
  textureCharacteristic->setCallbacks(new TextureCallbacks());
  
  // Create Nature Characteristic 
  natureCharacteristic = pService->createCharacteristic(
                           CHARACTERISTIC_UUID_NATURE,
                           BLECharacteristic::PROPERTY_READ |
                           BLECharacteristic::PROPERTY_WRITE
                         );

  
  natureCharacteristic->setValue(ns_nature);
  natureCharacteristic->setCallbacks(new NatureCallbacks());
  
  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}

void loop() {
  sendFrame();

  switch (ns_mode) {
    case 1:
      stepModeWave();
      break;
    default:
      stepModeIdle();
      break;
  }
}
