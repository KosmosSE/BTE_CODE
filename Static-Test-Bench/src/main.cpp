/*Developed by: Emanoel Henmerson Cavalcante Soares
  Avionics Manager - Kosmos Rocketry
  
  Wagner Ferreira Barbosa Junior
  Research and Development Analyst - Kosmos Rocketry

  Tayná da Silva Rosa
  Research and Development Analyst - Kosmos Rocketry
 */

#include "HX711.h"
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SD_CS_PIN           5
#define LOADCELL_DOUT_PIN   2
#define LOADCELL_SCK_PIN    32
#define TRANSDUTOR_PIN      34

const int ciclos_coleta        = 120;
const unsigned long intervalo_us = 10000; // 10 ms

HX711 scale;

// Dois buffers em RAM e índice compartilhado
static String buffers[2];
volatile int writeIndex = -1;

TaskHandle_t sdTaskHandle = NULL;
String nomeArquivo; // armazena o nome do arquivo atual

// Função que abre o arquivo apenas uma vez e escreve dados (com flush)
void appendToSD(const String &data) {
  static File file;
  static bool opened = false;
  if (!opened) {
    file = SD.open(nomeArquivo.c_str(), FILE_APPEND);
    if (!file) {
      Serial.println("[appendToSD] Falha ao abrir o arquivo para append");
      return;
    }
    opened = true;
  }
  size_t written = file.print(data);
  if (written == 0) {
    Serial.println("[appendToSD] Falha ao escrever no arquivo");
  }
  file.flush(); // força escrita imediata
}

// Task que roda no Core 1 e grava buffers quando sinalizado
void sdWriteTask(void *param) {
  while (true) {
    if (writeIndex >= 0) {
      appendToSD(buffers[writeIndex]);
      buffers[writeIndex].clear();
      writeIndex = -1;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // rendimento
  }
}

// Task que roda no Core 0, coletando dados e alternando buffers
void collectData() {
  int bufIndex = 0;
  buffers[0] = "";
  buffers[1] = "";

  while (true) {
    for (int i = 0; i < ciclos_coleta; i++) {
      unsigned long start_us = micros();

      float pressao_sd   = analogRead(TRANSDUTOR_PIN);
      float pressao_conv = (pressao_sd - 602) / 94.73684210526;
      float peso_kg      = scale.get_units(1);
      float peso_N       = peso_kg * 9.81;
      unsigned long ts   = millis();

      buffers[bufIndex] +=
        String(peso_kg)      + "," +
        String(peso_N)       + "," +
        String(pressao_sd)   + "," +
        String(pressao_conv) + "," +
        String(ts)           + "\n";

      // Garante intervalo de 10 ms
      while (micros() - start_us < intervalo_us) {}
    }

    // sinaliza para a task de SD gravar este buffer
    writeIndex = bufIndex;
    bufIndex = 1 - bufIndex;
    buffers[bufIndex].clear();
  }
}

void setup() {
  Serial.begin(115200);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    while (1);
  }

  // Gera nome de arquivo incremental: /data1.csv, /data2.csv, ...
  int fileIndex = 1;
  do {
    nomeArquivo = "/data_BTE_MORPHEUS" + String(fileIndex) + ".csv";
    fileIndex++;
  } while (SD.exists(nomeArquivo.c_str()));
  Serial.println("[setup] Novo arquivo: " + nomeArquivo);

  // Escreve header no novo arquivo
  File header = SD.open(nomeArquivo.c_str(), FILE_WRITE);
  if (header) {
    header.print("peso_kg,peso_N,pressao_sd,pressao_conv,tempo\n");
    header.close();
  } else {
    Serial.println("[setup] Falha ao criar arquivo de header");
  }

  
  // Inicializa HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(23293.00 / 6.098f);
  scale.tare();
  Serial.println("Coloque o peso de calibração na balança");
  delay(10000); 
  float load_cell_calibration = scale.get_units(20); // leitura inicial para estabilizar
  Serial.println(load_cell_calibration);
  // Cria task de escrita no Core 1
  xTaskCreatePinnedToCore(
    sdWriteTask,
    "SD Write Task",
    4096,
    NULL,
    1,
    &sdTaskHandle,
    1  // core 1
  );

  // Inicia coleta no Core 0 (essa chamada não retorna)
  collectData();
}

void loop() {
  // tudo é feito nas tasks; loop vazio
}
