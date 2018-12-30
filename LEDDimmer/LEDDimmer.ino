#define ZC_PIN 2
#define DATA_PIN 3
#define WARM_UP_CYCLES 5

enum State {
  WAITING_FOR_ZC,
  WAITING_FOR_TIME,
  HIGH_SIGNAL,
} current_state;

uint16_t microsInterval = 10000;
unsigned long lastInterruptTime = 0;
unsigned long startPulseAt = 0;
unsigned long stopPulseAt = 0;
uint8_t brightness = 0;
uint8_t targetBrightness = 0;
uint8_t fullCyclesLeft = 0;


void update_brightness() {
  // We want to gradually change the brightness, 
  // so we do it 1/255 max every 10 milliseconds.

  if (targetBrightness == brightness) {
    return;
  } else if (targetBrightness > brightness) {
    brightness++;
  } else {
    brightness--;
  }
}

void on_zc() {
  // Zero cross detected
  if (current_state == HIGH_SIGNAL) {
    digitalWrite(DATA_PIN, LOW);
  }
  microsInterval = micros() - lastInterruptTime;
  lastInterruptTime = micros();

  update_brightness();
  
  if(brightness == 0) {
    return;
  }
  current_state = WAITING_FOR_TIME;

  uint8_t real_brightness = brightness;
  
  if(fullCyclesLeft > 0) {
    fullCyclesLeft--;
    real_brightness = 255;
  }
  startPulseAt = lastInterruptTime + map(real_brightness, 0, 255, microsInterval*0.9-100, 400);
  stopPulseAt = startPulseAt + 100;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(ZC_PIN, INPUT_PULLUP);
  pinMode(DATA_PIN, OUTPUT);
  Serial.begin(19200);
  attachInterrupt(digitalPinToInterrupt(ZC_PIN), on_zc, RISING);
}

void loop() {
  if(Serial.available()) {
    uint8_t new_brightness = Serial.read();
    if(brightness == 0 and new_brightness > 0) {
      fullCyclesLeft = WARM_UP_CYCLES;

      // A small hack to properly deal with warmup cycles.
      brightness = 1;
    }
    targetBrightness = new_brightness;
  }

  switch(current_state) {
    case WAITING_FOR_ZC:
    return;
    break;
    
    case WAITING_FOR_TIME:
    if(micros()>=startPulseAt) {
      digitalWrite(DATA_PIN, HIGH);
      current_state = HIGH_SIGNAL;
    }
    break;
    
    case HIGH_SIGNAL:
    if(micros()>=stopPulseAt) {
      digitalWrite(DATA_PIN, LOW);
      current_state = WAITING_FOR_ZC;
    }
    break;
  }
}
