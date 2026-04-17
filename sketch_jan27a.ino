#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL3dN_FNfMK"
#define BLYNK_TEMPLATE_NAME "SMART CITY"
#define BLYNK_AUTH_TOKEN "cyZ6jAT6v_tTRlSaRnjhISRkvV3BstGN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// ---------------- WiFi ----------------
char ssid[] = "lap";
char pass[] = "12345678";

// ---------------- Pins ----------------
// Sensors
#define SOIL_PIN          34
#define TURB_PIN          35
#define ROAD_PIN          33

// Relays (LOW-level)
#define RELAY_PUMP        26
#define RELAY_HEATER      32

// Alerts
#define BUZZER_PIN        25

// LEDs
#define LED_PUMP          18   // Green
#define LED_ALERT         19   // Red
#define LED_HEATER        23   // Blue

// DHT Sensors
#define DHT_WEATHER_PIN   14
#define DHT_HEATER_PIN    27
#define DHTTYPE DHT11

DHT dhtWeather(DHT_WEATHER_PIN, DHTTYPE);
DHT dhtHeater(DHT_HEATER_PIN, DHTTYPE);

BlynkTimer timer;

// ---------------- Thresholds ----------
int soil_th   = 30;   // %
int turb_th   = 40;   // %
int cold_temp = 15;   // °C

// ---------------- States --------------
bool pumpState   = false;
bool heaterState = false;
int  last_soil   = 0;

bool dirtyAlertSent = false;
bool roadAlertSent  = false;

// ---------------- AI Prediction -------
int predictNextWatering(int now, int last) {
  int rate = last - now;
  if (rate <= 0) return 0;
  int hrs = (now - soil_th) / rate;
  return hrs < 0 ? 0 : hrs;
}

// ---------------- Relay Control -------
void setPump(bool state) {
  pumpState = state;
  digitalWrite(RELAY_PUMP, state ? LOW : HIGH); // LOW-level relay
  digitalWrite(LED_PUMP, state);
}

// ---------------- MAIN SYSTEM ---------
void systemUpdate() {

  // Soil (ADC up to 1023 handled)
  int soil_raw = analogRead(SOIL_PIN);
  int soil = map(soil_raw, 4095, 0, 0, 100);

  // Turbidity
  int turb_raw = analogRead(TURB_PIN);
  int turb = map(turb_raw, 4095, 0, 0, 100);

  // DHT Weather
  float tempWeather = dhtWeather.readTemperature();
  float humWeather  = dhtWeather.readHumidity();

  // DHT Heater
  float tempHeater = dhtHeater.readTemperature();

  // Road sensor
  int road = digitalRead(ROAD_PIN);

  if (isnan(tempWeather)) tempWeather = -1;
  if (isnan(humWeather))  humWeather  = -1;
  if (isnan(tempHeater))  tempHeater  = -1;

  // AI prediction
  int hours = predictNextWatering(soil, last_soil);
  last_soil = soil;

  // -------- AUTO IRRIGATION ----------
  if (soil < soil_th && turb > turb_th)
    setPump(true);
  else
    setPump(false);

  // -------- DIRTY WATER ALERT --------
  if (turb <= turb_th) {
    setPump(false);
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_ALERT, HIGH);

    if (!dirtyAlertSent) {
      Blynk.logEvent("dirty_water", "Dirty water detected! Pump stopped.");
      dirtyAlertSent = true;
    }
  } else {
    dirtyAlertSent = false;
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_ALERT, LOW);
  }

  // -------- ROAD ALERT ---------------
  if (road == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_ALERT, HIGH);

    if (!roadAlertSent) {
      Blynk.logEvent("road_alert", "Road flooded / blocked!");
      roadAlertSent = true;
    }
  } else {
    roadAlertSent = false;
  }

  // -------- PIPE HEATER --------------
  heaterState = (tempHeater != -1 && tempHeater <= cold_temp);
  digitalWrite(RELAY_HEATER, heaterState ? LOW : HIGH);
  digitalWrite(LED_HEATER, heaterState);

  // -------- BLYNK OUTPUT -------------
  Blynk.virtualWrite(V0, soil);         // Soil gauge
  Blynk.virtualWrite(V1, turb);         // Turbidity gauge
  Blynk.virtualWrite(V2, pumpState);    // Pump LED
  Blynk.virtualWrite(V3, tempWeather);  // Temp gauge
  Blynk.virtualWrite(V4, humWeather);   // Humidity gauge
  Blynk.virtualWrite(V5, tempHeater);   // Heater temp gauge
  Blynk.virtualWrite(V6, heaterState);  // Heater LED
  Blynk.virtualWrite(V7, hours);        // AI label

  // -------- SERIAL MONITOR -----------
  Serial.println("----- SYSTEM STATUS -----");
  Serial.print("Soil: "); Serial.println(soil);
  Serial.print("Turb: "); Serial.println(turb);
  Serial.print("Temp: "); Serial.println(tempWeather);
  Serial.print("Hum : "); Serial.println(humWeather);
  Serial.print("Heater Temp: "); Serial.println(tempHeater);
  Serial.print("Pump: "); Serial.println(pumpState);
  Serial.print("Heater: "); Serial.println(heaterState);
  Serial.print("Next Water (AI): "); Serial.print(hours); Serial.println(" hrs");
  Serial.println("-------------------------");
}

// ---------------- SETUP --------------
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_HEATER, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ROAD_PIN, INPUT);

  pinMode(LED_PUMP, OUTPUT);
  pinMode(LED_ALERT, OUTPUT);
  pinMode(LED_HEATER, OUTPUT);

  digitalWrite(RELAY_PUMP, HIGH);    // OFF
  digitalWrite(RELAY_HEATER, HIGH);  // OFF
  digitalWrite(BUZZER_PIN, LOW);

  dhtWeather.begin();
  dhtHeater.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(2000L, systemUpdate);
}

// ---------------- LOOP ---------------
void loop() {
  Blynk.run();
  timer.run();
}
