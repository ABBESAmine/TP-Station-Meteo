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

/* ========== GPIO ========== */
#define PINBUTTON 18   // Bouton
#define LED_C 26       // LED Celsius
#define LED_F 27       // LED Fahrenheit

/* ========== ETAT ========== */
bool isCelsius = true;        // unité actuelle

// Variables pour debounce fiable
int lastReading = HIGH;       // dernière lecture brute du bouton
int stableState = HIGH;       // état stable après debounce
unsigned long lastChangeTime = 0;
const unsigned long debounceMs = 40;  // délai de stabilisation

unsigned long lastPrint = 0;

/* ========== DONNEES SIMULEES ========== */
float fakeTemperature() {
  return random(180, 300) / 10.0; // 18.0 à 30.0
}

float fakeHumidity() {
  return random(300, 700) / 10.0; // 30% à 70%
}

/* ========== LED ========== */
void updateLEDs() {
  // Une seule LED allumée selon l’unité
  digitalWrite(LED_C, isCelsius ? HIGH : LOW);
  digitalWrite(LED_F, isCelsius ? LOW : HIGH);
}

/* ========== SETUP ========== */
void setup() {
  Serial.begin(115200);
  Serial.println("=== MODE SIMULATION DEMARRE ===");

  pinMode(PINBUTTON, INPUT_PULLUP); // bouton vers GND
  pinMode(LED_C, OUTPUT);
  pinMode(LED_F, OUTPUT);

  // LED initiale
  isCelsius = true;
  updateLEDs();
}

/* ========== LOOP PRINCIPALE ========== */
void loop() {
  // Lecture du bouton
  int reading = digitalRead(PINBUTTON);

  // Détection de changement brut
  if (reading != lastReading) {
    lastChangeTime = millis();
    lastReading = reading;
  }

  // Validation après stabilité
  if (millis() - lastChangeTime > debounceMs) {
    if (stableState != reading) {
      stableState = reading;

      // Toggle uniquement si bouton pressé (LOW)
      if (stableState == LOW) {
        isCelsius = !isCelsius;
        updateLEDs();

        // Debug Serial
        Serial.print("Button pressed! LED_C=");
        Serial.print(isCelsius ? "ON" : "OFF");
        Serial.print(", LED_F=");
        Serial.println(!isCelsius ? "ON" : "OFF");
      }
    }
  }

  /* ---- AFFICHAGE DES DONNEES SIMULEES ---- */
  if (millis() - lastPrint > 3000) {
    float temperature = fakeTemperature();
    float humidity = fakeHumidity();

    if (!isCelsius) temperature = temperature * 9 / 5 + 32;

    Serial.print("Température : ");
    Serial.print(temperature);
    Serial.print(isCelsius ? " °C" : " °F");
    Serial.print(" | Humidité : ");
    Serial.print(humidity);
    Serial.println(" %");

    lastPrint = millis();
  }
}
