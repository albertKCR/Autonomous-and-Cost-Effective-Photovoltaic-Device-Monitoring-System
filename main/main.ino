#include <Wire.h>
#include <ADS1256.h>
#include "WiFi.h"
#include "PVmonitoringSystem.h"
#include "Adafruit_MCP4725.h"
#include "SparkFun_External_EEPROM.h"
#include "DHT.h"
#include "HTTPSRedirect.h"
#include "time.h"

// Measurement control
int scaleIndex = 0;           // Current shunt scale index
int totalDataPoints = 0;      // Total stored points
int measurementsPerVoltage = 1;
float currentSum = 0;
float voltageDifference = 0;

float humidityValue, temperatureValue, irradianceValue;

int startVoltage_mV, endVoltage_mV, timeStep_ms, voltageStep_mV;
int scheduledHour, scheduledMinute;
int relayChannelIndex;
bool relaySelection[7] = {0};
int startVoltageBits, endVoltageBits, voltageStepBits;

double channel1Voltage, measuredVoltageDiff, appliedVoltage, measuredCurrent;
bool adcOverflowHigh, adcOverflowLow;

// Relay bit patterns
byte cellRelayPattern[8] = {
    B00010000, B00100000, B00000001, B00000010,
    B00000100, B00001000, B01000000, B10000000
};

int shuntRelayPattern[7] = {0x00, 0x20, 0x04, 0x14, 0x01, 0x03, 0x09};
float shuntResistances[7] = {100000, 10000, 1000, 100, 10, 1, 0.1};

// Hardware objects
Adafruit_MCP4725 dac;
ADS1256 adc(16, 17, 0, 5, 2.500);
ExternalEEPROM eepromChip;
DHT dhtSensor(4, DHT22);
HTTPSRedirect* httpsClient = nullptr;

// NTP
const char* ntpServerHost = "pool.ntp.org";
const long gmtOffsetSeconds = -10800;
const int daylightOffsetSeconds = 0;
int currentHour, currentMinute;

// Internet
String wifiSSID = "";
String wifiPassword = "";

// Google script
const char* deleteSheetCurrentDataURL = "";
const char* copyTempDataToChartURL = "";
const char* saveDataToGoogleDriveURL = "";
const char* sendDataToGoogleSheetURL = "";
const char* loadSettingsFromSheetURL = "";

void setup() {
    Serial.begin(115200);

    pinMode(latchPin1, OUTPUT);
    pinMode(clockPin1, OUTPUT);
    pinMode(dataPin1, OUTPUT);
    pinMode(latchPin2, OUTPUT);
    pinMode(clockPin2, OUTPUT);
    pinMode(dataPin2, OUTPUT);
    digitalWrite(latchPin1, HIGH);
    digitalWrite(latchPin2, HIGH);

    // Reset all relays
    digitalWrite(latchPin1, LOW);
    shiftOut(dataPin1, clockPin1, MSBFIRST, 0x00);
    digitalWrite(latchPin1, HIGH);

    digitalWrite(latchPin2, LOW);
    shiftOut(dataPin2, clockPin2, MSBFIRST, B00000000);
    digitalWrite(latchPin2, HIGH);

    Wire.begin();
    if (!eepromChip.begin(0b1010000, Wire)) {
        while (1);
    }
    adc.InitializeADC();
    dac.begin(0x60);
    dac.setVoltage(0x7FF, false);
    dhtSensor.begin();

    WiFi.mode(WIFI_STA);
    autonomousOperation();
}

void loop() {
    // No repeated loop actions — autonomous mode handles everything
}
