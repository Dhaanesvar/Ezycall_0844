// RAK3112_TTN_Downlink.ino
#include <RadioLib.h>

// ============================================================
// RAK3112 / SX1262 PIN MAP (WisBlock Interface)
// ============================================================
#define LORA_NSS       7
#define LORA_SCK       5
#define LORA_MOSI      6
#define LORA_MISO      3
#define LORA_RESET     8
#define LORA_BUSY      48
#define LORA_DIO1      47

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RESET, LORA_BUSY);
// ============================================================

// TTN Credentials
uint64_t joinEUI = 0x0000000000001000;
uint64_t devEUI = 0xAC1F09FFFE2402B3;
uint8_t nwkKey[] = { 0x45, 0xA8, 0xC1, 0x7F, 0xDB, 0xCF, 0x60, 0xAD, 0x78, 0xA1, 0x24, 0xB5, 0x9C, 0xDE, 0xEC, 0x13 };
uint8_t appKey[] = { 0x45, 0xA8, 0xC1, 0x7F, 0xDB, 0xCF, 0x60, 0xAD, 0x78, 0xA1, 0x24, 0xB5, 0x9C, 0xDE, 0xEC, 0x13 };

const LoRaWANBand_t Region = AS923;
const uint8_t subBand = 0;

LoRaWANNode node(&radio, &Region, subBand);

// ============================================================
// Use Serial (UART0) to talk to Pico – this is the same as Serial0
// Pins: J6-7 (RX) and J6-8 (TX)
// ============================================================
// We'll use Serial0 explicitly. Note: Serial (USB) is also called Serial.
// To avoid confusion, we define PICO_SERIAL as Serial0 (which is the hardware UART0).
#define PICO_SERIAL Serial0
#define UART_BAUD 115200

const uint32_t uplinkIntervalSeconds = 30;
uint32_t lastUplinkTime = 0;

uint8_t downlinkData[64];
size_t downlinkLen = 0;

void setup() {
  // Initialize both USB debug (Serial) and hardware UART0 (Serial0)
  Serial.begin(115200);   // USB debug (connected to computer)
  PICO_SERIAL.begin(UART_BAUD); // hardware UART0 on J6-7 (RX) and J6-8 (TX)
  delay(5000);
  
  Serial.println(F("\n[INFO] RAK3112 TTN Downlink Bridge Started"));
  PICO_SERIAL.println(F("RAK3112 bridge started")); // optional boot message to Pico

  // SPI and radio init (same as before)
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  int16_t state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[ERROR] Radio init failed, code: "));
    Serial.println(state);
    while (true);
  }
  Serial.println(F("[OK] Radio initialized"));
  radio.setDio2AsRfSwitch(true);
  radio.setTCXO(1.6);
  Serial.println(F("[OK] SX1262 configured"));

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[ERROR] Node init failed, code: "));
    Serial.println(state);
    while (true);
  }

  Serial.print(F("[INFO] Joining TTN..."));
  state = node.activateOTAA();
  if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.print(F(" [FAILED] Code: "));
    Serial.println(state);
    while (true);
  }
  Serial.println(F(" [SUCCESS]"));

  lastUplinkTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastUplinkTime >= (uplinkIntervalSeconds * 1000UL)) {
    lastUplinkTime = currentMillis;

    const char* uplinkMsg = "keepAlive";
    uint8_t uplinkPayload[strlen(uplinkMsg)];
    memcpy(uplinkPayload, uplinkMsg, strlen(uplinkMsg));
    size_t uplinkLen = strlen(uplinkMsg);

    Serial.print(F("[UPLINK] Sending keep-alive..."));

    int16_t state = node.sendReceive(uplinkPayload, uplinkLen, 1, downlinkData, &downlinkLen);

    if (state < RADIOLIB_ERR_NONE) {
      Serial.print(F(" [FAIL] Error code: "));
      Serial.println(state);
    } else if (downlinkLen > 0) {
      Serial.println(F(" [OK] Downlink received!"));
      
      // Convert to String and forward to Pico via hardware UART0
      String cmd = String((char*)downlinkData);
      cmd.trim();
      PICO_SERIAL.println(cmd);   // This goes to J6-8 TX -> Pico GP1

      Serial.print(F("[DOWNLINK] Forwarding to Pico: "));
      Serial.println(cmd);
      
      downlinkLen = 0;
    } else {
      Serial.println(F(" [OK] No downlink received"));
    }
  }
}