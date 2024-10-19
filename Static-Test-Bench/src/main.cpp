/*Developed by: Emanoel Henmerson Cavalcante Soares
  Avionics Manager - Kosmos Rocketry

  Letícia Gabriely Witt
  Research and Development Manager - Kosmos Rocketry
  
  Wagner Ferreira Barbosa Junior
  Research and Development Analyst - Kosmos Rocketry

  Tayná da Silva Rosa
  Research and Development Analyst - Kosmos Rocketry

  Eric Marcel de Andrade Bliesener 
  Acrux Aerospace Technologies
  07/06/2024 */
  

#include "HX711.h"   // Biblioteca para o sensor de carga HX711
#include "FS.h"      // Biblioteca do sistema de arquivos
#include "SD.h"      // Biblioteca para manipulação do cartão SD
#include <SPI.h>     // Biblioteca SPI para comunicação

// Definição dos pinos de comunicação do cartão SD - CS=5  MOSI = 23, sck = 18, MISO = 19;
#define p_SD 5

// Definição dos pinos para o sensor de carga HX711
const int LOADCELL_DOUT_PIN = 2;  // Pino de saída de dados do HX711
const int LOADCELL_SCK_PIN = 32;  // Pino de clock do HX711

float pressao_sd;
float pressao_convertida;

HX711 scale; // Criação de uma instância para o sensor de carga

String dataMessage; // String para armazenar os dados a serem gravados no SD

//configurações transdutor no pino 35
#define transdutor 34
#define avg_n 50

unsigned long ti;
unsigned long tf;
unsigned long delta_t;

// Função para criar um diretório no cartão SD
void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

// Função para adicionar dados ao final de um arquivo no cartão SD
void appendFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND); // Abre o arquivo no modo de adição
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// Função para ler o conteúdo de um arquivo do cartão SD
void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

struct result //Transdutor
{
  float avg;
  float std_dev;
};

result avg_read()//Transdutor
{
  float avg=0;
  float leitura[avg_n];
  float vari=0;
  uint8_t i=0;
  for (i=0; i<avg_n-1; i++) leitura[i] = static_cast <float> (analogRead(transdutor));
  for (i=0; i<avg_n-1; i++) avg += leitura[i];
  avg /= avg_n;
  for (i=0; i<avg_n-1; i++) vari += (leitura[i]-avg)*(leitura[i]-avg);
  return {avg, sqrt(vari/avg_n)};
}

float transductorFuncao()
{
  ti =  micros();
  result media = avg_read();
  tf = micros();
  //leitura = map(sensor, 600, 4095, 0, 10000000); //Conversão para Pa usando como base 600 = 101325 (1 atm) regra de tres (min esp, max esp, min sensor, max do sensor)
  //leitura = (sensor-600)/30;
  Serial.printf ("Media: %f \nDesvio Padrao: %f \n" , media.avg, media.std_dev);
  delta_t =  tf-ti;
  Serial.printf ("Tempo para %d leituras: %d microssegundos", avg_n, delta_t);
  //Serial.println(" Bar");
  while (micros()-ti < 15000){}
  Serial.println("\n\n-----------\n");
  return media.avg;
}
// Função de configuração inicial do sistema
void setup() {  

  Serial.begin(115200); // Inicializa a comunicação serial a 9600 bps

  Serial.println("Teste SD CARD");

  // Verifica se o cartão SD foi montado corretamente
  if (!SD.begin(p_SD)) {
    Serial.println("Card Mount Failed");
  } else {
    Serial.println("Encontrou o modulo");
  }

  uint8_t cardType = SD.cardType();

  // Verifica o tipo do cartão SD
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
  }else {
    Serial.println("Encontrou o sd card");
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  // Calibração da célula de carga
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  /*----------------------------------------------------------------------------------------------------------------
  --------------------------------------------------------------------------------------------------*/
  scale.set_scale(25499.0/6.098f);  // Define o fator de escala para o sensor de carga após a calibração 26409.0/6.098f
  scale.tare();			// Define o peso inicial para zero

  // Exibe informações de calibração do sensor de carga
  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());               // Leitura direta do ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));     // Média de 20 leituras do ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));		    // Média de 5 leituras menos o peso inicial

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(10), 1);     // Conversão da média de 10 leituras para unidades de peso

  Serial.println("Readings:");
    
  // Grava o cabeçalho do arquivo no cartão SD
  appendFile(SD, "/data.txt", "NEWTON, Corrente, MPa, tempo \n");

  //Definição transdutor
  pinMode(transdutor, INPUT);
}

// Loop principal do código que será executado repetidamente
void loop() {

  ti =  micros();

  pressao_sd = transductorFuncao(); 
  pressao_convertida = (pressao_sd-590)/300; //calibracao leticia

  // Montagem da string de dados com os valores lidos --alteração * 9.8
  dataMessage = String(scale.get_units(1) * 9.8, 1) + " , " + String(pressao_sd) + " , " + String(pressao_convertida) + " , " + String(millis()) + "\r\n";
  Serial.println(dataMessage); // Exibe os dados na serial


  // Grava os dados no arquivo do cartão SD
  appendFile(SD, "/data.txt", dataMessage.c_str());

  //MUDAR NA HORA DO TESTE 150000 -> 15000
  while (micros()-ti < 15000){} // Aguarda 15ms antes de realizar a próxima leitura
}
