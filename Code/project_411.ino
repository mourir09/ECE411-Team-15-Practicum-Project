#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// HC-SR04 sensor pins
const int trigPin = 14;
const int echoPin = 32;

// LED pins
const int redLED = 15;
const int yellowLED = 27;
const int blueLED = 33;

float lastDistanceCm = 0;
bool hasLast = false;

enum Zone {ZONE_FAR, ZONE_MEDIUM, ZONE_NEAR};
Zone currentZone = ZONE_FAR;

// count how many loops in a row we see NO echo
int noEchoCount = 0;          // number of consecutive loops with duration == 0
const int NO_ECHO_LIMIT = 10; // ~10 * 200ms = ~2 seconds

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  // Trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Echo with timeout
  unsigned long duration = pulseIn(echoPin, HIGH, 30000); // 30ms max

  // track consecutive "no echo" readings
  if (duration == 0) {
    noEchoCount++;         // no echo this loop
  } else {
    noEchoCount = 0;       // got a valid echo, reset counter
  }

  float distance_cm = duration * 0.034 / 2;

  // Ignore invalid readings (your original behavior)
  if (duration == 0 || distance_cm < 2 || distance_cm > 400) {
    distance_cm = lastDistanceCm; // keep old value
  }

  // Smooth the distance (your original smoothing)
  if (!hasLast) {
    lastDistanceCm = distance_cm;
    hasLast = true;
  } else {
    lastDistanceCm = 0.7 * lastDistanceCm + 0.3 * distance_cm;
  }

  // Print to Serial
  Serial.print("Distance: ");
  Serial.print(lastDistanceCm);
  Serial.println(" cm");

  // ==== CLEANER OLED DISPLAY ====
  display.clearDisplay();
  display.setCursor(0, 10);

  if (noEchoCount >= NO_ECHO_LIMIT) {
    // No object detected for a while: show a clean "no object" indicator
    display.print("----");      // or: display.print("No obj");
  } else {
    // Normal distance display
    display.print(lastDistanceCm);
    display.print(" cm");
  }
  display.display();
  // ==============================

  // Hysteresis for LED zones (unchanged)
  // Enter near if < 95, leave near if > 105
  // Enter medium if < 145 and > 105, leave medium if < 95 or > 155
  // Far otherwise
  if (currentZone == ZONE_NEAR) {
    if (lastDistanceCm > 85) currentZone = ZONE_MEDIUM;
  } else if (currentZone == ZONE_MEDIUM) {
    if (lastDistanceCm < 75) currentZone = ZONE_NEAR;
    else if (lastDistanceCm > 165) currentZone = ZONE_FAR;
  } else { // ZONE_FAR
    if (lastDistanceCm < 155) currentZone = ZONE_MEDIUM;
  }

  // If we haven't seen any echo for ~2 seconds, force FAR (blue)
  if (noEchoCount >= NO_ECHO_LIMIT) {
    currentZone = ZONE_FAR;
  }

  // LED control based on the zone (unchanged)
  if (currentZone == ZONE_NEAR) {
    digitalWrite(redLED, HIGH);
    digitalWrite(yellowLED, LOW);
    digitalWrite(blueLED, LOW);
  } else if (currentZone == ZONE_MEDIUM) {
    digitalWrite(redLED, LOW);
    digitalWrite(yellowLED, HIGH);
    digitalWrite(blueLED, LOW);
  } else {
    digitalWrite(redLED, LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(blueLED, HIGH);
  }

  delay(200);
}
