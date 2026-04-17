#define RAIN_PIN D2

#define IN1 D5
#define IN2 D6
#define ENA D7

#define RAIN     LOW
#define NO_RAIN  HIGH

int lastRainState = NO_RAIN;          // assume no rain at start
const unsigned long RUN_TIME = 1000;  // 1 second motor run
const int MOTOR_SPEED = 500;          // MID speed (0–1023)

void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  Serial.println("Motor stopped");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Automatic Roof System Started");

  pinMode(RAIN_PIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // ESP8266 PWM setup
  analogWriteRange(1023);
  analogWriteFreq(1000);

  stopMotor();
}

void loop() {
  int currentRainState = digitalRead(RAIN_PIN);

  // 🌧 Rain just detected → CLOSE roof
  if (currentRainState == RAIN && lastRainState == NO_RAIN) {
    Serial.println("Rain detected → Closing roof");

    analogWrite(ENA, MOTOR_SPEED);   // enable motor (mid speed)
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);         // CLOSE direction

    delay(RUN_TIME);
    stopMotor();

    Serial.println("Roof closed");
  }

  // ☀️ Rain just stopped → OPEN roof
  if (currentRainState == NO_RAIN && lastRainState == RAIN) {
    Serial.println("No rain → Opening roof");

    analogWrite(ENA, MOTOR_SPEED);   // enable motor (mid speed)
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);          // OPEN direction

    delay(RUN_TIME);
    stopMotor();

    Serial.println("Roof opened");
  }

  // Save rain state
  lastRainState = currentRainState;
}
