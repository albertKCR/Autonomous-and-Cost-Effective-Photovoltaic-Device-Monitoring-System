# ☀️ Autonomous and Low-Cost Photovoltaic Device Monitoring System

## 🔬 Overview
This project presents an **automated, and cost-effective I–V curve tracer** designed for **long-term monitoring of photovoltaic device prototypes**.  
The system measures **electrical parameters** such as:
- Short-circuit current (**ISC**)
- Open-circuit voltage (**VOC**)
- Fill factor (**FF**)
- Power conversion efficiency (**PCE**)

It also records **environmental conditions** like temperature, humidity, and light intensity.  

Using an **ESP32 microcontroller**, data is collected, processed, and transmitted via **Wi-Fi** to a **cloud-based spreadsheet and web interface** for real-time monitoring.  
This open-source platform is suitable for **laboratory**, **glovebox**, and **outdoor testing**, following **ISOS stability protocols**.

---

## 📌 Features
- **Voltage sweep range**: -2.5 V to +2.5 V (1.2 mV resolution)  
- **Current measurement**: down to **1 nA** with automated range selection  
- **Environmental monitoring**: temperature, humidity, and light intensity  
- **Cloud integration**: data sent to Google Sheets & displayed on a web dashboard  
- **Multi-device support**: up to **8 photovoltaic prototypes** monitored sequentially  
- **Open-source hardware and software** for easy replication and customization  

---

## 🏗️ Hardware & Assembly

### Required Components
- **Microcontroller**: ESP32  
- **Analog front-end**:  
  - DAC: MCP4725 (12-bit)  
  - ADC: ADS1256 (24-bit)  
  - Op-amps: CA3140, TL082, LMC662 (ultra-low bias current)  
- **Relay systems**: for resistor selection and device switching (AGN200A4HZ)  
- **Shunt resistors**: 0.1 Ω to 100 kΩ for current ranges from 240 nA to 250 mA  
- **Environmental sensors**:  
  - DHT22 (temperature & humidity)  
  - BPW34 photodiode (irradiance measurement)  
- **Power supply**: ±12 VDC, ±8 VDC, +5 VDC regulated outputs  

---

### Measurement Ranges

| Resistor | Max Current |
|----------|-------------|
| 100 kΩ   | 240 nA      |
| 10 kΩ    | 2.4 µA      |
| 1 kΩ     | 24 µA       |
| 100 Ω    | 240 µA      |
| 10 Ω     | 2.4 mA      |
| 1 Ω      | 24 mA       |
| 0.1 Ω    | 250 mA      |

---

## 💻 Software Installation
- **Configure** <a href="https://docs.google.com/spreadsheets/d/1kSAy_Uos-emDk9aBooSjsmjM2wZ1Fo_ap92WN2WwyN0/edit?usp=sharing">Google sheets</a>. 
- **Deploy** each Google API Script.
- Paste the Google sheets charts links in the web page and host the **Web Interface**.
- **Configure** Wi-Fi credentials and Google Script ID in the firmware.

---

## 🎛️ Usage
1. Access the **Web Interface** through your browser.  
2. In the **Settings tab**, enter the desired measurement parameters.
3. Start the measurement process.  
4. View the generated **I–V curves** in the **I–V Curves tab**.  
5. Monitor photovoltaic and environmental parameters in the **Results tab**.  
6. Export data as CSV files directly from the web interface for further analysis.

---


## 📂 Design Files
All design files are available for replication:

| File Name        | Description                     |
|------------------|---------------------------------|
| `main.ino` | ESP32 firmware                 |
| `PVmonitoringSystem` | Equipment library                 |
| `Other libraries.zip`     | Required Arduino libraries     |
| `Web_page_files.zip`| Interface HTML/JS/CSS          |
| `Gerber_Photovoltaic_Monitoring_System.zip`    | PCB manufacturing files        |

---

## 📧 Contact
For questions, suggestions, or collaborations, contact:  
- **Albert Kirchner** (kirchner_beto@hotmail.com)

---
