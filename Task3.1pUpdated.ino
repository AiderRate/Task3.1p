#include <WiFiNINA.h> // Include WiFiNINA library for WiFi communication
#include <Wire.h> // Include Wire library for I2C communication
#include "secrets.h" // Include secrets.h file for WiFi credentials

char ssid[] = SECRET_SSID; // WiFi network name
char pass[] = SECRET_PASS; // WiFi password

WiFiClient client; // Create a WiFi client object
char HOST_NAME[] = "maker.ifttt.com"; // IFTTT host name
String YOUR_IFTTT_KEY = "cdOepeE3dUYeDmMWcKA8Tn"; // Your IFTTT key

const int BH1750_address = 0x23; // I2C address of the BH1750 sensor

void setup() {
  Serial.begin(9600); // Start serial communication
  Wire.begin(); // Initialize I2C for the BH1750 sensor
  WiFi.begin(ssid, pass); // Connect to WiFi

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { // Wait until WiFi connection is established
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize BH1750 sensor
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x01); // Power on
  Wire.endTransmission();
  delay(10); // Allow time for sensor to power up
  
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x10); // Continuously measure in high resolution mode
  Wire.endTransmission();
  delay(10); // Allow time for mode setup
}

void loop() {
  int lightLevel = readLightSensor(); // Get the current light level from the sensor
  
  if (lightLevel == -1) { // Check for error reading the sensor
    Serial.println("Error reading light sensor");
    return; // Skip this loop iteration if an error occurs
  }

  if (lightLevel > 300) { // If light level is above threshold, trigger IFTTT event for sunlight exposure start
    triggerIFTTT("sunlight_exposure_start", lightLevel);
  } else if (lightLevel < 30) { // If light level is below threshold, trigger IFTTT event for sunlight exposure stop
    triggerIFTTT("sunlight_exposure_stop", lightLevel);
  }

  delay(3000); // Wait for 3 seconds before next reading
}

int readLightSensor() {
  Wire.beginTransmission(BH1750_address);
  Wire.requestFrom(BH1750_address, 2);
  if (Wire.available() == 2) {
    int reading = (Wire.read() << 8) | Wire.read(); // Combine two bytes into an integer
    return reading / 1.2; // Convert to lux
  }
  return -1; // Error reading sensor
}

void triggerIFTTT(String eventName, int lightLevel) {
  if (client.connect(HOST_NAME, 80)) { // Connect to IFTTT server
    String queryString = "/trigger/" + eventName + "/with/key/" + YOUR_IFTTT_KEY + "?value1=" + String(lightLevel);
    // Construct the query string for the webhook
    client.println("GET " + queryString + " HTTP/1.1"); // Send HTTP GET request
    client.println("Host: " + String(HOST_NAME)); // Specify the host
    client.println("Connection: close"); // Close connection after sending request
    client.println(); // End HTTP header
    
    while (client.connected() || client.available()) { // Read response from server
      char c = client.read();
      Serial.write(c);
    }
    client.stop(); // Disconnect after sending the request
    Serial.println("IFTTT event '" + eventName + "' triggered with light level: " + String(lightLevel)); // Print message
  } else {
    Serial.println("Failed to connect to IFTTT"); // Print error message
  }
}





