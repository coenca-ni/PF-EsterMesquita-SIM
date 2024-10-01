
# Explicação do Código Transmissor LoRa

## Bibliotecas Importadas

O código começa importando as bibliotecas necessárias para os sensores e a comunicação com o módulo LoRa:

```cpp
#include <SoftwareSerial.h>
#include "EBYTE.h"
#include "DHT.h"
#include <Adafruit_BMP085.h> //BMP180
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
```

- **SoftwareSerial.h**: Permite a comunicação serial em pinos digitais, essencial para o módulo LoRa.
- **EBYTE.h**: Biblioteca usada para controlar o módulo LoRa E32.
- **DHT.h**: Usada para leitura de temperatura e umidade do sensor DHT22.
- **Adafruit_BMP085.h**: Para comunicação com o sensor de pressão BMP180.

## Definições de Pinos e Estruturas

Aqui, os pinos dos sensores e do módulo LoRa são definidos. Também são criadas as estruturas para os pacotes de dados que serão enviados e possíveis comandos de reinicialização.

```cpp
#define DHTPINO 7
#define DHTMODELO DHT22
DHT dht(DHTPINO, DHTMODELO);

#define sensorChuva 8
const int pinoSensorChuva = sensorChuva;

#define pinSensorUV A0
int leituraUV = 0;
byte indiceUV = 0;

#define RX 2
#define TX 3
#define M0 4
#define M1 5
#define AX 6

struct PACOTE {
  unsigned long Contador;
  float Pressao;
  float Temp;
  float Umid;
  bool Chuva;
  byte IndiceUV;
  unsigned long tempoEnvio;
};

struct RESET {
  bool resetArduino;
};

PACOTE dados;
RESET reinicia;

SoftwareSerial SerialLora(RX, TX);
EBYTE Lora(&SerialLora, M0, M1, AX);
```

- **DHTPINO (7)**: Define o pino ao qual o sensor DHT22 está conectado.
- **sensorChuva (8)**: Define o pino para o sensor de chuva.
- **pinSensorUV (A0)**: Pino analógico para o sensor de radiação UV.
- **PACOTE**: Estrutura que define os dados a serem enviados, como contador, pressão, temperatura, umidade, chuva, índice UV, e tempo de envio.
- **SerialLora**: Configura a comunicação serial nos pinos RX e TX para o módulo LoRa.
- **EBYTE Lora**: Inicializa a biblioteca para o módulo LoRa com os pinos de controle M0, M1 e AX.

## Função `setup()`

Na função `setup()`, os sensores e o módulo LoRa são configurados para operação, e as conexões são verificadas.

```cpp
void setup() {
  Serial.begin(9600);
  SerialLora.begin(9600);

  if (!bmp.begin()) {
    Serial.println("Sensor BMP180 não foi identificado! Verifique as conexões.");
    while(1);
  }

  Serial.println("Iniciando envio...");

  Lora.init();
  Lora.SetAddressH(1);
  Lora.SetAddressL(0);
  Lora.SetUARTBaudRate(3);
  Lora.SetSpeed(0b010);
  Lora.SetTransmissionMode(0);
  Lora.SetAirDataRate(2);
  Lora.SetChannel(53);
  Lora.SetAddress(1);
  Lora.SaveParameters(PERMANENT);

  dht.begin();
  pinMode(pinoSensorChuva, INPUT); 
  pinMode(pinSensorUV, INPUT);
}
```

- **Serial.begin(9600)**: Inicia a comunicação serial com o monitor serial.
- **SerialLora.begin(9600)**: Inicia a comunicação serial com o módulo LoRa.
- **bmp.begin()**: Verifica se o sensor BMP180 foi inicializado corretamente. Caso contrário, entra em um loop infinito.
- **Lora.init()**: Inicializa o módulo LoRa com os parâmetros como endereço, baud rate da UART, velocidade de comunicação, canal de transmissão (915 MHz), e salva as configurações permanentemente.
- **dht.begin()**: Inicializa o sensor DHT22.
- **pinMode(pinoSensorChuva, INPUT)**: Configura o pino do sensor de chuva como entrada.
- **pinMode(pinSensorUV, INPUT)**: Configura o pino do sensor UV como entrada.

## Configuração LoRa

Abaixo estão os parâmetros configurados para o módulo LoRa e suas explicações:

```cpp
Lora.SetAddressH(1);   // Configura o endereço alto do dispositivo para 1 (parte do identificador do dispositivo na rede).
Lora.SetAddressL(0);   // Configura o endereço baixo do dispositivo para 0 (parte do identificador do dispositivo na rede).
Lora.SetUARTBaudRate(3);  // Configura a taxa de baud da UART para 9600 bps (comunicação serial com o módulo LoRa).
Lora.SetSpeed(0b010);   // Configura a velocidade de comunicação para 2.4 kbps (taxa de transmissão de dados via LoRa).
Lora.SetTransmissionMode(0);  // Define o modo de transmissão para contínuo (transmissão em tempo real).
Lora.SetAirDataRate(2);   // Configura a taxa de dados no ar para 2 kbps (velocidade de transmissão via rádio).
Lora.SetChannel(53);   // Define o canal de comunicação para 915 MHz (frequência utilizada para a transmissão).
Lora.SetAddress(1);    // Define o endereço do dispositivo para 1 (endereço exclusivo para identificação na rede).
Lora.SaveParameters(PERMANENT);  // Salva as configurações de forma permanente no módulo LoRa.
```

Essas configurações garantem que o dispositivo transmita corretamente os dados entre o transmissor e o receptor LoRa.

## Função `loop()`

A função principal onde ocorre a leitura dos sensores e o envio de dados via LoRa. O código dentro do `loop()` realiza leituras e envia os dados estruturados.

```cpp
void loop() {
  dados.Contador++;
  delay(100);

  float temperatura = dht.readTemperature();
  dados.Temp = temperatura;

  float umidade = dht.readHumidity();
  dados.Umid = umidade;

  if (digitalRead(pinoSensorChuva) == LOW) {
    dados.Chuva = 1;
  } else {
    dados.Chuva = 0;
  }

  pressao = bmp.readPressure() / 100;
  dados.Pressao = pressao;

  leituraUV = analogRead(pinSensorUV);
  indiceUV = map(leituraUV, 0, 203, 0, 10);
  dados.IndiceUV = indiceUV;

  dados.tempoEnvio = millis();
  Lora.SendStruct(&dados, sizeof(dados));

  Serial.print("Temperatura enviada: ");
  Serial.println(temperatura);

  Serial.print("Umidade enviada: ");
  Serial.println(umidade);

  Serial.print("Chuva enviada: ");
  Serial.println(dados.Chuva);

  Serial.print("Pressão: ");
  Serial.print(pressao);
  Serial.println(" hPa");

  Serial.print("Indice UV: ");
  Serial.println(dados.IndiceUV);

  Serial.print("Enviando pacote número: ");
  Serial.println(dados.Contador);

  Serial.print("Tempo de envio (ms): ");
  Serial.println(dados.tempoEnvio);

  Lora.GetStruct(&reinicia, sizeof(reinicia));
  if (reinicia.resetArduino == 1) {
    Serial.println("Recebido comando de reset! Resetando Arduino...");
    delay(1000);
    Reset();
    dados.Contador = 0;
  }

  delay(300000);
}
```

- **dados.Contador++**: Incrementa o contador de pacotes enviados.
- **dht.readTemperature() e dht.readHumidity()**: Lê os valores de temperatura e umidade do sensor DHT22.
- **digitalRead(pinoSensorChuva)**: Lê o valor do sensor de chuva (LOW indica presença de chuva).
- **bmp.readPressure()**: Lê o valor da pressão atmosférica do sensor BMP180, convertendo para hPa.
- **analogRead(pinSensorUV)**: Lê o valor do sensor de UV e o converte para o índice UV.
- **dados.tempoEnvio = millis()**: Armazena o tempo em milissegundos desde que o Arduino foi iniciado.
- **Lora.SendStruct()**: Envia a estrutura de dados via LoRa.
- **Lora.GetStruct()**: Verifica se um comando de reset foi recebido. Caso positivo, o Arduino é reiniciado.
