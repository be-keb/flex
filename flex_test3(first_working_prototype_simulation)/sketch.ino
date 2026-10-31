#include <Adafruit_NeoPixel.h>

// Wokwi FLIP prototype: closer hand = brighter cyan light.
// GPIO assignments match diagram.json. On real hardware, protect Echo
// with a voltage divider before connecting it to the ESP32.
constexpr int TRIG_PIN = 5;
constexpr int ECHO_PIN = 18;
constexpr int LED_PIN = 4;
constexpr int NUM_LEDS = 8;
constexpr int MIN_BRIGHTNESS = 12;
constexpr int MAX_BRIGHTNESS = 90; // Capped for safer first physical tests.
constexpr float NEAR_CM = 5.0;
constexpr float FAR_CM = 60.0;

Adafruit_NeoPixel lamp(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
int currentBrightness = MIN_BRIGHTNESS;
unsigned long lastGoodReadingMs = 0;
float filteredDistance = FAR_CM;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  lamp.begin();
  lamp.clear();
  lamp.show();
  lastGoodReadingMs = millis();
}

void loop() {
  // Trigger one ultrasonic measurement.
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // About 25 ms timeout corresponds to roughly 4 m.
  const unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);
  int targetBrightness = MIN_BRIGHTNESS;

  if (duration > 0) {
    const float distanceCm = duration * 0.0343f / 2.0f;
    filteredDistance = 0.70f * filteredDistance + 0.30f * distanceCm;
    lastGoodReadingMs = millis();

    const float boundedDistance = constrain(filteredDistance, NEAR_CM, FAR_CM);
    const float fraction = (FAR_CM - boundedDistance) / (FAR_CM - NEAR_CM);
    targetBrightness = MIN_BRIGHTNESS + (int)(fraction * (MAX_BRIGHTNESS - MIN_BRIGHTNESS));

    Serial.print("Distance: ");
    Serial.print(distanceCm, 1);
    Serial.print(" cm | brightness target: ");
    Serial.println(targetBrightness);
  } else {
    // Ignore brief missed echoes; fade to the dim glow after 700 ms.
    if (millis() - lastGoodReadingMs < 700UL) {
      targetBrightness = currentBrightness;
    }
    Serial.println("No echo; holding briefly, then returning to ambient glow.");
  }

  // Gradual brightness changes avoid flicker.
  if (currentBrightness < targetBrightness) currentBrightness = min(currentBrightness + 2, targetBrightness);
  if (currentBrightness > targetBrightness) currentBrightness = max(currentBrightness - 2, targetBrightness);

  lamp.setBrightness(currentBrightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    lamp.setPixelColor(i, 0, 150, 220); // cyan-blue
  }
  lamp.show();
  delay(35);
}
