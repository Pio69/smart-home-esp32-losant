#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Losant.h>
#include <math.h>
#include "secrets.h"

// 0 = dados simulados; 1 = PZEM-004T v3.0, apos confirmar o modelo.
#define USE_PZEM 0
// 0 = saida desativada para testes; 1 = habilitar somente apos validar o circuito.
#define ENABLE_RELAY 0
#if USE_PZEM
#include <PZEM004Tv30.h>
// Ajuste conforme o circuito validado. RX do ESP32 recebe TX do medidor.
PZEM004Tv30 pzem(Serial2, 16, 17);
#endif

constexpr int RELAY_PIN = 23;
// Muitos modulos sao ativos em LOW. Confirme a polaridade antes de habilitar.
constexpr int RELAY_ON = LOW;
constexpr int RELAY_OFF = HIGH;
constexpr unsigned long REPORT_INTERVAL = 30000;
constexpr unsigned long RETRY_INTERVAL = 15000;
bool relayOn = false;
unsigned long lastReport = 0;
unsigned long lastRetry = 0;
const char* rootCABuff = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure wifiClient;
LosantDevice device(LOSANT_DEVICE_ID);

void applyRelay() {
#if ENABLE_RELAY
  digitalWrite(RELAY_PIN, relayOn ? RELAY_ON : RELAY_OFF);
#endif
  Serial.printf("Saida solicitada: %s%s\n", relayOn ? "ligada" : "desligada",
                ENABLE_RELAY ? "" : " (somente simulacao)");
}

void handleCommand(LosantCommand *command) {
  if (strcmp(command->name, "toggle") == 0) {
    relayOn = !relayOn;
  } else if (strcmp(command->name, "on") == 0) {
    relayOn = true;
  } else if (strcmp(command->name, "off") == 0) {
    relayOn = false;
  } else {
    return;
  }
  applyRelay();
}

void publishReadings() {
  float voltage, current, power, frequency, powerFactor;
#if USE_PZEM
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  frequency = pzem.frequency();
  powerFactor = pzem.pf();
  if (isnan(voltage) || isnan(current) || isnan(power) ||
      isnan(frequency) || isnan(powerFactor)) {
    Serial.println("Falha ao ler o medidor; nenhuma leitura enviada.");
    return;
  }
#else
  voltage = 220.0f;
  current = relayOn ? 0.40f : 0.0f;
  powerFactor = 1.0f;
  power = voltage * current * powerFactor;
  frequency = 60.0f;
#endif
  StaticJsonDocument<256> buffer;
  JsonObject state = buffer.to<JsonObject>();
  state["voltage"] = voltage;
  state["current"] = current;
  state["power"] = power;
  state["frequency"] = frequency;
  state["powerFact"] = powerFactor;
  // True indica dados simulados; relayOn representa a ultima solicitacao.
  state["simulated"] = !USE_PZEM;
  state["relayOn"] = relayOn;
  device.sendState(state);
  Serial.println("Estado enviado a Losant.");
}

void setup() {
  Serial.begin(115200);
#if ENABLE_RELAY
  // Define nivel inativo antes de mudar o pino para saida.
  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT);
#endif
  applyRelay();
  wifiClient.setCACert(rootCABuff);
  device.onCommand(&handleCommand);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastRetry = millis() - RETRY_INTERVAL;
}

void loop() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastRetry >= RETRY_INTERVAL) {
      lastRetry = now;
      Serial.println("Tentando conectar ao Wi-Fi...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  } else if (!device.connected()) {
    if (now - lastRetry >= RETRY_INTERVAL) {
      lastRetry = now;
      Serial.println("Tentando conectar a Losant...");
      device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);
    }
  } else {
    device.loop();
    if (now - lastReport >= REPORT_INTERVAL) {
      lastReport = now;
      publishReadings();
    }
  }
  delay(50);
}
