 /*
  Código Transmissor de Temperatura LoRa

  Conexões LoRa
  Módulo     Arduino Nano
  M0          4
  M1          5
  Rx          2 (TX do Lora E32)
  Tx          3 (RX do Lora E32)
  Aux         6
  Vcc         5v
  Gnd         Gnd


  Sensor    Arduino Nano
  DHT22       7

*/

//Bibliotecas
#include <SoftwareSerial.h>
#include "EBYTE.h"
#include "DHT.h"
#include <Adafruit_BMP085.h> //BMP180
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>



//DHT22
#define DHTPINO 7 // Pino ao qual o sensor DHT22 está conectado
#define DHTMODELO DHT22
DHT dht(DHTPINO, DHTMODELO);

//Chuva
#define sensorChuva 8
const int pinoSensorChuva = sensorChuva;


//Radiaçao
#define pinSensorUV A0
int leituraUV=0; // VARIÁVEL PARA ARMAZENAR A LEITURA DA PORTA ANALÓGICA
byte indiceUV=0; // VARIÁVEL PARA ARMAZENAR A CONVERSÃO PARA INDICE UV

//Lora E32
#define RX 2
#define TX 3
#define M0 4
#define M1 5
#define AX 6

//Definição da Estrutura que será enviada
struct PACOTE {
  unsigned long Contador;
  float Pressao;
  float Temp;
  float Umid;
  bool Chuva;
  byte IndiceUV;
  unsigned long tempoEnvio;
};

//Estrutura para armazenar e enviar dados vindos do Firebase para resetar o sistema
struct RESET{
  bool resetArduino; // Variável para resetar o Arduino
};

//Define variável dados que serão enviados
PACOTE dados;
RESET reinicia;

//Definindo Serial Lora 
SoftwareSerial SerialLora(RX, TX);

//Chama bilioteca EBYTE, inserindo os parâmetros do Lora
EBYTE Lora(&SerialLora, M0, M1, AX);

//BMP 180
Adafruit_BMP085 bmp;//(sensorPressaoSLC, sensorPressaoSDA); //OBJETO DO TIPO Adafruit_BMP085 (I2C)

//Função reset
void (*Reset)() = 0;

float pressao;

void setup() {

  //Iniciando os "Monitores Seriais"
  Serial.begin(9600);
  SerialLora.begin(9600);

  //BMP180
  if (!bmp.begin()){ //SE O SENSOR NÃO FOR INICIALIZADO, FAZ
  Serial.println("Sensor BMP180 não foi identificado! Verifique as conexões."); //IMPRIME O TEXTO NO MONITOR SERIAL
  while(1); //SEMPRE ENTRE NO LOOP
  }

  //Mensagem no Serial Monitor
  Serial.println("Iniciando envio...");

  //Configuração dos parâmetros de conectividade
  Lora.init();
  Lora.SetAddressH(1);  // Configura o endereço alto do LoRa
  Lora.SetAddressL(0);  // Configura o endereço baixo do LoRa
  Lora.SetUARTBaudRate(3); // Configura a taxa de baud da UART
  Lora.SetSpeed(0b010);  // Configura a velocidade de comunicação
  Lora.SetTransmissionMode(0); // Configura o modo de transmissão
  Lora.SetAirDataRate(2);  // Configura a taxa de dados no ar
  Lora.SetChannel(53); //Configura o canal de comunicação 915Mhz
  Lora.SetAddress(1); // Define o endereço do dispositivo como 1
  Lora.SaveParameters(PERMANENT); // Salva as configurações permanentemente
 

  //Salva os parâmetros
  Lora.SaveParameters(PERMANENT);

  //Mostra os parâmetros no Serial Monitor
  Lora.PrintParameters();

  //Inicia do Sensor DHT22
  dht.begin();

  //Define Sensor de Chuva como entrada
  pinMode(pinoSensorChuva, INPUT); 

  //Define sensor UV
  pinMode(pinSensorUV, INPUT);

}

void loop() {

  //Definindo as variáveis da Estrutura
  dados.Contador++; //Contador
  delay(100); //Tempo para leitura

  //Temperatura
  float temperatura = dht.readTemperature(); //Leitura do Sensor
  dados.Temp = temperatura; //Temperatura lida atribuida à variável do Pacote de Dados

  //Umidade
  float umidade = dht.readHumidity();
  dados.Umid = umidade;

  //Chuva 
  if(digitalRead(pinoSensorChuva) == LOW){ //SE A LEITURA DO PINO FOR IGUAL A LOW, FAZ
    dados.Chuva = 1;
  }else{ //SENÃO, FAZ
    dados.Chuva = 0;
  }

  //Pressão
  pressao =  bmp.readPressure()/100;
  dados.Pressao = pressao;

  //Radiação UV
  leituraUV = analogRead(pinSensorUV); // REALIZA A LEITURA DA PORTA ANALÓGICA 
  indiceUV = map(leituraUV, 0,203,0,10) ; // CONVERTE A FAIXA DE SINAL DO SENSOR DE 0V A 1V PARA O INDICE UV DE 0 A 10.
  dados.IndiceUV = indiceUV;

  // Cap  tura o tempo antes de enviar
  
  dados.tempoEnvio = millis();  // Tempo em milissegundos desde que o sistema foi ligado

  //Envia a Estrutura (Pacote)
  Lora.SendStruct(&dados, sizeof(dados));

  //Mostra a temperatura enviada
  Serial.print("Temperatura enviada: ");
  Serial.println(temperatura);
  
  //Mostra a umidade enviada
  Serial.print("Umidade enviada: ");
  Serial.println(umidade);

   
  //Mostra status Chuva enviada
  Serial.print("Chuva enviada: ");
  Serial.println(dados.Chuva);

  //Mostra Pressão enviada
  Serial.print("Pressão: "); //IMPRIME O TEXTO NO MONITOR SERIAL
  Serial.print(pressao); //IMPRIME NO MONITOR SERIAL A PRESSÃO
  Serial.println(" hPa"); //IMPRIME O TEXTO NO MONITOR SERIAL

  //Mostra Pressão enviada
  Serial.print("Indice UV: "); //IMPRIME O TEXTO NO MONITOR SERIAL
  Serial.println(dados.IndiceUV); //IMPRIME NO MONITOR SERIAL O INDICE UV
 
  
  //Mostra o número do pacote que está enviando
  Serial.print("Enviando pacote número: "); 
  Serial.println(dados.Contador);

  Serial.print("Tempo de envio (ms): "); 
  Serial.println(dados.tempoEnvio);

  Serial.println("Enviado!");

    // Verifica se há um comando de reset recebido
  Lora.GetStruct(&reinicia, sizeof(reinicia));
  if (reinicia.resetArduino == 1) {
    Serial.println("Recebido comando de reset! Resetando Arduino...");
    delay(1000);
    Reset(); // Comando para reiniciar o Arduino
    dados.Contador=0;
  }

  //delay(60000); //Envio de 1 em 1 minuto
  delay(300000); //Envio de 5 em 5 minuto
  


}