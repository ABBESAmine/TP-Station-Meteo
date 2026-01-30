/*
 * Station météo connectée - ESP32
 * Parcours A - Broker: captain.dev0.pandor.cloud:1884
 */

#define SIMULATION true  // false pour DHT22 réel

// Pins (à adapter selon votre câblage)
#define PIN_DHT     4
#define PIN_BUTTON  0
#define PIN_LED_C   2
#define PIN_LED_F   15

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_C, OUTPUT);
  pinMode(PIN_LED_F, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  // TODO: WiFi, MQTT, lecture capteur/simulation, debounce bouton, LEDs
  delay(100);
}
