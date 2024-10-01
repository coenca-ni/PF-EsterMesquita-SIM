
# Explicação do Código Receptor LoRa

## Bibliotecas Importadas

O código começa importando as bibliotecas necessárias para conectar ao Wi-Fi, ThingSpeak, Firebase, e o módulo LoRa.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ThingSpeak.h"
#include <DNSServer.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "EBYTE.h"
```

- **WiFi.h**: Para conectar o ESP32 a redes Wi-Fi.
- **ThingSpeak.h**: Para enviar dados ao serviço de armazenamento de dados ThingSpeak.
- **Firebase_ESP_Client.h**: Para integração com o banco de dados Firebase.
- **EBYTE.h**: Usada para controlar o módulo LoRa E32.

## Definições de Pinos e Estruturas

Aqui, os pinos usados pelo LoRa e os LEDs são definidos. Também são criadas estruturas para armazenar os pacotes recebidos e possíveis comandos de reinicialização.

```cpp
#define RX 16   // Pino RX do ESP32 (conecte ao pino TX do EBYTE)
#define TX 17   // Pino TX do ESP32 (conecte ao pino RX do EBYTE)
#define M0 25   // Pino M0 do EBYTE
#define M1 22   // Pino M1 do EBYTE
#define AX 21   // Pino AUX do EBYTE
#define PINO_RESET_ARDUINO 33 // Pino do ESP32 conectado ao RST do Arduino Nano

#define VERMELHO_PIN 27
#define VERDE_PIN 14
#define AZUL_PIN 13

struct PACOTE {
  unsigned long Contador;
  float Pressao;
  float Temp;
  float Umid;
  bool Chuva;
  byte IndiceUV;
  unsigned long tempoEnvio;
};

struct RESET{
  bool resetArduino; // Variável para resetar o Arduino
};

PACOTE dados;
RESET reinicia;
EBYTE Lora(&Serial2, M0, M1, AX);
```

- **RX, TX**: Pinos para comunicação serial com o módulo LoRa.
- **PINO_RESET_ARDUINO**: Pino de reset do Arduino conectado ao ESP32.
- **VERMELHO_PIN, VERDE_PIN, AZUL_PIN**: Pinos para controle de LEDs RGB.
- **PACOTE**: Estrutura que armazena os dados recebidos, incluindo temperatura, umidade, chuva e pressão.
- **EBYTE Lora**: Inicializa a biblioteca para o módulo LoRa.

## Função `setup()`

Na função `setup()`, os módulos de comunicação e o Wi-Fi são configurados. O ESP32 tenta conectar-se ao Firebase e ThingSpeak para armazenamento dos dados recebidos.

```cpp
void setup() {
  reinicia.resetArduino = 0;

  pinMode(PINO_RESET_ARDUINO, OUTPUT);
  digitalWrite(PINO_RESET_ARDUINO, HIGH);
  
  pinMode(VERMELHO_PIN, OUTPUT);
  pinMode(VERDE_PIN, OUTPUT);
  pinMode(AZUL_PIN, OUTPUT);

  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RX, TX);  

  // Configura WiFi e ThingSpeak
  wm.setConfigPortalTimeout(240);
  if (!wm.autoConnect("Projeto SIM", "projetosim#123")) {
    Serial.println(F("Falha na conexão WiFi. Reiniciando..."));
    definirCor(255, 0, 0);
    delay(3000);
    ESP.restart();
  }

  // Configura Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  ThingSpeak.begin(client);

  // Configura o LoRa
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
  Lora.PrintParameters();  
}
```

- **Firebase.begin()**: Inicia a conexão com o Firebase.
- **ThingSpeak.begin()**: Configura a comunicação com o ThingSpeak.
- **Lora.init()**: Inicializa o módulo LoRa com parâmetros como endereço, baud rate, velocidade de transmissão, e canal de comunicação.

## Configuração LoRa

Abaixo estão os parâmetros configurados para o módulo LoRa e suas explicações:

```cpp
Lora.SetAddressH(1);   // Configura o endereço alto do dispositivo para 1.
Lora.SetAddressL(0);   // Configura o endereço baixo do dispositivo para 0.
Lora.SetUARTBaudRate(3);  // Configura a taxa de baud da UART para 9600 bps.
Lora.SetSpeed(0b010);   // Configura a velocidade de comunicação para 2.4 kbps.
Lora.SetTransmissionMode(0);  // Define o modo de transmissão contínuo.
Lora.SetAirDataRate(2);   // Configura a taxa de dados no ar para 2 kbps.
Lora.SetChannel(53);   // Define o canal de comunicação para 915 MHz.
Lora.SetAddress(1);    // Define o endereço do dispositivo para 1.
Lora.SaveParameters(PERMANENT);  // Salva as configurações de forma permanente.
```

## Função `loop()`

A função `loop()` monitora a recepção dos pacotes via LoRa, faz cálculos de latência e taxa de perda de pacotes, e envia os dados para o Firebase e ThingSpeak.

```cpp
void loop() {
  Serial.println("Aguardando sinal LoRa...");
  delay(1000);
  
  if(Serial2.available()){
    definirCor(128, 0, 128);
    bool recebido = Lora.GetStruct(&dados, sizeof(dados));

    // Sincroniza tempos e calcula latência
    tempoRecebimento = millis();  
    diferencaTempo = tempoRecebimento - dados.tempoEnvio;
    latencia = (tempoRecebimento - diferencaTempo) - dados.tempoEnvio;
    pacotesRecebidos++;
    taxaPerdaPacotes = ((float)(dados.Contador - pacotesRecebidos) / (float)dados.Contador) * 100;
    qualidadeTransmissao = 100.0 - taxaPerdaPacotes;

    // Envia os dados para Firebase e ThingSpeak
    enviarParaFirebase();
    enviarParaThingSpeak();
  }
}
```

- **Lora.GetStruct()**: Recebe a estrutura de dados enviada pelo transmissor via LoRa.
- **latencia**: Calcula a latência entre a transmissão e recepção dos dados.
- **taxaPerdaPacotes**: Calcula a taxa de perda de pacotes com base no contador de pacotes.

