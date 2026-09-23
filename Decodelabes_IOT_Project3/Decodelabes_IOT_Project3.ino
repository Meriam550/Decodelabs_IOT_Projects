/**
 * ============================================================================
 * Project 3: Cloud-Connected Security Node (IoT Telemetry)
 * Hardware: Arduino Uno + HC-SR04 Ultrasonic Sensor + ESP8266 Wi-Fi Module
 * Cloud Platform: ThingSpeak (HTTP REST API / Telemetry)
 * ============================================================================
 */

#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const uint8_t PIN_TRIG = 9;       // HC-SR04 Trigger Pin
const uint8_t PIN_ECHO = 8;       // HC-SR04 Echo Pin
const uint8_t PIN_ESP_RX = 2;     // Arduino Pin 2 connects to ESP8266 TX
const uint8_t PIN_ESP_TX = 3;     // Arduino Pin 3 connects to ESP8266 RX

// --- SOFTWARE SERIAL FOR ESP8266 ---
SoftwareSerial espSerial(PIN_ESP_RX, PIN_ESP_TX); // RX, TX

// --- WI-FI & CLOUD CREDENTIALS ---
//const String WIFI_SSID = "my_SSID";         // my Wi-Fi Name case using physical hardware
//const String WIFI_PASS = "my_WiFi_Password";     // my Wi-Fi Password case using physical hardware
const String API_KEY   = "9P32BTD376U813WC";// Write API Key from ThingSpeak
const String HOST      = "api.thingspeak.com";
const String PORT      = "80";


// --- TELEMETRY & SECURITY THRESHOLDS ---
const int INTRUDER_DISTANCE_CM = 50;  // Alert triggered if object < 50 cm
const unsigned long UPLOAD_INTERVAL = 15000; // Cloud upload every 15s (ThingSpeak rate limit)

unsigned long lastUploadTime = 0;

// --- FUNCTION DECLARATIONS ---
long measureDistanceCM();
void initESP8266();
bool sendATCommand(String cmd, unsigned long timeoutMs, String expectedResponse);
void uploadTelemetry(long distance, bool intruderDetected);

void setup() {
  // 1. Initialize Hardware Serial (for Debugging)
  Serial.begin(9600);
  
  // 2. Initialize Software Serial (for ESP8266 at standard 9600 baud)
  espSerial.begin(9600);

  // 3. Configure Ultrasonic Sensor Pins
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.println(F("\n=============================================="));
  Serial.println(F(" IoT Cloud Security Node Initializing...       "));
  Serial.println(F("=============================================="));

  // 4. Initialize ESP8266 Wi-Fi Module
  initESP8266();
}

void loop() {
  // Continuously read distance
  long distance = measureDistanceCM();

  // Evaluate security state
  bool intruderAlert = (distance > 0 && distance < INTRUDER_DISTANCE_CM);

  // Periodic Cloud Telemetry Stream (Non-blocking timer)
  if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
    lastUploadTime = millis();
    
    Serial.print(F("[SENSOR] Live Distance: "));
    Serial.print(distance);
    Serial.print(F(" cm | Status: "));
    Serial.println(intruderAlert ? F("[INTRUDER DETECTED!]") : F("[SECURE]"));

    // Transmit telemetry payload to Cloud
    uploadTelemetry(distance, intruderAlert);
  }

  delay(200);
}

// ============================================================================
// ULTRASONIC SENSOR MEASUREMENT
// ============================================================================
long measureDistanceCM() {
  // Clear trigger pin
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Send 10µs ultrasonic trigger pulse
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Read echo travel duration in microseconds (Timeout: 30ms = ~500cm max)
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duration == 0) {
    return 400; // Out of range or no reflection
  }

  // Speed of sound = 343 m/s = 0.0343 cm/µs (Divide by 2 for round trip)
  long distanceCm = duration * 0.0343 / 2;
  return constrain(distanceCm, 2, 400);
}

// ============================================================================
// ESP8266 WI-FI & TCP/HTTP STACK
// ============================================================================
void initESP8266() {
  delay(1000);
  sendATCommand("AT", 2000, "OK");
  sendATCommand("AT+CWMODE=1", 2000, "OK"); // Station mode (Client)
  
  // Connect to Wi-Fi Network
  String connectCmd = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"";
  sendATCommand(connectCmd, 8000, "OK");
  
  Serial.println(F("[WIFI] Module connected & ready."));
}

void uploadTelemetry(long distance, bool intruderDetected) {
  // Establish TCP connection to Cloud Server
  String startTCP = "AT+CIPSTART=\"TCP\",\"" + HOST + "\"," + PORT;
  if (!sendATCommand(startTCP, 4000, "OK")) {
    Serial.println(F("[ERROR] TCP Connection Failed."));
    return;
  }

  // Build HTTP GET Request (Field 1 = Distance, Field 2 = Alert Status)
  String httpRequest = "GET /update?api_key=" + API_KEY + 
                       "&field1=" + String(distance) + 
                       "&field2=" + String(intruderDetected ? 1 : 0) + 
                       " HTTP/1.1\r\n" +
                       "Host: " + HOST + "\r\n" +
                       "Connection: close\r\n\r\n";

  // Send payload length
  String sendCmd = "AT+CIPSEND=" + String(httpRequest.length());
  if (sendATCommand(sendCmd, 2000, ">")) {
    espSerial.print(httpRequest);
    Serial.println(F("[CLOUD] Telemetry stream published successfully!"));
  }

  // Close connection
  sendATCommand("AT+CIPCLOSE", 2000, "OK");
}

bool sendATCommand(String cmd, unsigned long timeoutMs, String expectedResponse) {
  espSerial.println(cmd);
  unsigned long start = millis();
  String response = "";

  while (millis() - start < timeoutMs) {
    while (espSerial.available()) {
      char c = espSerial.read();
      response += c;
    }
    if (response.indexOf(expectedResponse) != -1) {
      return true;
    }
  }
  return false;
}