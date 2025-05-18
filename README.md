**ESP32 Dual-Core RTOS Data Logger**

> High‑precision weight and pressure data acquisition with SD logging using both cores of the ESP32 and FreeRTOS.

---

## 📋 Table of Contents

1. [Description](#description)
2. [Features](#features)
3. [Hardware Requirements](#hardware-requirements)
4. [Wiring Diagram](#wiring-diagram)
5. [Software Requirements](#software-requirements)
6. [Installation](#installation)
7. [Usage](#usage)
8. [Configuration](#configuration)
9. [Troubleshooting](#troubleshooting)
10. [Contributing](#contributing)
11. [License](#license)

---

## 🔍 Description

This project implements a robust data‑logging system on the ESP32, leveraging both CPU cores via FreeRTOS tasks. Core 0 handles high‑frequency weight and pressure sampling, while Core 1 asynchronously writes buffered CSV data to an SD card.
Ideal for experiments requiring precise timing and reliable storage over long collection periods.

---

## 🚀 Features

* **Dual‑core scheduling**: Dedicated tasks pinned to Core 0 (data acquisition) and Core 1 (SD writes).
* **Double buffering**: Two RAM buffers alternate to avoid data loss during SD writes.
* **High‑precision timing**: 10 ms sampling interval using `micros()` or optional RTOS delay.
* **HX711 load cell**: Calibrated weight measurement in kg and N.
* **Analog pressure sensor**: Raw and converted pressure readings.
* **Automatic file naming**: Generates incremental CSV files (`data_BTE_MORPHEUS1.csv`, `data_BTE_MORPHEUS2.csv`, …).

---

## 🛠️ Hardware Requirements

* ESP32 development module
* HX711 load‑cell amplifier
* Load cell (strain gauge)
* Analog pressure transducer
* Micro SD card module + SD card
* Connecting wires
* Breadboard (optional)

---

## 🔌 Wiring Diagram

| Module          | ESP32 Pin      |
| --------------- | -------------- |
| SD CS           | GPIO 5         |
| SD MISO (DO)    | GPIO 19 (HSPI) |
| SD MOSI (DI)    | GPIO 23 (HSPI) |
| SD SCK          | GPIO 18 (HSPI) |
| HX711 DOUT      | GPIO 2         |
| HX711 SCK       | GPIO 32        |
| Pressure Signal | GPIO 34 (ADC1) |

*(Adjust SD pins if using VSPI or custom mapping.)*

---

## 💾 Software Requirements

* Arduino IDE (>= 1.8.13) or VS Code + PlatformIO
* ESP32 Board support (Espressif)
* Libraries:

  * [HX711](https://github.com/bogde/HX711)
  * `SD.h`, `FS.h`, `SPI.h`
  * FreeRTOS (bundled with ESP32 core)

---

## ⚙️ Installation

1. Clone this repository:

   ```sh
   git clone https://github.com/<your‑username>/esp32-dualcore-data-logger.git
   cd esp32-dualcore-data-logger
   ```
2. Open in Arduino IDE or PlatformIO.
3. Install required libraries via Library Manager or `platformio.ini`.
4. Configure pin definitions or intervals in `src/main.cpp` if needed.

---

## ▶️ Usage

1. Insert SD card and power on the ESP32.
2. Open Serial Monitor at `115200 baud`.
3. Wait for calibration prompt: place calibration weight on the load cell.
4. Observe printed calibration value, then data collection will start automatically.
5. Remove SD card to retrieve CSV file(s) with columns:

   ```csv
   peso_kg,peso_N,pressao_sd,pressao_conv,tempo
   ```

---

## 🔧 Configuration

Edit the following constants in `main.cpp` as needed:

```cpp
#define SD_CS_PIN           5
#define LOADCELL_DOUT_PIN   2
#define LOADCELL_SCK_PIN    32
#define TRANSDUTOR_PIN      34
const int ciclos_coleta        = 120;
const unsigned long intervalo_us = 10000; // 10 ms
```

* **Buffers**: two `String` buffers (`buffers[2]`). Consider switching to fixed arrays for long‑term runs.
* **RTOS**: adjust stack size or task priorities in `xTaskCreatePinnedToCore()`.

---

## ❗ Troubleshooting

* **SD card mount failed**: confirm wiring, CS pin, and SD format (FAT16/32).
* **Heap crashes or fragmentation**: switch from `String` to fixed‑size char buffers.
* **Timing drift**: use `vTaskDelayUntil()` instead of busy‑wait loops.
* **Concurrent writes**: protect `writeIndex` with FreeRTOS mutex or critical sections.

---

## 🤝 Contributing

Contributions are welcome!

1. Fork the repo.
2. Create a feature branch (`git checkout -b feature/new-task`).
3. Commit your changes (`git commit -m "Add ..."`).
4. Push (`git push origin feature/new-task`).
5. Open a Pull Request.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

* Inspired by FreeRTOS examples and the [HX711 library](https://github.com/bogde/HX711).
* Thanks to the ESP32 community and Arduino ecosystem.
