#include "PVmonitoringSystem.h"


// Update relay outputs for the current scale index
void updateShuntScale() {
    digitalWrite(latchPin1, LOW);
    shiftOut(dataPin1, clockPin1, MSBFIRST, shuntRelayPattern[scaleIndex]);
    digitalWrite(latchPin1, HIGH);
}

// Increase scale index (lower current range)
void increaseShuntScale() {
    if (scaleIndex + 1 < 7) {
        scaleIndex++;
        updateShuntScale();
    }
}

// Decrease scale index (higher current range)
void decreaseShuntScale() {
    if (scaleIndex - 1 >= 0) {
        scaleIndex--;
        updateShuntScale();
    }
}

// Read integer from serial input
void scanIntegerInput(int &value) {
    bool waiting = true;
    int serialValue = 0;
    char buffer[20];

    while (waiting) {
        if (Serial.available() > 0) {
            int count = 0;
            for (int i = 0; Serial.available() > 0; i++) {
                buffer[i] = Serial.read();
                count = i;
                delay(10);
            }

            if (buffer[0] == '-') {
                count--;
                for (int i = 0; i <= count; i++) {
                    buffer[i + 1] -= 48;
                    serialValue += buffer[i + 1] * pow(10, (count - i));
                }
                serialValue *= -1;
                if (count > 1) serialValue--;
            } else {
                for (int i = 0; i <= count; i++) {
                    buffer[i] -= 48;
                    serialValue += buffer[i] * pow(10, (count - i));
                }
                if (count > 1) ++serialValue;
            }

            value = serialValue;
            waiting = false;
        }
    }
}

// Save a measurement to EEPROM
void saveMeasurement(int address, float voltage, float current, float diff) {
    address = 1 + ((address - 1) * 12);
    eepromChip.put(address, voltage);
    address += 4;
    eepromChip.put(address, current);
    address += 4;
    eepromChip.put(address, diff);
}

// Read a measurement from EEPROM
void readMeasurement(int address, float &voltage, float &current, float &diff) {
    address = 1 + ((address - 1) * 12);
    eepromChip.get(address, voltage);
    address += 4;
    eepromChip.get(address, current);
    address += 4;
    eepromChip.get(address, diff);
}

// Read only the diff value from EEPROM
float readDiffFromEEPROM(int address) {
    float data;
    address = ((address - 1) * 12) + 9;
    eepromChip.get(address, data);
    return data;
}

// -------------------------
// Autonomous Operation
// -------------------------

void autonomousOperation() {
    connectToWiFi();
    configTime(gmtOffsetSeconds, daylightOffsetSeconds, ntpServerHost);

    while (true) {
        loadSettingsFromSheet();
        updateCurrentTime();

        if (currentHour == scheduledHour && currentMinute == scheduledMinute) {
            deleteSheetCurrentData();

            for (int i = 0; i < 8; i++) {
                if (!relaySelection[i]) continue;

                digitalWrite(latchPin2, LOW);
                shiftOut(dataPin2, clockPin2, MSBFIRST, cellRelayPattern[i]);
                digitalWrite(latchPin2, HIGH);
                delay(3000);
                relayChannelIndex = i;

                digitalWrite(lampPin, HIGH);
                autonomousSweep();
                digitalWrite(lampPin, LOW);

                digitalWrite(latchPin1, LOW);
                shiftOut(dataPin1, clockPin1, MSBFIRST, 0x00);
                digitalWrite(latchPin1, HIGH);

                digitalWrite(latchPin2, LOW);
                shiftOut(dataPin2, clockPin2, MSBFIRST, 0x00);
                digitalWrite(latchPin2, HIGH);

                measureAllSensors();
                sendDataToGoogleSheet();
            }

            copyTempDataToChart();
            saveDataToGoogleDrive();

            if (WiFi.status() != WL_CONNECTED) {
                connectToWiFi();
            }
        }
    }
}

void autonomousSweep() {
    startVoltageBits = ((startVoltage_mV - 100 + 2500) / 0.9) * 0.001 * (4095 / 5);
    endVoltageBits   = ((endVoltage_mV   + 150 + 2500) / 0.9) * 0.001 * (4095 / 5);
    voltageStepBits  = (voltageStep_mV * 0.001) * (4095 / 5);

    dac.setVoltage(startVoltageBits, false);
    scaleIndex = 0;
    updateShuntScale();

    adcMeasure(shuntResistances[scaleIndex]);
    while ((adcOverflowHigh && scaleIndex != 6) || (adcOverflowLow && scaleIndex != 0)) {
        if (adcOverflowHigh) { increaseShuntScale(); delay(100); }
        else if (adcOverflowLow) { decreaseShuntScale(); delay(100); }
        adcMeasure(shuntResistances[scaleIndex]);
    }

    sweepControlLoop(startVoltageBits, endVoltageBits, timeStep_ms);

    dac.setVoltage(0x7FF, false);
    scaleIndex = 0;
    updateShuntScale();
}

void averageMeasurements(int count) {
    currentSum = 0;
    for (int i = 0; i < count; i++) {
        adcMeasure(shuntResistances[scaleIndex]);
        currentSum += measuredCurrent;
    }
    measuredCurrent = currentSum / count;
}

void adcMeasure(float shuntResistance) {
    adc.setMUX(SING_6);
    appliedVoltage = adc.convertToVoltage(adc.readSingle()) - 2.5;

    adc.setMUX(SING_2);
    channel1Voltage = adc.convertToVoltage(adc.readSingle()) - 2.5;

    measuredVoltageDiff = appliedVoltage - (channel1Voltage / 100);
    measuredCurrent = channel1Voltage / (100 * shuntResistance);
    voltageDifference = measuredVoltageDiff;

    if (abs(channel1Voltage) > 2.40) {
        adcOverflowHigh = true;
        adcOverflowLow = false;
    } else if (abs(channel1Voltage) < 0.1) {
        adcOverflowHigh = false;
        adcOverflowLow = true;
    } else {
        adcOverflowHigh = false;
        adcOverflowLow = false;
    }
}

void sweepControlLoop(int startBits, int endBits, int timeStepMs) {
    unsigned long startTime;
    float v, c, d;

    for (int dacValue = startBits; dacValue < endBits; dacValue += voltageStepBits) {
        while ((adcOverflowHigh && scaleIndex != 6) || (adcOverflowLow && scaleIndex != 0)) {
            if (adcOverflowHigh) { increaseShuntScale(); dac.setVoltage(dacValue, false); }
            else if (adcOverflowLow) { decreaseShuntScale(); dac.setVoltage(dacValue, false); }

            while (voltageDifference > readDiffFromEEPROM(totalDataPoints)) {
                dac.setVoltage(dacValue, false);
                adcMeasure(shuntResistances[scaleIndex]);
            }
            adcMeasure(shuntResistances[scaleIndex]);
        }

        adcMeasure(shuntResistances[scaleIndex]);
        startTime = millis();

        saveMeasurement(totalDataPoints + 1, appliedVoltage, measuredCurrent, measuredVoltageDiff);
        readMeasurement(totalDataPoints + 1, v, c, d);

        totalDataPoints++;
        dac.setVoltage(dacValue, false);

        while (millis() - startTime < (unsigned long)timeStepMs) {}
    }
}

// -------------------------
// Sensors
// -------------------------

void measureIrradiance() {
    digitalWrite(latchPin2, LOW);
    shiftOut(dataPin2, clockPin2, MSBFIRST, cellRelayPattern[0]);
    digitalWrite(latchPin2, HIGH);

    dac.setVoltage(0, false);
    measurementsPerVoltage = 10;

    scaleIndex = 3;
    updateShuntScale();

    adcMeasure(shuntResistances[scaleIndex]);
    while ((adcOverflowHigh && scaleIndex != 6) || (adcOverflowLow && scaleIndex != 0)) {
        if (adcOverflowHigh) { increaseShuntScale(); delay(100); }
        else if (adcOverflowLow) { decreaseShuntScale(); delay(100); }
        adcMeasure(shuntResistances[scaleIndex]);
    }

    digitalWrite(latchPin2, LOW);
    shiftOut(dataPin2, clockPin2, MSBFIRST, 0x00);
    digitalWrite(latchPin2, HIGH);

    averageMeasurements(measurementsPerVoltage);

    digitalWrite(latchPin2, LOW);
    shiftOut(dataPin2, clockPin2, MSBFIRST, 0b00000000);
    digitalWrite(latchPin2, HIGH);

    irradianceValue = measuredCurrent;
}

void measureAllSensors() {
    humidityValue = dhtSensor.readHumidity();
    temperatureValue = dhtSensor.readTemperature();
    if (isnan(temperatureValue) || isnan(humidityValue)) {
        Serial.println("Failed to read from DHT sensor");
    }
    measureIrradiance();
}

// -------------------------
// Internet
// -------------------------

void connectToWiFi() {
    const char* host = "script.google.com";
    const int httpsPort = 443;

    WiFi.begin(wifiSSID, wifiPassword);
    Serial.println(WiFi.macAddress());

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    delete httpsClient;
    httpsClient = new HTTPSRedirect(httpsPort);
    httpsClient->setInsecure();
    httpsClient->setPrintResponseBody(true);
    httpsClient->setContentTypeHeader("application/json");

    for (int i = 0; i < 5; i++) {
        if (httpsClient->connect(host, httpsPort) == 1) {
            Serial.println("[OK]");
            return;
        }
        Serial.println("[Error]");
    }
    Serial.println("[Error connecting to host]");
}

void updateCurrentTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        currentHour = timeinfo.tm_hour;
        currentMinute = timeinfo.tm_min;
    }
}

// -------------------------
// Google Sheets / Drive
// -------------------------

void deleteSheetCurrentData() {
    String url = String("/macros/s/") + deleteSheetCurrentDataURL + "/exec?cal";
    const char* host = "script.google.com";
    const int httpsPort = 443;

    if (!httpsClient) {
        httpsClient = new HTTPSRedirect(httpsPort);
        httpsClient->setInsecure();
        httpsClient->setPrintResponseBody(true);
        httpsClient->setContentTypeHeader("application/json");
    }

    if (!httpsClient->connected()) {
        httpsClient->connect(host, httpsPort);
    }

    if (httpsClient->GET(url, host)) {
        Serial.println("[OK]");
    } else {
        Serial.println("[Error]");
    }
}

void copyTempDataToChart() {
    String url = String("/macros/s/") + copyTempDataToChartURL + "/exec?cal";
    const char* host = "script.google.com";

    if (httpsClient->GET(url, host)) {
        Serial.println("[OK]");
    } else {
        Serial.println("[Error]");
    }
}

void saveDataToGoogleDrive() {
    String url = String("/macros/s/") + saveDataToGoogleDriveURL + "/exec?cal";
    const char* host = "script.google.com";

    if (httpsClient->GET(url, host)) {
        Serial.println("[OK]");
    } else {
        Serial.println("[Error]");
    }
}

void sendDataToGoogleSheet() {
    const char* GScriptId = sendDataToGoogleSheetURL;
    String payloadBase = "{\"command\": \"append_row\", \"sheet_name\": \"Sheet1\", \"relayIndex\": \"" 
                        + String(relayChannelIndex) + "\", \"values\": ";
    String payload;
    const char* host = "script.google.com";
    const int httpsPort = 443;
    String url = String("/macros/s/") + GScriptId + "/exec?cal";

    String batchData[40];
    int batchIndex = 0;
    float v, c, d;

    while (totalDataPoints > 0) {
        int itemsToSend = min(40, totalDataPoints);
        for (int i = 0; i < itemsToSend; i++) {
            readMeasurement(batchIndex + 1, v, c, d);
            batchData[i] = String(c, 13) + "," + String(d, 5);
            batchIndex++;
        }
        totalDataPoints -= itemsToSend;

        payload = payloadBase + "\"" + batchData[0];
        for (int i = 1; i < itemsToSend; i++) {
            payload += "," + batchData[i];
        }
        payload += "\"}";

        if (httpsClient->POST(url, host, payload)) {
            Serial.println("[OK]");
        } else {
            Serial.println("[Error]");
        }
    }

    // Send sensor data
    payloadBase = "{\"command\": \"sensor\", \"sheet_name\": \"sensor\", \"values\": ";
    payload = payloadBase + "\"" + String(humidityValue) + "," 
            + String(temperatureValue) + "," + String(irradianceValue) + "\"}";

    if (httpsClient->POST(url, host, payload)) {
        Serial.println("[OK]");
    } else {
        Serial.println("[Error]");
    }

    totalDataPoints = 0;
}

void loadSettingsFromSheet() {
    String url = String("/macros/s/") + loadSettingsFromSheetURL + "/exec?cal";
    const char* host = "script.google.com";

    if (!httpsClient) {
        httpsClient = new HTTPSRedirect(443);
        httpsClient->setInsecure();
        httpsClient->setPrintResponseBody(true);
        httpsClient->setContentTypeHeader("application/json");
    }

    if (!httpsClient->connected()) {
        httpsClient->connect(host, 443);
    }

    if (httpsClient->GET(url, host)) {
        String response = httpsClient->getResponseBody();

        int idx = response.indexOf(":");
        scheduledHour = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        idx = response.indexOf(",");
        scheduledMinute = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        idx = response.indexOf(",");
        startVoltage_mV = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        idx = response.indexOf(",");
        endVoltage_mV = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        idx = response.indexOf(",");
        timeStep_ms = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        idx = response.indexOf(",");
        voltageStep_mV = response.substring(0, idx).toInt();

        response = response.substring(idx + 1);
        for (int i = 0; i < 8; i++) relaySelection[i] = false;
        if (response.indexOf("1") != -1) relaySelection[0] = true;
        if (response.indexOf("2") != -1) relaySelection[1] = true;
        if (response.indexOf("3") != -1) relaySelection[2] = true;
        if (response.indexOf("4") != -1) relaySelection[3] = true;
        if (response.indexOf("5") != -1) relaySelection[4] = true;
        if (response.indexOf("6") != -1) relaySelection[5] = true;
        if (response.indexOf("7") != -1) relaySelection[6] = true;

        Serial.println("[OK]");
    } else {
        Serial.println("[Error]");
    }
}
