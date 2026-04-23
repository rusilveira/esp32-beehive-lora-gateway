#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

// =====================================================
// PINOS LORA (VALIDADOS)
// =====================================================

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_DIO0  26
#define LORA_RST   14

// =====================================================
// OBJETOS GLOBAIS
// =====================================================

SPIClass spiLoRa(VSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, RADIOLIB_NC, spiLoRa);

// =====================================================
// SUPERVISAO WIFI / API
// =====================================================

static const unsigned long WIFI_CHECK_INTERVAL_MS = 10000;
static const unsigned long WIFI_RECONNECT_TIMEOUT_MS = 15000;
static const unsigned long OFFLINE_RESTART_TIMEOUT_MS = 20UL * 60UL * 1000UL; // 20 min
static const uint8_t MAX_WIFI_FAILS = 5;
static const uint8_t MAX_API_FAILS = 5;

static unsigned long ultimoCheckWiFiMs = 0;
static unsigned long ultimoSucessoApiMs = 0;
static unsigned long ultimoWiFiOkMs = 0;

static uint8_t falhasWiFiConsecutivas = 0;
static uint8_t falhasApiConsecutivas = 0;

// =====================================================
// PROTOCOLO
// =====================================================

constexpr uint8_t MAGIC_BYTE    = 0xA5;
constexpr uint8_t PROTO_VERSION = 0x01;

constexpr uint8_t MSG_DATA = 0x10;
constexpr uint8_t MSG_ACK  = 0x20;

constexpr uint8_t GATEWAY_ID = 0xF0;

// =====================================================
// FUNÇÃO CONEXÃO WIFI
// =====================================================

static bool conectarWiFi(unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(500);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[WIFI] Conectando");

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Conectado");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());

    ultimoWiFiOkMs = millis();
    falhasWiFiConsecutivas = 0;
    return true;
  }

  Serial.println("[WIFI] Falha na conexao");
  falhasWiFiConsecutivas++;
  return false;
}

static void garantirWiFiConectado() {
  if (WiFi.status() == WL_CONNECTED) {
    ultimoWiFiOkMs = millis();
    return;
  }

  Serial.println("[WIFI] Desconectado. Tentando reconectar...");
  bool ok = conectarWiFi(WIFI_RECONNECT_TIMEOUT_MS);

  if (!ok) {
    Serial.print("[WIFI] Falhas consecutivas: ");
    Serial.println(falhasWiFiConsecutivas);

    if (falhasWiFiConsecutivas >= MAX_WIFI_FAILS) {
      Serial.println("[WIFI] Muitas falhas consecutivas. Reiniciando ESP...");
      delay(1000);
      ESP.restart();
    }
  }
}

static void verificarSaudeSistema() {
  unsigned long agora = millis();

  if (WiFi.status() == WL_CONNECTED) {
    ultimoWiFiOkMs = agora;
  }

  bool offlineWiFi = (agora - ultimoWiFiOkMs) > OFFLINE_RESTART_TIMEOUT_MS;
  bool offlineApi = ultimoSucessoApiMs > 0 && (agora - ultimoSucessoApiMs) > OFFLINE_RESTART_TIMEOUT_MS;

  if (offlineWiFi) {
    Serial.println("[SYS] Muito tempo sem WiFi valido. Reiniciando ESP...");
    delay(1000);
    ESP.restart();
  }

  if (falhasApiConsecutivas >= MAX_API_FAILS) {
    Serial.println("[SYS] Muitas falhas consecutivas de API. Reiniciando ESP...");
    delay(1000);
    ESP.restart();
  }

  if (offlineApi && WiFi.status() == WL_CONNECTED) {
    Serial.println("[SYS] Muito tempo sem sucesso de envio HTTP. Reiniciando ESP...");
    delay(1000);
    ESP.restart();
  }
}

// =====================================================
// PAYLOAD REAL DA COLMEIA
// =====================================================

struct PayloadColmeia {
  int16_t tempInterna_x100;
  uint16_t umidInterna_x100;

  int16_t tempExterna_x100;
  uint16_t umidExterna_x100;

  int32_t peso_x1000;
  uint16_t bateria_mV;

  uint8_t flags;
  uint8_t reservado;
};

// =====================================================
// FUNÇÃO HTTP
// =====================================================

static void enviarParaBackend(
  uint8_t nodeId,
  uint8_t seq,
  PayloadColmeia& p
) {
  garantirWiFiConectado();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi indisponivel. Envio cancelado.");
    falhasApiConsecutivas++;
    return;
  }

  HTTPClient http;
  http.setTimeout(10000);

  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";

  json += "\"colmeia_id\":\"jatai_01\",";
  json += "\"peso\":" + String(p.peso_x1000 / 1000.0f, 3) + ",";
  json += "\"bateria\":" + String(p.bateria_mV / 1000.0f, 3) + ",";

  json += "\"interna\":{";
  json += "\"temperatura\":" + String(p.tempInterna_x100 / 100.0f, 2) + ",";
  json += "\"umidade\":" + String(p.umidInterna_x100 / 100.0f, 2);
  json += "},";

  json += "\"externa\":{";
  json += "\"temperatura\":" + String(p.tempExterna_x100 / 100.0f, 2) + ",";
  json += "\"umidade\":" + String(p.umidExterna_x100 / 100.0f, 2);
  json += "}";

  json += "}";

  int httpCode = http.POST(json);

  Serial.print("[API] HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0 && httpCode < 400) {
    ultimoSucessoApiMs = millis();
    falhasApiConsecutivas = 0;
  } else {
    falhasApiConsecutivas++;
    Serial.print("[API] Falhas consecutivas: ");
    Serial.println(falhasApiConsecutivas);
  }

  http.end();
}

// =====================================================
// CONTROLE DE DUPLICIDADE
// =====================================================

uint8_t lastSeq = 255;

// =====================================================
// PROTOTIPOS
// =====================================================

uint16_t crc16(const uint8_t* data, size_t len);
bool validPacket(uint8_t* buf, size_t len);
size_t buildAck(uint8_t seq, uint8_t* buf);
void initRadio();

bool conectarWiFi(unsigned long timeoutMs = WIFI_RECONNECT_TIMEOUT_MS);
void garantirWiFiConectado();
void verificarSaudeSistema();

// =====================================================
// CRC16
// =====================================================

uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;

    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }

  return crc;
}

// =====================================================
// VALIDA PACOTE DATA
// =====================================================

bool validPacket(uint8_t* buf, size_t len) {
  if (len < 8) {
    return false;
  }

  uint16_t crc_rx = (buf[len - 2] << 8) | buf[len - 1];
  uint16_t crc_calc = crc16(buf, len - 2);

  return (
    crc_rx == crc_calc &&
    buf[0] == MAGIC_BYTE &&
    buf[1] == PROTO_VERSION &&
    buf[2] == MSG_DATA
  );
}

// =====================================================
// MONTA ACK
// =====================================================

size_t buildAck(uint8_t seq, uint8_t* buf) {
  size_t i = 0;

  buf[i++] = MAGIC_BYTE;
  buf[i++] = PROTO_VERSION;
  buf[i++] = MSG_ACK;
  buf[i++] = GATEWAY_ID;
  buf[i++] = seq;
  buf[i++] = 0x00;

  uint16_t crc = crc16(buf, i);
  buf[i++] = crc >> 8;
  buf[i++] = crc & 0xFF;

  return i;
}

// =====================================================
// INIT RADIO
// =====================================================

void initRadio() {
  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  delay(100);

  if (radio.begin() != RADIOLIB_ERR_NONE) {
    Serial.println("Erro init radio");

    while (true) {
      delay(1000);
    }
  }

  radio.setFrequency(915.0);
  radio.setBandwidth(500.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setOutputPower(2);
  radio.setPreambleLength(12);

  Serial.println("Radio RX OK");
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("=== RX GATEWAY ===");

  conectarWiFi();
  ultimoSucessoApiMs = millis();
  initRadio();
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  unsigned long agora = millis();

  if (agora - ultimoCheckWiFiMs >= WIFI_CHECK_INTERVAL_MS) {
    ultimoCheckWiFiMs = agora;
    garantirWiFiConectado();
    verificarSaudeSistema();
  }

  uint8_t buf[64];

  int state = radio.receive(buf, sizeof(buf), 1000);

  if (state != RADIOLIB_ERR_NONE) {
    return;
  }

  size_t len = radio.getPacketLength();

  if (!validPacket(buf, len)) {
    return;
  }

  uint8_t nodeId = buf[3];
  uint8_t seq = buf[4];
  uint8_t payloadLen = buf[5];

  if (payloadLen != sizeof(PayloadColmeia)) {
    Serial.println("Payload size invalido");
    return;
  }

  delay(100);

  uint8_t ack[16];
  size_t ackLen = buildAck(seq, ack);

  radio.transmit(ack, ackLen);

  if (seq == lastSeq) {
    Serial.print("DUP seq=");
    Serial.println(seq);
    return;
  }

  lastSeq = seq;

  PayloadColmeia p;
  memcpy(&p, &buf[6], sizeof(PayloadColmeia));
  enviarParaBackend(nodeId, seq, p);

  bool dhtInternoValido = p.flags & (1 << 0);
  bool dhtExternoValido = p.flags & (1 << 1);

  Serial.println();

  Serial.print("RX node=");
  Serial.print(nodeId);
  Serial.print(" seq=");
  Serial.println(seq);

  Serial.print("Peso (kg): ");
  Serial.println(p.peso_x1000 / 1000.0f, 3);

  Serial.print("Bateria (V): ");
  Serial.println(p.bateria_mV / 1000.0f, 3);

  Serial.print("Temp interna (C): ");
  if (dhtInternoValido) {
    Serial.println(p.tempInterna_x100 / 100.0f, 2);
  } else {
    Serial.println("ERRO");
  }

  Serial.print("Umid interna (%): ");
  if (dhtInternoValido) {
    Serial.println(p.umidInterna_x100 / 100.0f, 2);
  } else {
    Serial.println("ERRO");
  }

  Serial.print("Temp externa (C): ");
  if (dhtExternoValido) {
    Serial.println(p.tempExterna_x100 / 100.0f, 2);
  } else {
    Serial.println("ERRO");
  }

  Serial.print("Umid externa (%): ");
  if (dhtExternoValido) {
    Serial.println(p.umidExterna_x100 / 100.0f, 2);
  } else {
    Serial.println("ERRO");
  }
}