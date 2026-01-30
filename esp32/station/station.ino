/***************************************************
 * TP STATION METEO CONNECTEE - ESP32
 * STEP 2 : MODE SIMULATION UNIQUEMENT
 *
 * Ce code NE FAIT QUE :
 * - Simuler température + humidité
 * - Gérer un bouton avec anti-rebond fiable
 * - Allumer une LED selon l’unité
 * - Afficher les valeurs en local (Serial Monitor)
 *
 *
 *
 *
 ***************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* ========== GPIO ========== */
#define PINBUTTON 18
#define LED_C 26
#define LED_F 27

/* ========== WIFI ========== */
const char* ssid = "Hamahoullah";
const char* password = "foumalade";

/* ========== MQTT ========== */
const char* mqtt_server = "captain.dev0.pandor.cloud/goat/data";
const int mqtt_port = 1884;


const char* topic_temp = "station/meteo/temperature";
 

WiFiClient espClient;
PubSubClient client(espClient);

/* ========== ETAT ========== */
bool isCelsius = true;

int lastReading = HIGH;
int stableState = HIGH;
unsigned long lastChangeTime = 0;
const unsigned long debounceMs = 40;

unsigned long lastPrint = 0;

/* ========== DONNEES SIMULEES ========== */
float fakeTemperature() {
  return random(180, 300) / 10.0;
}

float fakeHumidity() {
  return random(300, 700) / 10.0;
}

/* ========== LED ========== */
void updateLEDs() {
  digitalWrite(LED_C, isCelsius ? HIGH : LOW);
  digitalWrite(LED_F, isCelsius ? LOW : HIGH);
}

/* ========== MQTT CALLBACK (BONUS) ========== */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(msg);

  if (String(topic) == topic_unit) {
    if (msg == "C") isCelsius = true;
    if (msg == "F") isCelsius = false;
    updateLEDs();
  }
}

/* ========== WIFI ========== */
void setupWiFi() {
  Serial.print("Connexion WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté !");
}

/* ========== MQTT ========== */
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect("ESP32_Meteo")) {
      Serial.println("connecté !");
      client.subscribe(topic_unit); // bonus
    } else {
      Serial.print("échec, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

/* ========== SETUP ========== */
void setup() {
  Serial.begin(115200);
  Serial.println("=== MODE SIMULATION + MQTT ===");

  pinMode(PINBUTTON, INPUT_PULLUP);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_F, OUTPUT);

  updateLEDs();

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

/* ========== LOOP ========== */
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // --- Gestion bouton ---
  int reading = digitalRead(PINBUTTON);
  if (reading != lastReading) {
    lastChangeTime = millis();
    lastReading = reading;
  }

  if (millis() - lastChangeTime > debounceMs) {
    if (stableState != reading) {
      stableState = reading;
      if (stableState == LOW) {
        isCelsius = !isCelsius;
        updateLEDs();
      }
    }
  }

  // --- Publication MQTT ---
  if (millis() - lastPrint > 3000) {
    float temperature = fakeTemperature();
    float humidity = fakeHumidity();

    if (!isCelsius) temperature = temperature * 9 / 5 + 32;

    char tempStr[10];
    char humStr[10];
    dtostrf(temperature, 4, 1, tempStr);
    dtostrf(humidity, 4, 1, humStr);

    client.publish(topic_temp, tempStr);
    client.publish(topic_hum, humStr);

    Serial.print("MQTT -> Temp: ");
    Serial.print(tempStr);
    Serial.print(isCelsius ? " °C" : " °F");
    Serial.print(" | Hum: ");
    Serial.println(humStr);

    lastPrint = millis();
  }
}
