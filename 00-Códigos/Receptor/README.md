
# Parte 1: Configuração e Conexão do Sistema (Wi-Fi, Firebase, ThingSpeak, LoRa)

Nesta primeira parte, explicaremos como o sistema é configurado e conectado aos serviços de nuvem e como o módulo LoRa é preparado para comunicação.

## Configuração de Wi-Fi e Inicialização do Firebase e ThingSpeak

Na função `setup()`, a primeira tarefa é configurar a conexão Wi-Fi utilizando a biblioteca `WiFiManager`. Caso a conexão falhe, o ESP32 será reiniciado.

```cpp
// Configuração de Wi-Fi
wm.autoConnect("Projeto SIM", "projetosim#123");
```

Após a conexão Wi-Fi, o Firebase e o ThingSpeak são configurados para receber os dados:

```cpp
// Inicialização do Firebase
Firebase.begin(&config, &auth);

// Inicialização do ThingSpeak
ThingSpeak.begin(client);
```

## Configuração do Módulo LoRa

A comunicação via LoRa é configurada definindo os pinos RX, TX, M0, M1 e AX, que conectam o ESP32 ao módulo EBYTE LoRa E32:

```cpp
#define RX 16
#define TX 17
#define M0 25
#define M1 22
#define AX 21
```

Depois, o módulo LoRa é inicializado com os parâmetros de configuração apropriados:

```cpp
Lora.init();
Lora.SetAddress(1);
Lora.SetSpeed(0b010);  // Velocidade de comunicação
Lora.SetChannel(53);   // Canal 915MHz
Lora.SaveParameters(PERMANENT);
```

## Estruturas de Dados

O sistema utiliza as seguintes estruturas para armazenar os dados:

### Estrutura `PACOTE`

Armazena as leituras dos sensores de temperatura, umidade, pressão, chuva e índice UV.

```cpp
struct PACOTE {
  unsigned long Contador;
  float Pressao;
  float Temp;
  float Umid;
  bool Chuva;
  byte IndiceUV;

};
```

### Estrutura `RESET`

Permite o reinício do sistema remotamente:

```cpp
struct RESET{
  bool resetArduino;
};
```

---

---

# Parte 2: Processamento, Cálculos, e Envio de Dados

Nesta parte, vamos detalhar como o sistema processa os dados recebidos dos sensores, realiza cálculos e os envia ao Firebase e ThingSpeak.

## Recepção dos Dados dos Sensores via LoRa

Na função `loop()`, o sistema verifica constantemente se há dados recebidos via LoRa:

```cpp
void loop() {
    if(Serial2.available()) {
        bool recebido = Lora.GetStruct(&dados, sizeof(dados));
        pacotesRecebidos++;
        enviarParaFirebase();
        enviarParaThingSpeak();
    }
}
```

Quando os dados são recebidos, eles são armazenados na estrutura `PACOTE` e então processados para serem enviados ao Firebase e ao ThingSpeak.

## Cálculos de Qualidade de Transmissão

A qualidade da transmissão é determinada pela taxa de perda de pacotes:

```cpp
float qualidadeTransmissao = 100.0 - taxaPerdaPacotes;
```

Se a qualidade for superior a 67%, a transmissão é considerada **alta**. Entre 34% e 67% é **média**, e abaixo de 34% é **baixa**.

```cpp
if (qualidadeTransmissao >= 67) {
  statusQualidade = "alta";
} else if (qualidadeTransmissao >= 34) {
  statusQualidade = "media";
} else {
  statusQualidade = "baixa";
}
```

## Tratamento e Envio dos Dados ao Firebase

A função `enviarParaFirebase()` formata e envia os dados dos sensores para o banco de dados em tempo real:


```cpp
void enviarParaFirebase() {
    temperaturaString = String(dados.Temp) + "°C";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/temperatura", temperaturaString);

    umidadeString = String(dados.Umid) + "%";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/umidade", umidadeString);

    Pressao = String(dados.Pressao) + "hPa";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/pressao", Pressao);

    stringChuva = (dados.Chuva == 1) ? "chovendo" : "não chove";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/chuva", stringChuva);

    Firebase.RTDB.setString(&fbdo, "/sensores/ar/radiacao", String(dados.IndiceUV));

    txPerda = String(taxaPerdaPacotes) + "%";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/taxaPerdaPacotes", txPerda);

    qTransmissao = String(qualidadeTransmissao) + "%";
    Firebase.RTDB.setString(&fbdo, "/sensores/ar/qualidadeTransmissao", qTransmissao);
}
```

Cada valor dos sensores é convertido em uma string e enviado para os nós correspondentes no Firebase.

## Envio de Dados ao ThingSpeak

O ThingSpeak é utilizado para visualização dos dados em tempo real:

```cpp
void enviarParaThingSpeak() {
    ThingSpeak.setField(1, dados.Temp);
    ThingSpeak.setField(2, dados.Umid);
    ThingSpeak.setField(3, dados.Pressao);
    ThingSpeak.setField(4, dados.IndiceUV);
    ThingSpeak.setField(5, dados.Chuva);
    ThingSpeak.setField(6, taxaPerdaPacotes);
    ThingSpeak.setField(7, qualidadeTransmissao);
   
    
    int x = ThingSpeak.writeFields(meuNumeroCanal, minhaChaveEscritaAPI);
    if (x == 200) {
        Serial.println("Atualização do ThingSpeak bem-sucedida.");
    } else {
        Serial.println("Falha ao atualizar o ThingSpeak.");
    }
}
```

Esta função envia os dados formatados para os campos configurados no canal ThingSpeak.

---
