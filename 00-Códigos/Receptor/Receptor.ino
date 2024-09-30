/*
  Código Receptor de Temperatura LoRa - ESP32

  Conxões LoRa:
  Módulo     ESP32
  M0          35
  M1          22
  Rx          16 (TX do LoRa E32)
  Tx          17 (RX do LoRa E32)
  Aux         21
  Vcc         3.3V -> externa
  Gnd         Gnd -> externa
*/

// Inclui as bibliotecas necessárias
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



// Configurações do Firebase
#define API_KEY "AIzaSyB1gRscAV5YV-_p9aF7hLHGzbiuoFmKSVM" // Chave API do Firebase
#define DATABASE_URL "https://simbora-cb6c1-default-rtdb.firebaseio.com/" // URL do banco de dados

// Inicializa objetos do Firebase e variáveis auxiliares
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
String resetWifi, resetSistema, temperaturaString, umidadeString, statusTemperatura, statusUmidade, statusChuva, Pressao, statusUV, contador, pacote, statusQualidade, txPerda, qTransmissao, stringChuva;


// Configuração dos pinos do LoRa E32
#define RX 16   // Pino RX do ESP32 (conecte ao pino TX do EBYTE)
#define TX 17   // Pino TX do ESP32 (conecte ao pino RX do EBYTE)
#define M0 25   // Pino M0 do EBYTE
#define M1 22   // Pino M1 do EBYTE
#define AX 21   // Pino AUX do EBYTE
#define PINO_RESET_ARDUINO 33 // Pino do ESP32 conectado ao RST do Arduino Nano

// Configuração dos pinos do LED RGB
#define VERMELHO_PIN 27
#define VERDE_PIN 14
#define AZUL_PIN 13

// Estrutura para armazenar os dados recebidos
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




// Declarção da variável global 'dados' do tipo 'PACOTE'
PACOTE dados;
RESET reinicia;


// Cria um objeto LoRa para comunicação
EBYTE Lora(&Serial2, M0, M1, AX);



//Define variável aceitável de leitura
float variacaoAceitavel = 5.0;
float tempminAceitavel =  0.0;
float tempmaxAceitavel = 50.0;

// Configurações do ThingSpeak
WiFiClient client;
unsigned long meuNumeroCanal = 1; // Número do canal do ThingSpeak
const char *minhaChaveEscritaAPI = "DVYHJ95R7M9R68FP"; // Chave API do ThingSpeak

// Variáveis para monitorar perda de pacotes
int ultimoContadorRecebido = 0;
unsigned long pacotesPerdidos = 0;
unsigned long pacotesRecebidos = 0;
// Variável para guardar a diferença de tempo entre transmissor e receptor
unsigned long diferencaTempo = 0;
bool sincronizado = false;
float taxaPerdaPacotes = 0.0;
float qualidadeTransmissao;
unsigned long tempoRecebimento;
unsigned long latencia;
float latenciaFloat;
int buscadados = 0; //get struct do firebase 
int qtdbuscaAceitavel = 5; //após tentar normalizar a leitura X vezes e não conseguir


// Função para definir a cor do LED RGB
void definirCor(int vermelho, int verde, int azul) {
  analogWrite(VERMELHO_PIN, vermelho);
  analogWrite(VERDE_PIN, verde);
  analogWrite(AZUL_PIN, azul);
}

// Função para apagar o LED
void apagarLED() {
  definirCor(0, 0, 0);
}

// Função para piscar o LED
void piscarLED(int vermelho, int verde, int azul, int tempo) {
  definirCor(vermelho, verde, azul);
  delay(tempo);
  apagarLED();
  delay(tempo);
}

// Declaração dos protótipos das funções
void enviarParaFirebase();
void enviarParaThingSpeak();
void reiniciaSistema();

//Inicia configuração WifiManager
WiFiManager wm;


void setup() {
  //Inicia a variável resetArduino com nível lógico baixo
  reinicia.resetArduino = 0;

  pinMode(PINO_RESET_ARDUINO, OUTPUT); // Configura o pino como saída
  digitalWrite(PINO_RESET_ARDUINO, HIGH); // Mantém o Arduino Nano fora do estado de reset
  
  // Configura os pinos RGB como saí­da
  pinMode(VERMELHO_PIN, OUTPUT);
  pinMode(VERDE_PIN, OUTPUT);
  pinMode(AZUL_PIN, OUTPUT);

  // Inicia a comunicação serial
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RX, TX);  // Configura a Serial2 para comunicação com o LoRa E32

  // Configuração do WiFi
 
  wm.setConfigPortalTimeout(240);  // Tempo de espera para configuração do portal

  // Inicia a conexão WiFi piscando o LED verde
  for (int i = 0; i < 10; i++) {
    piscarLED(0, 255, 0, 500); // Verde piscando (conectando ao WiFi)
  }

  // Tenta conectar ao WiFi
  if (!wm.autoConnect("Projeto SIM", "projetosim#123")) {
    Serial.println(F("Falha na conexão WiFi. Reiniciando..."));
    definirCor(255, 0, 0); // LED Vermelho direto (sem conexão WiFi)
    delay(3000);
    ESP.restart();
  }

  // Se a conexão WiFi for bem-sucedida
  Serial.println(F("Conectado na rede WiFi com sucesso."));
  Serial.print(F("Endereçoo IP: "));
  Serial.println(WiFi.localIP());
  apagarLED();
  definirCor(0, 255, 0); // Verde contí­nuo (conectado ao WiFi)

  // Inicializa o Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println(F("Conectado ao Firebase com sucesso."));
    signupOK = true;
  } else {
    Serial.printf("Falha ao conectar ao Firebase: %s\n", config.signer.signupError.message.c_str());
    piscarLED(255, 0, 0, 1000); // LED Vermelho piscando lentamente (erro não especificado)
  }
  config.token_status_callback = tokenStatusCallback; // Callback para verificar o status do token
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Inicializa o ThingSpeak
  ThingSpeak.begin(client);

  // Configura o módulo LoRa E32
  Lora.init();
  Lora.SetAddressH(1);  // Configura o endereço alto do LoRa
  Lora.SetAddressL(0);  // Configura o endereço baixo do LoRa
  Lora.SetUARTBaudRate(3); // Configura a taxa de baud da UART
  Lora.SetSpeed(0b010);  // Configura a velocidade de comunicação
  Lora.SetTransmissionMode(0); // Configura o modo de transmissão
  Lora.SetAirDataRate(2);  // Configura a taxa de dados no ar
  Lora.SetChannel(53); // Configura o canal de comunicação 915Mhz
  Lora.SetAddress(1); // Define o endereço do dispositivo como 1
  Lora.SaveParameters(PERMANENT); // Salva as configurações permanentemente
  Lora.PrintParameters();  // Exibe as configurações no monitor serial
}

void loop() {
  Serial.println("Aguardando sinal LoRa...");
  delay(1000);
  

  // Verifica se há dados recebidos via LoRa
  if(Serial2.available()){
    
    definirCor(128, 0, 128); // Roxo
    bool recebido = Lora.GetStruct(&dados, sizeof(dados));  // Recebe a estrutura de dados

    // Captura o tempo de recepção
    tempoRecebimento = millis();  // Tempo atual no receptor  
    if(!sincronizado){
      diferencaTempo = tempoRecebimento - dados.tempoEnvio; // Define a diferença entre transmissor e receptor
      sincronizado = true;
      Serial.println("Tempos sincronizados.");

    }  

    // Calcula a latência
    latencia = (tempoRecebimento - diferencaTempo) - dados.tempoEnvio;
    pacotesRecebidos++;
    
    
      
    // Atualiza o contador de pacotes recebidos
    
    Serial.print("Pacotes recebidos: ");Serial.println(pacotesRecebidos);
    // Calcula a taxa de perda de pacotes
    taxaPerdaPacotes=0.0;
    taxaPerdaPacotes = ((float)(dados.Contador - pacotesRecebidos) / (float)dados.Contador) * 100;      // Calcula a qualidade da transmissão
    qualidadeTransmissao = 100.0 - taxaPerdaPacotes;
    
    // Define o status da qualidade de transmissão baseado na qualidade de transmissão
    if (qualidadeTransmissao >= 67) {
      statusQualidade = "alta";  // Alta
      definirCor(0, 0, 255); // Azul direto (tudo certo, qualidade alta)    
    } else if (qualidadeTransmissao >= 34 && qualidadeTransmissao < 67) {
      statusQualidade = "media";  // Média
      definirCor(255, 255, 0); // Amarelo 
      
    } else {
      statusQualidade = "baixa";  // Baixa
      definirCor(255, 165, 0); // Laranja
      
    }

    
    

    if(dados.Temp>tempminAceitavel && dados.Temp<tempmaxAceitavel){
      definirCor(0, 0, 255); // Azul direto (enviando e recebendo pacotes, tudo certo)
    
     
      // Filtro para o sensor de chuva
      
      if(dados.Chuva == 1){
        stringChuva = "chovendo";
      }else if(dados.Chuva == 0){
        stringChuva = "não chove";
      }else{
        stringChuva = "Sem status de Chuva.";
      }

      //Status Temperatura
      if (dados.Temp <= 0.0) {
        statusTemperatura = "muito_baixa";
      } else if (dados.Temp > 0.00 && dados.Temp <= 10.0) {
        statusTemperatura = "baixa";
      } else if (dados.Temp > 10.0 && dados.Temp <= 20.0) {
      statusTemperatura = "moderada";
      } else if (dados.Temp > 20.0 && dados.Temp <= 35.0) {
        statusTemperatura = "alta";
      } else {
        statusTemperatura = "muito_alta";
      }

      //Status Umidade
      if (dados.Umid>=0.0 && dados.Umid<=20.0) {
        statusUmidade = "baixa";
      } else if (dados.Umid>20.0 && dados.Umid <= 50.0) {
        statusUmidade = "moderada";
      } else {
        statusUmidade = "alta";
      }
      //Indice UV
      if(dados.IndiceUV>=8){
        statusUV = "extremo";
      }else if(dados.IndiceUV>=6){
        statusUV = "muito_alto";
      }else if(dados.IndiceUV>=4){
        statusUV = "alto";
      }else if(dados.IndiceUV>=3){
        statusUV = "moderado";
      }else{
        statusUV = "baixo";
      }
      

      // Exibe os dados recebidos no monitor serial
      Serial.print("Pacote n°: "); Serial.println(dados.Contador);
      Serial.print("Temperatura: "); Serial.println(dados.Temp);
      Serial.print("Umidade: "); Serial.println(dados.Umid);
      Serial.print("Chuva: "); Serial.println(stringChuva);
      Serial.print("Pressão: "); Serial.println(dados.Pressao);
      Serial.print("Índice UV: "); Serial.println(dados.IndiceUV);
      Serial.print("Taxa de Perda de Pacotes: "); Serial.print(taxaPerdaPacotes); Serial.println("%");
      Serial.print("Qualidade de Transmissão: "); Serial.print(qualidadeTransmissao); Serial.println("%");
      Serial.print("Status Qualidade de Trasmissão: "); Serial.println(statusQualidade);
  
      // Envia os dados para o Firebase e ThingSpeak
      enviarParaFirebase();
      enviarParaThingSpeak();
      
      // Pisca o LED roxo uma vez após enviar os dados
      piscarLED(128, 0, 128, 2000); // Roxo piscando uma vez
      apagarLED();
      delay(100);
      definirCor(0, 0, 255); // Azul direto (enviando e recebendo pacotes, tudo certo)

  
      // Verifica se é necessário resetar o Sistema
      if(Firebase.RTDB.getString(&fbdo, "/wifi/resetSistema")) {
        resetSistema = fbdo.stringData();          
      }
      if(resetSistema == "1") {
        Serial.println("Resetando Sistema...");
        Firebase.RTDB.setString(&fbdo, "/wifi/resetSistema", "0");  // Reseta o valor no Firebase
        reiniciaSistema();
   
      }    



    }else{
    Lora.GetStruct(&dados, sizeof(dados));
    buscadados++;
    }
    

    
      
    // Verifica se o WiFi está conectado
    if(WiFi.status() != WL_CONNECTED) {
      piscarLED(255, 0, 0, 200); // LED Vermelho direto (sem conexão WiFi)
      apagarLED ();
      piscarLED(255, 0, 0, 200); // LED Vermelho direto (sem conexão WiFi)
      apagarLED();
    }

    if(buscadados>qtdbuscaAceitavel){
      definirCor(255, 0, 0); //Vermelho
      delay(60000); //1 minuto vermelho para voltar a buscar (significa que precisa reiniciar o sistema)
      buscadados=0;
    }
  }else{
    Serial.println("Aguardando transmissão.");
    for (int i = 0; i < 5; i++) {
      piscarLED(0, 0, 255, 500);    //azul piscando 
    }
  }
    //Verifica se é necessário reiniciar wifi
  
  if(Firebase.RTDB.getString(&fbdo, "/wifi/resetWifi")) {
    resetWifi = fbdo.stringData();          
  }
  if(resetWifi == "1") {
    Serial.println("Reiniciando Wifi...");
    Firebase.RTDB.setString(&fbdo, "/wifi/resetWifi", "0");  // Reseta o valor no Firebase
    reiniciaWifi();
    resetWifi = "0";
  }    
  
  
}


void enviarParaFirebase() {
  // Verifica se o Firebase está pronto para enviar dados para o FIREBASE
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Início do envio// 

    // Envia os dados do número de pacotes enviados pelo transmissor e recebidos pelo ESP
    pacote = String(dados.Contador);

    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/pacote", pacote)) {
      Serial.println("Número de Pacotes Enviados pelo transmissor enviado para o Firebase.");
    } else {
      Serial.println("Falha ao enviar Número de Pacotes para o Firebase.");
    }

    contador = String(pacotesRecebidos);

    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/contador", contador)) {
      Serial.println("Número de Pacotes Recebidos pelo Esp enviado para o Firebase.");
    } else {
      Serial.println("Falha ao enviar Número de Pacotes recebidos para o Firebase.");
    }

    //Envia Temperatura e Status
    temperaturaString = String(dados.Temp) + "°C";
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/temperatura", temperaturaString)) {
      Serial.println("Temperatura enviada para o Firebase.");
    } else {
      Serial.println("Falha ao enviar a Temperatura para o Firebase.");
    }

    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/temperatura_status", statusTemperatura)){
      Serial.println(F("Status Temperatura Enviado."));
    } else {
      Serial.println(F("Falha ao status da Temperatura!"));
    }

    //Envia Umidade e Status
    umidadeString = String(dados.Umid) + "%";
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/umidade", umidadeString)) {
      Serial.println("Umidade enviada para o Firebase.");
    } else {
      Serial.println("Falha ao enviar a Umidade para o Firebase.");
    }

    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/umidade_status", statusUmidade)){
      Serial.println(F("Status Umidade Enviado."));
    } else {
      Serial.println(F("Falha ao enviar Status da Umidade!"));
    }

    //Envia pressão
    Pressao = String(dados.Pressao) + "hPa";
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/pressao", Pressao)) {
      Serial.println("Pressao enviada para o Firebase.");
    } else {
      Serial.println("Falha ao enviar a Pressao para o Firebase.");
    }

    // Envia o status de chuva corrigido para o Firebase
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/chuva", stringChuva)) {
      Serial.println("String Chuva enviado para o Firebase.");
    } else {
      Serial.println("Falha ao enviar o String Chuva para o Firebase.");
    }

    // Envia Status Índice UV
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/radiacao", statusUV)){
      Serial.println(F("Indice UV Enviado."));
    }else {
      Serial.println(F("Falha ao enviar Índice UV!"));
    }    

    // Envia a taxa de perda de pacotes para o Firebase
    txPerda = String(taxaPerdaPacotes) + "%";
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/taxaPerdaPacotes", txPerda)) {
      Serial.println("Taxa de Perda de Pacotes enviada para o Firebase.");
    } else {
      Serial.println("Falha ao enviar a Taxa de Perda de Pacotes para o Firebase.");
    }

    // Envia a qualidade de transmissão para o Firebase
    qTransmissao = String(qualidadeTransmissao) + "%";
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/qualidadeTransmissao", qTransmissao)) {
      Serial.println("Qualidade de Transmissão de Pacotes enviada para o Firebase.");
    } else {
      Serial.println("Falha ao enviar a Qualidade de Transmissão de Pacotes para o Firebase.");
    }

    // Envia a qualidade de transmissão para o Firebase
    latenciaFloat = (float)latencia;  // Converter latência para float
    if (Firebase.RTDB.setFloat(&fbdo, "/sensores/ar/latencia", latenciaFloat)) {
    Serial.println("Latência enviada para o Firebase.");
    }else {
    Serial.println("Falha ao enviar Latência para o Firebase.");
    }

    // Envia o status da Qualidade de Transmissão para o Firebase
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/statusQualidade", statusQualidade)) {
      Serial.println("Status da Qualidade de Transmissão enviado para o Firebase.");
    } else {
      Serial.println("Falha ao enviar o Status da Qualidade de Transmissão para o Firebase.");
    }

    // Envia o tempo de atividade
    if (Firebase.RTDB.setString(&fbdo, "/sensores/ar/tempodeatividade", tempoRecebimento)) {
      Serial.println("Tempo de atividade enviado para o Firebase.");
    } else {
      Serial.println("Falha ao enviar o tempo pde atividade para o Firebase.");
    }



  }
  //FIM DO ENVIO PARA O FIREBASE//
  //delay(60000); //Envio de 1 em 1 minuto
  delay(150000); // Delay de 2 minutos e meio 
}


void enviarParaThingSpeak() {
  // Envia os dados para o ThingSpeak
  ThingSpeak.setField(1, dados.Temp); //FIELD 1 -> TEMPERATURA
  ThingSpeak.setField(2, dados.Umid); //FIELD 2 -> UMIDADE
  ThingSpeak.setField(3, dados.Pressao); //FIELD 3 -> PRESSAO
  ThingSpeak.setField(4, dados.IndiceUV); //FIELD 4 -> INDICE UV
  ThingSpeak.setField(5, dados.Chuva); //FIELD 5 -> STATUS CHUVA 
  ThingSpeak.setField(6, taxaPerdaPacotes); //FIELD 6 -> TAXA DE PERDA DE PACOTES
  ThingSpeak.setField(7, qualidadeTransmissao); //FIELD 7 -> QUALIDADE DE TRANSMISSÃO
  ThingSpeak.setField(8, latenciaFloat); //FIELD 8 -> LATENCIA DE TRANSMISSÃO

  int x = ThingSpeak.writeFields(meuNumeroCanal, minhaChaveEscritaAPI);
  if (x == 200) {
    Serial.println("Atualização do ThingSpeak bem-sucedida.");
  } else {
    Serial.println("Falha ao atualizar o ThingSpeak.");
  }
}

void reiniciaSistema() {
  //reseta o sistema completamente
  piscarLED(255, 0, 0, 100); // Vermelho piscando rápido indicando que o sistema está reiniciando
  reinicia.resetArduino = 1;
  Lora.SendStruct(&reinicia, sizeof(reinicia));
  ESP.restart(); // Reinicia o ESP32
  delay(1000);
  reinicia.resetArduino = 0;
  dados.Contador=0;
  resetSistema = "0";

}


void reiniciaWifi() {
  //reseta o sistema completamente
  piscarLED(0, 255, 0, 100); // Verde piscando rápido indicando que o sistema está reiniciando
  wm.resetSettings();
  delay(1000);
  resetWifi = "0";
  ESP.restart(); // Reinicia o ESP32

}




