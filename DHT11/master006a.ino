#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// =====================================================
// ACCESS POINT PARA O SLAVE
// =====================================================

const char* ssid_ap =
  "ESP32_Transmission_Line";

const char* password_ap =
  "password123";


// =====================================================
// WI-FI COM INTERNET
// =====================================================

const char* wifiSSID =
  "REDE_WIFI";

const char* wifiPassword =
  "Senha";


// =====================================================
// THINGSPEAK
// =====================================================

const char* writeAPIKey =
  "CHAVE_API_WRITE";

const char* thingSpeakServer =
  "https://api.thingspeak.com/update";


// =====================================================
// SERVIDOR LOCAL
// =====================================================

WiFiServer server(80);


// =====================================================
// VARIAVEIS
// =====================================================

unsigned long ultimoPacote = 0;

float ultimaTemperatura = 0.0;

// NOVO CAMPO
float ultimaUmidade = 0.0;

int ultimoRSSI = 0;

float ultimaDistancia = 0.0;

unsigned long ultimoRTT = 0;

unsigned long ultimoTempoRecepcao = 0;

// 1 = ACK confirmado
// 0 = falha
int ultimoACK = 0;

bool dadosNovos = false;


// =====================================================
// THINGSPEAK
// =====================================================

const unsigned long intervaloThingSpeak =
  20000;

unsigned long ultimoEnvioThingSpeak = 0;


// =====================================================
// ENVIA PARA THINGSPEAK
// =====================================================

void enviarThingSpeak() {

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    Serial.println(
      "Sem conexao com Internet."
    );

    return;
  }


  WiFiClientSecure secureClient;

  secureClient.setInsecure();

  HTTPClient http;


  // ===================================================
  // CAMPOS DO THINGSPEAK
  // ===================================================

  /*
      Field 1 = Temperatura
      Field 2 = Umidade
      Field 3 = RSSI
      Field 4 = Distancia estimada
      Field 5 = Numero do pacote
      Field 6 = Tempo de recepcao
      Field 7 = RTT
      Field 8 = ACK
  */


  String url =
    String(thingSpeakServer) +

    "?api_key=" +
    String(writeAPIKey) +

    "&field1=" +
    String(ultimaTemperatura, 2) +

    "&field2=" +
    String(ultimaUmidade, 2) +

    "&field3=" +
    String(ultimoRSSI) +

    "&field4=" +
    String(ultimaDistancia, 2) +

    "&field5=" +
    String(ultimoPacote) +

    "&field6=" +
    String(ultimoTempoRecepcao) +

    "&field7=" +
    String(ultimoRTT) +

    "&field8=" +
    String(ultimoACK);


  Serial.println();

  Serial.println(
    "Enviando para ThingSpeak..."
  );


  if (
    http.begin(
      secureClient,
      url
    )
  ) {

    int httpCode =
      http.GET();


    if (httpCode > 0) {

      String resposta =
        http.getString();


      Serial.print(
        "HTTP: "
      );

      Serial.println(
        httpCode
      );


      Serial.print(
        "Entry ID: "
      );

      Serial.println(
        resposta
      );


      if (
        resposta.toInt() > 0
      ) {

        Serial.println(
          "ThingSpeak atualizado."
        );

      }

      else {

        Serial.println(
          "Atualizacao nao aceita."
        );
      }
    }

    else {

      Serial.print(
        "Erro HTTP: "
      );

      Serial.println(
        http.errorToString(httpCode)
      );
    }


    http.end();
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    "ESP32 MASTER / RECEPTOR"
  );

  Serial.println(
    "======================================"
  );


  // ===================================================
  // AP + STATION
  // ===================================================

  WiFi.mode(WIFI_AP_STA);


  // ===================================================
  // ACCESS POINT
  // ===================================================

  WiFi.softAP(
    ssid_ap,
    password_ap
  );


  Serial.println();

  Serial.println(
    "Access Point criado."
  );


  Serial.print(
    "IP Master: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // ===================================================
  // INTERNET
  // ===================================================

  Serial.println();

  Serial.print(
    "Conectando ao roteador"
  );


  WiFi.begin(
    wifiSSID,
    wifiPassword
  );


  unsigned long inicioWiFi =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - inicioWiFi < 20000
  ) {

    delay(500);

    Serial.print(".");
  }


  if (
    WiFi.status() == WL_CONNECTED
  ) {

    Serial.println();

    Serial.println(
      "Internet conectada."
    );


    Serial.print(
      "IP Internet: "
    );

    Serial.println(
      WiFi.localIP()
    );

  }

  else {

    Serial.println();

    Serial.println(
      "Internet nao conectada."
    );
  }


  // ===================================================
  // SERVIDOR LOCAL
  // ===================================================

  server.begin();


  Serial.println();

  Serial.println(
    "Servidor iniciado."
  );


  Serial.println();

  Serial.println(
    "Pacote,Tempo(ms),Temperatura(C),Umidade(%),RSSI(dBm),Distancia(m),RTT(ms),ACK"
  );
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  WiFiClient slaveClient =
    server.available();


  if (slaveClient) {

    // =================================================
    // AGUARDA PRIMEIRO PACOTE
    // =================================================

    unsigned long inicioTimeout =
      millis();


    while (
      slaveClient.connected() &&
      !slaveClient.available()
    ) {

      if (
        millis() - inicioTimeout > 2000
      ) {

        slaveClient.stop();

        return;
      }
    }


    // =================================================
    // RECEBE DATA
    // =================================================

    if (
      slaveClient.available()
    ) {

      String dados =
        slaveClient.readStringUntil('\n');

      dados.trim();


      /*
          NOVO FORMATO:

          DATA,pacote,temperatura,umidade,rssi,distancia

          Exemplo:

          DATA,25,29.00,72.00,-59,0.97
      */


      if (
        dados.startsWith("DATA,")
      ) {

        String conteudo =
          dados.substring(5);


        // ===============================================
        // LOCALIZA AS CINCO VIRGULAS
        // ===============================================

        int v1 =
          conteudo.indexOf(',');


        int v2 =
          conteudo.indexOf(
            ',',
            v1 + 1
          );


        int v3 =
          conteudo.indexOf(
            ',',
            v2 + 1
          );


        int v4 =
          conteudo.indexOf(
            ',',
            v3 + 1
          );


        if (
          v1 > 0 &&
          v2 > v1 &&
          v3 > v2 &&
          v4 > v3
        ) {

          // =============================================
          // CAMPOS RECEBIDOS
          // =============================================

          // PACOTE
          ultimoPacote =
            conteudo.substring(
              0,
              v1
            ).toInt();


          // TEMPERATURA
          ultimaTemperatura =
            conteudo.substring(
              v1 + 1,
              v2
            ).toFloat();


          // UMIDADE
          ultimaUmidade =
            conteudo.substring(
              v2 + 1,
              v3
            ).toFloat();


          // RSSI
          ultimoRSSI =
            conteudo.substring(
              v3 + 1,
              v4
            ).toInt();


          // DISTANCIA
          ultimaDistancia =
            conteudo.substring(
              v4 + 1
            ).toFloat();


          // TEMPO DE RECEPCAO
          ultimoTempoRecepcao =
            millis();


          // inicialmente ainda não sabemos
          // se o Slave recebeu o ACK

          ultimoACK = 0;

          ultimoRTT = 0;


          // =============================================
          // ENVIA ACK
          // =============================================

          slaveClient.print(
            "ACK,"
          );

          slaveClient.println(
            ultimoPacote
          );


          // =============================================
          // AGUARDA O SLAVE INFORMAR RTT
          // =============================================

          unsigned long inicioRTTTimeout =
            millis();


          while (
            slaveClient.connected() &&
            !slaveClient.available() &&
            millis() -
            inicioRTTTimeout <
            1000
          ) {

            delay(1);
          }


          // =============================================
          // RECEBE RTT
          // =============================================

          if (
            slaveClient.available()
          ) {

            String mensagemRTT =
              slaveClient.readStringUntil('\n');

            mensagemRTT.trim();


            /*
                Formato:

                RTT,pacote,rtt

                Exemplo:

                RTT,25,7
            */


            if (
              mensagemRTT.startsWith(
                "RTT,"
              )
            ) {

              String dadosRTT =
                mensagemRTT.substring(4);


              int virgulaRTT =
                dadosRTT.indexOf(',');


              if (
                virgulaRTT > 0
              ) {

                unsigned long pacoteRTT =
                  dadosRTT.substring(
                    0,
                    virgulaRTT
                  ).toInt();


                unsigned long rtt =
                  dadosRTT.substring(
                    virgulaRTT + 1
                  ).toInt();


                // Confirma que é o mesmo pacote
                if (
                  pacoteRTT ==
                  ultimoPacote
                ) {

                  ultimoRTT =
                    rtt;

                  ultimoACK =
                    1;
                }
              }
            }
          }


          // =============================================
          // MOSTRA NO MONITOR SERIAL
          // =============================================

          Serial.print(
            ultimoPacote
          );

          Serial.print(",");


          Serial.print(
            ultimoTempoRecepcao
          );

          Serial.print(",");


          Serial.print(
            ultimaTemperatura,
            2
          );

          Serial.print(",");


          Serial.print(
            ultimaUmidade,
            2
          );

          Serial.print(",");


          Serial.print(
            ultimoRSSI
          );

          Serial.print(",");


          Serial.print(
            ultimaDistancia,
            2
          );

          Serial.print(",");


          Serial.print(
            ultimoRTT
          );

          Serial.print(",");


          if (
            ultimoACK == 1
          ) {

            Serial.println(
              "OK"
            );

          }

          else {

            Serial.println(
              "ACK_ERROR"
            );
          }


          dadosNovos =
            true;
        }
      }
    }


    slaveClient.stop();
  }


  // ===================================================
  // THINGSPEAK
  // ===================================================

/*
//Serial.print("dadosNovos = ");
Serial.print(dadosNovos);

Serial.print(" | WiFi = ");
Serial.print(WiFi.status());

Serial.print(" | Tempo desde ultimo envio = ");
Serial.println(millis() - ultimoEnvioThingSpeak);
*/
  if (
    dadosNovos &&
    WiFi.status() ==
      WL_CONNECTED &&
    millis() -
      ultimoEnvioThingSpeak >=
      intervaloThingSpeak
  ) {

    enviarThingSpeak();


    ultimoEnvioThingSpeak =
      millis();

    dadosNovos =
      false;
  }
}



