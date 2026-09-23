/**
 * ============================================================================
 * Project 4: Edge-Computing Smart Home Appliance (Interrupts & Safety)
 * Hardware: Arduino Uno + PIR Motion Sensor + Gas/Smoke Sensor + Actuators
 * ============================================================================
 */

const uint8_t PIN_PIR_INTERRUPT = 2;  // PIR Sensor (Hardware Interrupt INT0)
const uint8_t PIN_GAS_SENSOR    = A0; // Analog Gas/Smoke Sensor

const uint8_t PIN_SMART_LIGHT   = 8;  // Smart Room Light (Green LED)
const uint8_t PIN_ALARM_BUZZER  = 9;  // Emergency Siren Buzzer (Tone generator)
const uint8_t PIN_FAULT_LED     = 10; // Flashing Red Alert LED

const int GAS_DANGER_THRESHOLD  = 400;   // Threshold for toxic gas
const unsigned long LIGHT_TIMEOUT_MS = 5000; 
const unsigned long BLINK_INTERVAL_MS = 150; 

volatile bool g_motionDetected = false;
volatile unsigned long g_lastMotionTime = 0;

bool g_safetyOverrideActive = false;
unsigned long g_lastBlinkTime = 0;
bool g_faultLedState = false;

// Hardware Interrupt Service Routine
void pirMotionISR() {
  int pirState = digitalRead(PIN_PIR_INTERRUPT);
  if (pirState == HIGH) {
    g_motionDetected = true;
    g_lastMotionTime = millis();
  } else {
    g_motionDetected = false;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_SMART_LIGHT, OUTPUT);
  pinMode(PIN_ALARM_BUZZER, OUTPUT);
  pinMode(PIN_FAULT_LED, OUTPUT);

  digitalWrite(PIN_SMART_LIGHT, LOW);
  noTone(PIN_ALARM_BUZZER);
  digitalWrite(PIN_FAULT_LED, LOW);

  // Attach Hardware Interrupt to Pin 2
  pinMode(PIN_PIR_INTERRUPT, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_PIR_INTERRUPT), pirMotionISR, CHANGE);

  Serial.println(F("Smart Home Edge Controller Initialized."));
}

void loop() {
  int gasLevel = analogRead(PIN_GAS_SENSOR);

  // ==========================================================================
  // EMERGENCY SAFETY OVERRIDE
  // ==========================================================================
  if (gasLevel > GAS_DANGER_THRESHOLD) {
    g_safetyOverrideActive = true;

    // 1. Force Smart Light OFF
    digitalWrite(PIN_SMART_LIGHT, LOW);

    // 2. Play audible 1000Hz emergency siren on Pin 9
    tone(PIN_ALARM_BUZZER, 1000);

    // 3. Strobe Red LED
    if (millis() - g_lastBlinkTime >= BLINK_INTERVAL_MS) {
      g_lastBlinkTime = millis();
      g_faultLedState = !g_faultLedState;
      digitalWrite(PIN_FAULT_LED, g_faultLedState);
    }

    Serial.print(F("[CRITICAL ALERT] GAS DETECTED! Level: "));
    Serial.println(gasLevel);
  }
  // ==========================================================================
  // NORMAL OPERATION
  // ==========================================================================
  else {
    if (g_safetyOverrideActive) {
      g_safetyOverrideActive = false;
      noTone(PIN_ALARM_BUZZER); // Turn OFF Buzzer
      digitalWrite(PIN_FAULT_LED, LOW);
      Serial.println(F("[SYSTEM RECOVERED] Normal Mode."));
    }

    // Motion Light Logic
    if (g_motionDetected || (millis() - g_lastMotionTime < LIGHT_TIMEOUT_MS)) {
      digitalWrite(PIN_SMART_LIGHT, HIGH);
    } else {
      digitalWrite(PIN_SMART_LIGHT, LOW);
    }
  }

  delay(20);
}