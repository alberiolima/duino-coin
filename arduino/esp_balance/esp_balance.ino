#include <ArduinoJson.h>
#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <HTTPClient.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#endif

/* Wifi config */
const char* SSID = "ridimuim";
const char* PASSWORD = "88999448494";

/* Duino-coin user name */
const char* USERNAME = "ridimuim";

/* Variáveis públicas */
float saldo_atual = 0.0f;
float coins_segundo = 0.0f;
float contador = 0.0f;
float soma_coins_segundo = 0.0f;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  /* Conecta com a rede Wifi */
  Serial.println("Conectando: " + String(SSID));  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  int wait_passes = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(50);
    Serial.print(".");
    if (++wait_passes >= 100) {
      WiFi.begin(SSID, PASSWORD);
      wait_passes = 0;
    }
  }
  digitalWrite(LED_BUILTIN, HIGH );
  Serial.println("\nConectado com successo na rede WiFi");
  Serial.println("Local IP address: " + WiFi.localIP().toString());
  Serial.println();

  /* Atualiza primeira informação */
  Serial.println("Agurdade um pouco a primeira informação");
  while (!atualizaDados()) {
    yield();
  }

}

void loop() {
  yield();
  
  /* Mostra dados quando houver atualização */
  if (atualizaDados()) {
    
    /* Atualiza dados para calculo da média */
    contador++;
    soma_coins_segundo += coins_segundo;
    
    /* Envia informações pela porta serial */
    printTime();
    Serial.print(" Saldo: ");
    Serial.print(saldo_atual,8);
    Serial.print("\t Coins/hora: ");
    Serial.print(coins_segundo * 3600.0f,8);
    Serial.print("\t Coins/dia: ");
    Serial.print(coins_segundo * 86400.0f,2);
    Serial.print("\t Media/dia: ");
    Serial.print((soma_coins_segundo / contador )* 86400.0f,2);
    Serial.println();    
  }
}

/* Atualiza informações */
boolean atualizaDados() {
  unsigned long proximaConsulta = 0;
  static float saldo_anterior = 0.0f;
  static unsigned long tempo_anterior = 0.0f;
  
  /* Verifica intervalo entre atualizações */
  if ( millis() < proximaConsulta ) {
    return false;
  }
  proximaConsulta = millis() + 5000UL;
  
  /* Consulta servidor */
  String url_c = "https://server.duinocoin.com/balances/" + String(USERNAME);
  String r = httpGetString(url_c);
  if ( r == "" ) {
    return false;
  }
  
  /* Desempacota dados json */
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, r);  
  float saldo = float(doc["result"]["balance"]);
  if ( saldo == saldo_anterior ) {
    return false;
  }
  
  /* Calculadados */
  unsigned long tempo = millis();
  float tempo_decorrido_s = (float)(tempo - tempo_anterior)/1000.0f;
  float diff = saldo - saldo_anterior;
  saldo_anterior = saldo;
  tempo_anterior = tempo; 
  saldo_atual = saldo;
  coins_segundo = diff / tempo_decorrido_s;
  return true;  
}

/* Retorna dados da consulta HTTP/GET */
String httpGetString(String URL) {
  String payload = "";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (http.begin(client, URL)) {
    if (http.GET() == HTTP_CODE_OK) { 
      payload = http.getString();
    }    
    http.end();
  }
  return payload;
}

/* Imprime hora */
void printTime() {
  unsigned long t = millis()/1000;
  /* Horas */
  unsigned long h = t/3600;
  t = t%3600;
  if ( h < 10 ) {
    Serial.print("0");
  }
  Serial.print(h);
  Serial.print(":");
  
  /* minugos */
  h = t / 60;
  t = t%60;
  if ( h < 10 ) {
    Serial.print("0");
  }
  Serial.print(h);
  Serial.print(":");  

  /* segundos */
  if ( t < 10 ) {
    Serial.print("0");
  }
  Serial.print(t); 
}
