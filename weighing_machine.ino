#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// I2C pins for OLED
#define OLED_SDA 18
#define OLED_SCL 19

// HX711 pins
#define DT 21  // Data pin
#define SCK 22 // Clock pin

// Relay pin
#define RELAY_PIN 23

// HX711 object
HX711 scale;

// Calibration factor (adjust based on your calibration)
float calibrationFactor = 3310;

// Unit selection
enum Unit { KG, G, LB };
Unit currentUnit = KG;

// Peak load tracking
float peakLoad = 0.0;
float currentWeight = 0.0;

// Timer variables
unsigned long startTime = 0;
unsigned long stopTime = 0;
bool isSystemActive = false;

// Buttons to simulate user input (replace with actual button reads in final code)
bool startButton = false;
bool stopButton = false;
bool tareButton = false;
bool zeroButton = false;
bool unitKgButton = false;
bool unitGButton = false;
bool unitLbButton = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize HX711
  scale.begin(DT, SCK);
  scale.set_scale(calibrationFactor);

  // Initialize relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially

  // Tare the scale
  Serial.println("Taring the scale...");
  display.println("Taring the scale...");
  display.display();
  delay(3000);
  scale.tare();
  display.clearDisplay();
  display.println("Scale is ready.");
  display.display();
  delay(2000); // Allow time for the user to see
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    // Handle commands from Serial Monitor
    if (command.equalsIgnoreCase("START") && !isSystemActive) {
      startSystem();
    }

    if (command.equalsIgnoreCase("STOP") && isSystemActive) {
      stopSystem();
    }

    if (command.equalsIgnoreCase("TARE")) {
      tareSystem();
    }

    if (command.equalsIgnoreCase("ZERO")) {
      zeroSystem();
    }

    if (command.equalsIgnoreCase("UNIT_KG")) {
      changeUnit(KG);
    }

    if (command.equalsIgnoreCase("UNIT_G")) {
      changeUnit(G);
    }

    if (command.equalsIgnoreCase("UNIT_LB")) {
      changeUnit(LB);
    }
  }

  // If the system is active, display the weight and elapsed time
  if (isSystemActive) {
    // Check if the scale is ready
    if (scale.is_ready()) {
      currentWeight = scale.get_units(10); // Average of 10 readings
      peakLoad = max(peakLoad, currentWeight); // Update peak load if needed

      // Convert to selected unit
      float displayWeight = currentWeight;
      if (currentUnit == G) {
        displayWeight *= 1000;  // Convert kg to grams
      } else if (currentUnit == LB) {
        displayWeight *= 2.20462;  // Convert kg to pounds
      }

      // Timer functionality
      unsigned long elapsedTime = millis() - startTime;
      unsigned long seconds = elapsedTime / 1000;
      unsigned long minutes = seconds / 60;
      seconds = seconds % 60;

      // Display weight, time, and peak load
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Weight:");
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.print(displayWeight, 2);
      display.print(currentUnit == KG ? " kg" : currentUnit == G ? " g" : " lbs");

      // Display time
      display.setTextSize(1);
      display.setCursor(0, 50);
      display.print("Time: ");
      display.print(minutes);
      display.print("m ");
      display.print(seconds);
      display.println("s");

      // Display peak load
      display.setCursor(0, 40);
      display.print("Peak Load: ");
      display.print(peakLoad, 2);
      display.println(currentUnit == KG ? " kg" : currentUnit == G ? " g" : " lbs");

      display.display();

      // Log weight and time to Serial Monitor
      Serial.print("Weight: ");
      Serial.print(displayWeight, 2);
      Serial.print(" kg, Time: ");
      Serial.print(minutes);
      Serial.print("m ");
      Serial.print(seconds);
      Serial.println("s");
    } else {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Scale not ready.");
      display.display();
      Serial.println("Scale not ready. Check connections.");
    }
  }

  delay(500); // Small delay to avoid flooding the OLED
}

// Start system function
void startSystem() {
  Serial.println("START command received. System starting...");

  // Show a 2-second countdown animation
  for (int i = 2; i > 0; i--) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Starting in...");
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(i);
    display.display();
    delay(1000);
  }

  // Activate relay
  Serial.println("Turning on relay...");
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Relay turned on.");

  // Set the system as active
  isSystemActive = true;
  startTime = millis(); // Start the timer
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Started");
  display.display();
  delay(2000);  // Wait for 2 seconds before starting readings
  display.clearDisplay();
  display.println("Now measuring...");
  display.display();
}

// Stop system function
void stopSystem() {
  // Stop timer and calculate overall time
  stopTime = millis();
  unsigned long totalTime = stopTime - startTime;
  unsigned long totalSeconds = totalTime / 1000;
  unsigned long totalMinutes = totalSeconds / 60;
  totalSeconds = totalSeconds % 60;

  Serial.println("STOP command received. Shutting down system...");

  // Deactivate relay
  Serial.println("Turning off relay...");
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Relay turned off.");

  // Stop scale operations
  isSystemActive = false;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Stopped");
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Total Time: ");
  display.print(totalMinutes);
  display.print("m ");
  display.print(totalSeconds);
  display.println("s");

  // Display peak load
  display.setCursor(0, 40);
  display.print("Peak Load: ");
  display.print(peakLoad, 2);
  display.println(currentUnit == KG ? " kg" : currentUnit == G ? " g" : " lbs");

  display.display();

  // Log the total time and peak load to Serial Monitor
  Serial.print("Total Time: ");
  Serial.print(totalMinutes);
  Serial.print("m ");
  Serial.print(totalSeconds);
  Serial.println("s");
  Serial.print("Peak Load: ");
  Serial.print(peakLoad, 2);
  Serial.println(currentUnit == KG ? " kg" : currentUnit == G ? " g" : " lbs");

  delay(2000); // Wait for a moment before resetting the system

  // Simulate system shutdown (halt further execution)
  while (true) {
    // Infinite loop to simulate shutdown
  }
}

// Tare system function
void tareSystem() {
  Serial.println("Taring the system...");
  scale.tare();  // Reset the scale to zero
  peakLoad = 0;  // Reset the peak load
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Taring complete");
  display.display();
  delay(1000); // Allow the user to see
}

// Zero system function
void zeroSystem() {
  Serial.println("Zeroing the system...");
  scale.set_scale(calibrationFactor);  // Reset scale to calibration factor
  peakLoad = 0;  // Reset peak load
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Zeroed");
  display.display();
  delay(1000); // Allow the user to see
}

// Change unit function
void changeUnit(Unit newUnit) {
  if (currentUnit != newUnit) {
    currentUnit = newUnit;
    Serial.print("Unit changed to: ");
    if (currentUnit == KG) {
      Serial.println("kg");
    } else if (currentUnit == G) {
      Serial.println("g");
    } else if (currentUnit == LB) {
      Serial.println("lbs");
    }
  }
}