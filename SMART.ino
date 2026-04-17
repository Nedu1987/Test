#define BLYNK_TEMPLATE_ID "TMPL3yvGdv-H4"
#define BLYNK_TEMPLATE_NAME "smart ir"
#define BLYNK_AUTH_TOKEN "foF-R_bu7qtP4-pCs0e14NqV5pfl4qpZ"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// WiFi credentials
char ssid[] = "lap";          // 🔹 Your WiFi name
char pass[] = "12345678";     // 🔹 Your WiFi password

// Pin definitions
#define DHTPIN D2
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0
#define FLAME_SENSOR_PIN D5
#define RAIN_SENSOR_PIN D6
#define RELAY_PIN D7
#define BUZZER_PIN D1

DHT dht(DHTPIN, DHTTYPE);

bool manualControl = false;
bool pumpState = false;
bool fireNotified = false;

BlynkTimer timer;

// Blynk virtual pins:
// V0 - Soil moisture (0–1023)
// V1 - Temperature
// V2 - Humidity
// V3 - Rain status
// V5 - Manual pump control switch
// V6 - Pump status string

void controlPump(int soilRaw, int rain);

// 🔹 Manual control from Blynk
BLYNK_WRITE(V5) {
  manualControl = param.asInt();
}

// 🔹 Read sensors and send data to Blynk
void readSensors() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int soilRaw = analogRead(SOIL_MOISTURE_PIN); // 0 (wet) to 1023 (dry)
  int flame = digitalRead(FLAME_SENSOR_PIN);
  int rain = digitalRead(RAIN_SENSOR_PIN);     // LOW = rain detected

  // Send sensor data to Blynk
  Blynk.virtualWrite(V0, soilRaw);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, (rain == LOW) ? "Rain Detected" : "No Rain");

  // 🔥 Fire detection
  if (flame == LOW) { // Active LOW
    digitalWrite(BUZZER_PIN, HIGH);
    if (!fireNotified) {
      Blynk.logEvent("fire_detected", "🔥 Fire Detected! Take immediate action!");
      fireNotified = true;
      Serial.println("🔥 Fire Alert Sent and Buzzer ON");
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    fireNotified = false;
  }

  // 💧 Pump control
  controlPump(soilRaw, rain);
}

// 🔹 Control pump logic
void controlPump(int soilRaw, int rain) {
  const int threshold = 800;  // Adjust as needed (1023 = very dry)

  // Rain override: Pump always OFF when raining
  if (rain == LOW) {
    pumpState = false;
    digitalWrite(RELAY_PIN, HIGH); // OFF (relay is active LOW)
    Blynk.virtualWrite(V6, "Pump OFF (Rain Detected)");
    return;
  }

  // Manual mode
  if (manualControl) {
    pumpState = true;
    digitalWrite(RELAY_PIN, LOW); // ON
    Blynk.virtualWrite(V6, "Pump ON (Manual)");
  } 
  // Auto mode
  else {
    if (soilRaw < threshold) {
      pumpState = true;
      digitalWrite(RELAY_PIN, LOW); // ON
      Blynk.virtualWrite(V6, "Pump ON (Auto)");
    } else {
      pumpState = false;
      
      digitalWrite(RELAY_PIN, HIGH); // OFF
      Blynk.virtualWrite(V6, "Pump OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH); // Pump OFF
  digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF

  timer.setInterval(1000L, readSensors); // Read every 1 second
}

void loop() {
  Blynk.run();
  timer.run();
}
