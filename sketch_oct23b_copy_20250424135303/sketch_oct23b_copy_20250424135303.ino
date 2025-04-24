#include <Servo.h>
#include <Keypad.h>

#define BUZZER_PIN 12

Servo myServo;
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

enum Mode {
  WAITING, TIME_INPUT, TIME_CONFIRM,
  FAST_SETUP, FAST_CONFIRM,
  SLOW_SETUP, SLOW_CONFIRM,
  FAST_RUNNING, SLOW_RUNNING, PAUSED
};
Mode currentMode = WAITING;

String delayBuffer = "";
int customDelay = 0;
bool delayReady = false;

String timeBuffer = "";
unsigned long totalRunTime = 0;
bool timeReady = false;
unsigned long loopStartTime = 0;
bool isRunning = false;

void setup() {
  myServo.attach(10);
  myServo.write(0);
  Serial.begin(9600);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    beep(1);  // Beep on key press

    if (key == '*') {
      resetAll();
      Serial.println("System reset to neutral.");
      beep(3); // Confirm reset
    }
    else if (key == 'D') {
      if (currentMode == PAUSED) {
        currentMode = isRunning ? FAST_RUNNING : SLOW_RUNNING;
        Serial.println("Resumed.");
        beep(3); // Confirm resume
      } else if (currentMode == FAST_RUNNING || currentMode == SLOW_RUNNING) {
        currentMode = PAUSED;
        Serial.println("Paused.");
        beep(2); // Confirm pause
      }
    }
    else if (key == 'C') {
      if (currentMode == WAITING || currentMode == TIME_INPUT) {
        if (currentMode == WAITING) {
          timeBuffer = "";
          currentMode = TIME_INPUT;
          Serial.println("Enter 3-digit runtime in seconds:");
          beep(2);  // Beep to prompt for time
        } else if (timeBuffer.length() == 3) {
          totalRunTime = timeBuffer.toInt() * 1000UL;
          currentMode = TIME_CONFIRM;
          Serial.print("Confirm runtime: ");
          Serial.print(totalRunTime / 1000);
          Serial.println("s. Press C to confirm.");
          beep(2);  // Beep for confirmation prompt
        }
      }
      else if (currentMode == TIME_CONFIRM) {
        timeReady = true;
        currentMode = WAITING;
        Serial.println("Runtime confirmed. Press A or B to set delay.");
        beep(3);  // Confirm time confirmation
      }
    }
    else if (key == 'A' && timeReady) {
      handleDelayInput(key, FAST_SETUP, FAST_CONFIRM, FAST_RUNNING);
      beep(2);  // Beep for delay input prompt
    }
    else if (key == 'B' && timeReady) {
      handleDelayInput(key, SLOW_SETUP, SLOW_CONFIRM, SLOW_RUNNING);
      beep(2);  // Beep for delay input prompt
    }
    else if (isdigit(key)) {
      if (currentMode == TIME_INPUT) {
        timeBuffer += key;
        Serial.print("Keyed: "); Serial.println(key);
        if (timeBuffer.length() == 3) {
          Serial.println("Press C to confirm runtime.");
        }
      }
      else if (currentMode == FAST_SETUP || currentMode == SLOW_SETUP) {
        delayBuffer += key;
        Serial.print("Keyed: "); Serial.println(key);
        if (delayBuffer.length() == 3) {
          Serial.println("Press A or B to confirm delay.");
        }
      }
    }
  }

  if ((currentMode == FAST_RUNNING || currentMode == SLOW_RUNNING) && timeReady) {
    if (millis() - loopStartTime >= totalRunTime) {
      Serial.println("Runtime complete. Stopping.");
      resetAll();
    } else {
      performDip(customDelay, currentMode == SLOW_RUNNING);
    }
  }
}

void handleDelayInput(char key, Mode inputMode, Mode confirmMode, Mode runMode) {
  if (currentMode == WAITING) {
    delayBuffer = "";
    currentMode = inputMode;
    Serial.println((key == 'A') ? "Enter 3-digit delay for FAST:" : "Enter 3-digit delay for SLOW:");
  }
  else if (currentMode == inputMode && delayBuffer.length() == 3) {
    customDelay = delayBuffer.toInt();
    currentMode = confirmMode;
    Serial.print("Confirm delay: ");
    Serial.print(customDelay);
    Serial.println("ms. Press A or B to confirm.");
  }
  else if (currentMode == confirmMode) {
    delayReady = true;
    currentMode = runMode;
    loopStartTime = millis();
    isRunning = (runMode == FAST_RUNNING);
    Serial.println("Delay confirmed. Loop started.");
    beep(3); // Confirm delay confirmation
  }
}

void performDip(int delayTime, bool slowRise) {
  myServo.write(180); // dip down
  delay(delayTime);
  if (slowRise) {
    for (int pos = 180; pos >= 0; pos--) {
      myServo.write(pos);
      delay(10); // slow rise
    }
  } else {
    myServo.write(0); // fast up
  }
  delay(delayTime);
}

void resetAll() {
  currentMode = WAITING;
  delayBuffer = "";
  customDelay = 0;
  delayReady = false;
  timeBuffer = "";
  totalRunTime = 0;
  timeReady = false;
  isRunning = false;
  loopStartTime = 0;
  myServo.write(0);
  beep(3);  // Confirm reset
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);  // Beep duration
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);  // Interval between beeps
  }
}
