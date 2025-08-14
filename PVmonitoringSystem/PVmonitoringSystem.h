#ifndef PVmonitoringSystem_H
#define PVmonitoringSystem_H

#include "Adafruit_MCP4725.h"
#include <ADS1256.h>
#include "SparkFun_External_EEPROM.h"
#include <Wire.h>
#include "DHT.h"
#include "WiFi.h"
#include "HTTPSRedirect.h"

// -------------------------
// Global Variables
// -------------------------

// Pin definitions
const int latchPin1 = 27;
const int clockPin1 = 25;
const int dataPin1  = 26;

const int latchPin2 = 32;
const int clockPin2 = 33;
const int dataPin2  = 13;

const int lampPin   = 12;

// Measurement control
extern int scaleIndex;           // Current shunt scale index
extern int totalDataPoints;      // Total stored points
extern int measurementsPerVoltage;
extern float currentSum;
extern float voltageDifference;
 
extern float humidityValue, temperatureValue, irradianceValue;
 
extern int startVoltage_mV, endVoltage_mV, timeStep_ms, voltageStep_mV;
extern int scheduledHour, scheduledMinute;
extern int relayChannelIndex;
extern bool relaySelection[7];
extern int startVoltageBits, endVoltageBits, voltageStepBits;
 
extern double channel1Voltage, measuredVoltageDiff, appliedVoltage, measuredCurrent;
extern bool adcOverflowHigh, adcOverflowLow;
 
 // Relay bit patterns
extern byte cellRelayPattern[8];
 
extern int shuntRelayPattern[7];
extern float shuntResistances[7];
 
 // Hardware objects
extern Adafruit_MCP4725 dac;
extern ADS1256 adc;
extern ExternalEEPROM eepromChip;
extern DHT dhtSensor;
extern HTTPSRedirect* httpsClient;
 
 // NTP
extern const char* ntpServerHost;
extern const long gmtOffsetSeconds;
extern const int daylightOffsetSeconds;
extern int currentHour, currentMinute;

// Internet
extern String wifiSSID;
extern String wifiPassword;

// Google script
extern const char* deleteSheetCurrentDataURL;
extern const char* copyTempDataToChartURL;
extern const char* saveDataToGoogleDriveURL;
extern const char* sendDataToGoogleSheetURL;
extern const char* loadSettingsFromSheetURL;

// --- Calibration ---
void scanIntegerInput(int &value);                 // Read integer from serial input
void calibration();

// --- EEPROM ---
void saveMeasurement(int address, float voltage, float current, float diff); // Save data in EEPROM
void readMeasurement(int address, float &voltage, float &current, float &diff); // Read data from EEPROM
float readDiffFromEEPROM(int address);             // Read only diff value from EEPROM

// --- Ammeter scale control ---
void updateShuntScale();                           // Update the relay to match scaleIndex
void increaseShuntScale();                         // Switch to next higher scale
void decreaseShuntScale();                         // Switch to next lower scale

// --- Autonomous operation ---
void autonomousOperation();                        // Run scheduled autonomous measurements
void autonomousSweep();                            // Perform an I-V sweep
void averageMeasurements(int count);               // Take multiple measurements and average them
void sweepControlLoop(int startBits, int endBits, int timeStepMs); // Sweep loop control
void adcMeasure(float shuntResistance);            // Read applied voltage and DUT current

// --- Sensors ---
void measureAllSensors();                          // Read temperature, humidity, irradiance
void measureIrradiance();                          // Read irradiance sensor

// --- Internet ---
void connectToWiFi();                              // Connect to Wi-Fi
void updateCurrentTime();                          // Get current time from NTP

// --- Google Sheets ---
void sendDataToGoogleSheet();                      // Send measurement data to Google Sheet
void copyTempDataToChart();                        // Copy temporary data to chart
void deleteSheetCurrentData();                     // Clear current data in sheet
void saveDataToGoogleDrive();                      // Save CSV file to Google Drive
void loadSettingsFromSheet();                      // Load measurement settings from sheet

#endif
