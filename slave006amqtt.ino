#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// =====================================================
// DS18B20
// =====================================================

#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// =====================================================
// WI-FI DO MASTER
// =====================================================

const char* ssid = "ESP32_Transmission_Line";
const char* password = "password123";

const char* serverIP = "192.168.4.1";
const uint16_t serverPort = 80;

WiFiClient client;

// =====================================================
// CALIBRACAO RSSI / DISTANCIA
// =====================================================

// Valor médio medido a 1 metro
const float RSSI_1M = -59.3;

// Expoente de propagação para espaço livre
const float N_PATH = 2.0;

// =====================================================
// CONTADOR DE PACOTES
// =====================================================

unsigned long pacote = 0;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  sensors.begin();

  Serial.println();
  Serial.println("======================================");
  Serial.println("ESP32 SLAVE / TRANSMISSOR");
  Serial.println("======================================");

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Master");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("Wi-Fi conectado.");

  Serial.print("IP do Slave: ");
  Serial.println(WiFi.localIP());

  Serial.println();

  Serial.println(
    "Pacote,Tempo(ms),Temperatura(C),RSSI(dBm),Distancia(m),RTT(ms),ACK"
  );
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // VERIFICA CONEXAO WI-FI
  // ===================================================

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Wi-Fi desconectado. Reconectando...");

    WiFi.disconnect();

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {

      delay(500);
      Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi reconectado.");
  }


  // ===================================================
  // LEITURA DO DS18B20
  // ===================================================

  sensors.requestTemperatures();

  float temperatura =
    sensors.getTempCByIndex(0);


  if (temperatura == DEVICE_DISCONNECTED_C) {

    Serial.println(
      "Erro: DS18B20 desconectado!"
    );

    delay(2000);

    return;
  }


  // ===================================================
  // RSSI
  // ===================================================

  int rssi = WiFi.RSSI();


  // ===================================================
  // ESTIMATIVA DE DISTANCIA
  // ===================================================

  float distancia =
    pow(
      10.0,
      (RSSI_1M - rssi) /
      (10.0 * N_PATH)
    );


  // ===================================================
  // NUMERO DO PACOTE
  // ===================================================

  pacote++;


  // ===================================================
  // CONEXAO COM MASTER
  // ===================================================

  if (client.connect(serverIP, serverPort)) {

    /*
       Formato:

       DATA,pacote,temperatura,rssi,distancia

       Exemplo:

       DATA,25,29.50,-59,0.97
    */

    String mensagem =
      "DATA," +
      String(pacote) + "," +
      String(temperatura, 2) + "," +
      String(rssi) + "," +
      String(distancia, 2);


    // =================================================
    // MARCA INICIO DO RTT
    // =================================================

    unsigned long inicioRTT =
      millis();


    // =================================================
    // ENVIA PACOTE
    // =================================================

    client.println(mensagem);


    // =================================================
    // AGUARDA ACK
    // =================================================

    unsigned long inicioTimeout =
      millis();


    while (!client.available()) {

      if (
        millis() - inicioTimeout >
        2000
      ) {

        Serial.print(pacote);
        Serial.print(",");

        Serial.print(millis());
        Serial.print(",");

        Serial.print(temperatura, 2);
        Serial.print(",");

        Serial.print(rssi);
        Serial.print(",");

        Serial.print(distancia, 2);
        Serial.print(",");

        Serial.print("-1");
        Serial.println(",TIMEOUT");

        client.stop();

        delay(3000);

        return;
      }
    }


    // =================================================
    // RECEBE ACK
    // =================================================

    String resposta =
      client.readStringUntil('\n');

    resposta.trim();


    // =================================================
    // CALCULA RTT
    // =================================================

    unsigned long rtt =
      millis() - inicioRTT;


    String ackEsperado =
      "ACK," + String(pacote);


    // =================================================
    // VERIFICA ACK
    // =================================================

    if (resposta == ackEsperado) {

      // -----------------------------------------------
      // INFORMA RTT AO MASTER
      // -----------------------------------------------

      String mensagemRTT =
        "RTT," +
        String(pacote) + "," +
        String(rtt);

      client.println(mensagemRTT);


      // -----------------------------------------------
      // MOSTRA CSV NO SLAVE
      // -----------------------------------------------

      Serial.print(pacote);
      Serial.print(",");

      Serial.print(millis());
      Serial.print(",");

      Serial.print(temperatura, 2);
      Serial.print(",");

      Serial.print(rssi);
      Serial.print(",");

      Serial.print(distancia, 2);
      Serial.print(",");

      Serial.print(rtt);
      Serial.println(",OK");

    }

    else {

      Serial.print(pacote);
      Serial.print(",");

      Serial.print(millis());
      Serial.print(",");

      Serial.print(temperatura, 2);
      Serial.print(",");

      Serial.print(rssi);
      Serial.print(",");

      Serial.print(distancia, 2);
      Serial.print(",");

      Serial.print(rtt);
      Serial.println(",ACK_ERROR");
    }


    client.stop();
  }

  else {

    Serial.print(pacote);
    Serial.print(",");

    Serial.print(millis());
    Serial.print(",");

    Serial.print(temperatura, 2);
    Serial.print(",");

    Serial.print(rssi);
    Serial.print(",");

    Serial.print(distancia, 2);
    Serial.print(",");

    Serial.print("-1");

    Serial.println(
      ",CONNECTION_ERROR"
    );
  }


  delay(3000);
}
