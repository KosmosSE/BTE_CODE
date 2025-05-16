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

// Função que faz uma só vez a abertura do arquivo e escreve dados (com debug)
void appendToSD(const String &data) {
  static File file;
  static bool opened = false;
  if (!opened) {
    file = SD.open("/data_teste_morpheus.csv", FILE_APPEND);
    if (!file) {
      Serial.println("[appendToSD] Falha ao abrir o arquivo para append");
    } else {
      Serial.println("[appendToSD] Arquivo aberto com sucesso para append");
      opened = true;
    }
  }
  if (opened) {
    size_t written = file.print(data);
    if (written == 0) {
      Serial.println("[appendToSD] Falha ao escrever no arquivo");
    } else {
      Serial.printf("[appendToSD] Escreveu %u bytes\n", written);
    }
    file.flush(); // força escrita imediata
  }
}

// Task que fica no Core 1 e grava buffers quando sinalizados
void sdWriteTask(void *param) {
  unsigned long start_us, end_us;
  while (true) {
    if (writeIndex >= 0) {
      start_us = micros();
      // Serial.printf("[SD Task] Gravando buffer %d em %lu us\n", writeIndex, start_us);
      appendToSD(buffers[writeIndex]);
      end_us = micros();
      // Serial.printf("[SD Task] Tempo de gravação do buffer %d: %lu us\n", writeIndex, end_us - start_us);
      buffers[writeIndex].clear();
      writeIndex = -1;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // rende
  }
}

// Task que roda no Core 0, coletando dados e alternando buffers
void collectData() {
  unsigned long start_us, end_us;
  int bufIndex = 0;
  buffers[0] = "";
  buffers[1] = "";

  while (true) {
    start_us = micros();
    // Serial.printf("[Collect Task] Iniciando buffer %d em %lu us\n", bufIndex, start_us);

    // Preenche o buffer atual
    for (int i = 0; i < ciclos_coleta; i++) {
      unsigned long sample_us = micros();

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

      // Garante intervalo de 13 ms
      while (micros() - sample_us < intervalo_us) { }
    }

    end_us = micros();
    // Serial.printf("[Collect Task] Finalizou buffer %d em %lu us (duração: %lu us)\n",
    //               bufIndex, end_us, end_us - start_us);

    // Sinaliza ao SD task para gravar este buffer
    writeIndex = bufIndex;
    // Alterna buffer
    bufIndex = 1 - bufIndex;
    buffers[bufIndex].clear();
  }
}

void setup() {
  Serial.begin(115200);

  if (!SD.begin(SD_CS_PIN)) {
    // Serial.println("Card Mount Failed");
    while (1);
  }

  // Configura HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(25772.0 / 6.098f);
  scale.tare();

  // Remove e recria header do CSV

  File header = SD.open("/data_teste_morpheus.csv", FILE_WRITE);
  if (header) {
    header.print("peso_kg,peso_N,pressao_sd,pressao_conv,tempo\n");
    header.close();
    // Serial.println("[setup] Header escrito e arquivo fechado");
  } else {
    // Serial.println("[setup] Falha ao criar arquivo de header");
  }

  // Cria task de escrita no Core 1
  xTaskCreatePinnedToCore(
    sdWriteTask,       // função
    "SD Write Task",   // nome
    4096,              // stack
    NULL,              // parâmetro
    1,                 // prioridade
    &sdTaskHandle,     // handle
    1                  // core 1
  );

  // Inicia coleta no Core 0 (função não retorna)
  collectData();
}

void loop() {
  // tudo é feito nas tasks; loop vazio
}
