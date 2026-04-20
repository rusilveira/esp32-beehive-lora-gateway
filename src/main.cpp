#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// ==========================
// PINOS LoRa (VALIDADOS)
// ==========================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_DIO0  26
#define LORA_RST   14

SPIClass spiLoRa(VSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, RADIOLIB_NC, spiLoRa);

// ==========================
// PROTOCOLO
// ==========================
constexpr uint8_t MAGIC_BYTE    = 0xA5;
constexpr uint8_t PROTO_VERSION = 0x01;

constexpr uint8_t MSG_DATA = 0x10;
constexpr uint8_t MSG_ACK  = 0x20;

constexpr uint8_t GATEWAY_ID = 0xF0;

// ==========================
// PAYLOAD REAL DA COLMEIA
// ==========================
struct PayloadColmeia {
  int16_t tempInterna_x100;
  uint16_t umidInterna_x100;

  int16_t tempExterna_x100;
  uint16_t umidExterna_x100;

  int32_t peso_x1000;

  uint8_t flags;
  uint8_t reservado;
};

// ==========================
// CRC16
// ==========================
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

// ==========================
// VALIDA PACOTE DATA
// ==========================
bool validPacket(uint8_t* buf, size_t len) {
  if (len < 8) return false;

  uint16_t crc_rx = (buf[len - 2] << 8) | buf[len - 1];
  uint16_t crc_calc = crc16(buf, len - 2);

  return (
    crc_rx == crc_calc &&
    buf[0] == MAGIC_BYTE &&
    buf[1] == PROTO_VERSION &&
    buf[2] == MSG_DATA
  );
}

// ==========================
// MONTA ACK
// ==========================
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

// ==========================
// INIT RADIO
// ==========================
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

// ==========================
// CONTROLE DE DUPLICIDADE
// ==========================
uint8_t lastSeq = 255;

// ==========================
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("=== RX GATEWAY ===");
  initRadio();
}

// ==========================
void loop() {
  uint8_t buf[64];

  int state = radio.receive(buf, sizeof(buf), 1000);

  if (state != RADIOLIB_ERR_NONE) return;

  size_t len = radio.getPacketLength();

  if (!validPacket(buf, len)) return;

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

  bool dhtInternoValido = p.flags & (1 << 0);
  bool dhtExternoValido = p.flags & (1 << 1);

  Serial.println();
  Serial.print("RX node=");
  Serial.print(nodeId);
  Serial.print(" seq=");
  Serial.println(seq);

  Serial.print("Peso (kg): ");
  Serial.println(p.peso_x1000 / 1000.0f, 3);

  Serial.print("Temp interna (C): ");
  if (dhtInternoValido) Serial.println(p.tempInterna_x100 / 100.0f, 2);
  else Serial.println("ERRO");

  Serial.print("Umid interna (%): ");
  if (dhtInternoValido) Serial.println(p.umidInterna_x100 / 100.0f, 2);
  else Serial.println("ERRO");

  Serial.print("Temp externa (C): ");
  if (dhtExternoValido) Serial.println(p.tempExterna_x100 / 100.0f, 2);
  else Serial.println("ERRO");

  Serial.print("Umid externa (%): ");
  if (dhtExternoValido) Serial.println(p.umidExterna_x100 / 100.0f, 2);
  else Serial.println("ERRO");
}