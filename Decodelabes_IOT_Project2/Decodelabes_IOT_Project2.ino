/**
 * ============================================================================
 * Project: Automated Irrigation Controller (Closed-Loop Actuator Logic)
 * Target: Arduino Uno (ATmega328P)
 * Description: Production-ready firmware featuring safe active-low relay boot,
 *              Exponential Moving Average (EMA) signal filtering, ADC calibration
 *              normalization, and dual-threshold closed-loop hysteresis control.
 * ============================================================================
 */

#include <Arduino.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
constexpr uint8_t PIN_SOIL_SENSOR = A0;  // Analog input from Soil Moisture Sensor
constexpr uint8_t PIN_RELAY_PUMP  = 7;   // Digital output to Active-LOW Relay Module

// ============================================================================
// RELAY LOGIC LEVEL DEFINITIONS (ACTIVE-LOW)
// ============================================================================
constexpr uint8_t RELAY_ACTIVE_STATE   = LOW;   // Energize coil -> Turn ON Pump
constexpr uint8_t RELAY_INACTIVE_STATE = HIGH;  // De-energize coil -> Turn OFF Pump

// ============================================================================
// SIGNAL FILTERING PARAMETERS (EMA)
// ============================================================================
// Alpha smoothing factor: 0.2 (20% current reading, 80% historical weight)
constexpr float EMA_ALPHA = 0.2f;

// ============================================================================
// CALIBRATION & NORMALIZATION CONSTANTS
// ============================================================================
// Field Calibration Anchors (10-bit ADC: 0 - 1023)
constexpr int16_t ADC_DRY = 850;  // Raw ADC reading in dry air (0% Moisture)
constexpr int16_t ADC_WET = 350;  // Raw ADC reading in water (100% Moisture)

// ============================================================================
// HYSTERESIS THRESHOLDS (PERCENTAGE: 0% - 100%)
// ============================================================================
constexpr int8_t TURN_ON_THRESHOLD  = 30; // Start watering when moisture falls below 30%
constexpr int8_t TURN_OFF_THRESHOLD = 45; // Stop watering when moisture rises above 45%
// Deadband Range [30% - 45%]: Holds previous state to prevent relay contact chattering

// ============================================================================
// TIMING CONSTANTS (NON-BLOCKING)
// ============================================================================
constexpr uint32_t SAMPLE_INTERVAL_MS    = 100;  // ADC acquisition & filtering interval
constexpr uint32_t TELEMETRY_INTERVAL_MS = 1000; // Serial monitor output interval

// ============================================================================
// STATE & FILTER VARIABLES
// ============================================================================
float   g_filtered_adc     = 0.0f;
int8_t  g_moisture_percent = 0;
bool    g_pump_active      = false;

uint32_t g_last_sample_time    = 0;
uint32_t g_last_telemetry_time = 0;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void updateSensorFilter();
void processHysteresisLogic();
void emitTelemetry();
void setPumpState(bool activate);

// ============================================================================
// SYSTEM INITIALIZATION (setup)
// ============================================================================
void setup() {
  // --------------------------------------------------------------------------
  // CRITICAL SAFE BOOT SEQUENCE FOR ACTIVE-LOW RELAYS:
  // Pre-load the output register with HIGH *before* setting pinMode to OUTPUT.
  // This avoids transient low glitches (microsecond brownout firing) during MCU boot.
  // --------------------------------------------------------------------------
  digitalWrite(PIN_RELAY_PUMP, RELAY_INACTIVE_STATE);
  pinMode(PIN_RELAY_PUMP, OUTPUT);

  // Initialize Serial Telemetry
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {
    // Wait for native USB if applicable
  }

  // Pre-charge the EMA filter with an initial sample
  g_filtered_adc = static_cast<float>(analogRead(PIN_SOIL_SENSOR));

  Serial.println(F("\n======================================================="));
  Serial.println(F(" Automated Irrigation Controller Firmware Initialized "));
  Serial.println(F(" Relay Driver: ACTIVE-LOW on Pin 7 | Sensor on Pin A0   "));
  Serial.println(F("=======================================================\n"));
}

// ============================================================================
// MAIN EXECUTION LOOP (loop)
// ============================================================================
void loop() {
  const uint32_t current_time = millis();

  // Task 1: Periodic ADC Sampling & Signal Conditioning (10 Hz)
  if (current_time - g_last_sample_time >= SAMPLE_INTERVAL_MS) {
    g_last_sample_time = current_time;

    // Phase 1: Signal Filtering
    updateSensorFilter();

    // Phase 3: Closed-Loop Hysteresis Actuator Evaluation
    processHysteresisLogic();
  }

  // Task 2: Periodic Telemetry Logging (1 Hz)
  if (current_time - g_last_telemetry_time >= TELEMETRY_INTERVAL_MS) {
    g_last_telemetry_time = current_time;
    emitTelemetry();
  }
}

// ============================================================================
// PHASE 1 & 2: SIGNAL CONDITIONING, FILTERING & NORMALIZATION
// ============================================================================
void updateSensorFilter() {
  // Acquire raw 10-bit ADC sample
  const int16_t raw_adc = analogRead(PIN_SOIL_SENSOR);

  // Phase 1: Exponential Moving Average (EMA) Filter
  // Formula: Filtered = (alpha * Raw) + ((1 - alpha) * Previous_Filtered)
  g_filtered_adc = (EMA_ALPHA * static_cast<float>(raw_adc)) + ((1.0f - EMA_ALPHA) * g_filtered_adc);

  // Phase 2: Calibration Mapping & Saturation Constrain
  // Note: Soil resistance drops with moisture, so higher ADC = Dry, lower ADC = Wet.
  const long mapped_value = map(static_cast<long>(g_filtered_adc + 0.5f), ADC_DRY, ADC_WET, 0, 100);

  // Clamp output firmly between 0% and 100%
  g_moisture_percent = static_cast<int8_t>(constrain(mapped_value, 0, 100));
}

// ============================================================================
// PHASE 3: CLOSED-LOOP HYSTERESIS ACTUATOR LOGIC
// ============================================================================
void processHysteresisLogic() {
  if (!g_pump_active && (g_moisture_percent < TURN_ON_THRESHOLD)) {
    // Soil is critically dry: trip lower threshold
    setPumpState(true);
  } 
  else if (g_pump_active && (g_moisture_percent > TURN_OFF_THRESHOLD)) {
    // Sufficient saturation reached: trip upper threshold
    setPumpState(false);
  }
  // Deadband [TURN_ON_THRESHOLD ... TURN_OFF_THRESHOLD]: Retain current state
}

// ============================================================================
// HARDWARE ACTUATOR ABSTRACTION
// ============================================================================
void setPumpState(bool activate) {
  g_pump_active = activate;
  digitalWrite(PIN_RELAY_PUMP, activate ? RELAY_ACTIVE_STATE : RELAY_INACTIVE_STATE);
}

// ============================================================================
// TELEMETRY & DIAGNOSTICS
// ============================================================================
void emitTelemetry() {
  Serial.print(F("[TELEMETRY] Raw ADC Filtered: "));
  Serial.print(g_filtered_adc, 1);
  Serial.print(F(" | Moisture: "));
  Serial.print(g_moisture_percent);
  Serial.print(F("% | State: "));

  if (g_pump_active) {
    Serial.println(F("[PUMP ACTIVE - WATERING]"));
  } else {
    Serial.println(F("[STANDBY - IDLE]"));
  }
}