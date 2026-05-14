#include <W55RP20_Ethernet3.h>
#include <EEPROM.h>

#define LED_PIN 25

// Default Ethernet configuration
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress defaultIP(13, 22, 0, 213);
IPAddress gateway(13, 22, 0, 1);
IPAddress subnet(255, 0, 0, 0);
IPAddress ledIP(13, 22, 255, 15);
const int ledPort = 1884;

// EEPROM addresses
const int uidAddr = 0;
const int ipAddr = 32;
const int uidMaxLength = 32;

String companyUID = "";
IPAddress myIP = defaultIP;
bool ethernetNeedsRestart = false;
bool ethernetOk = false;

// Helper functions
String ipToString(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

// Load / save UID
void loadUID() {
  String uid = "";
  for (int i = 0; i < uidMaxLength; i++) {
    char c = EEPROM.read(uidAddr + i);
    if (c == 0) break;
    uid += c;
  }
  companyUID = (uid.length() > 0) ? uid : "0d-16";
}

void saveUID() {
  for (int i = 0; i < companyUID.length(); i++)
    EEPROM.write(uidAddr + i, companyUID[i]);
  EEPROM.write(uidAddr + companyUID.length(), 0);
  EEPROM.commit();
}

// Load / save IP
void loadIP() {
  uint8_t b[4];
  for (int i = 0; i < 4; i++) b[i] = EEPROM.read(ipAddr + i);
  if (b[0] != 0 || b[1] != 0 || b[2] != 0 || b[3] != 0)
    myIP = IPAddress(b[0], b[1], b[2], b[3]);
  else
    myIP = defaultIP;
}

void saveIP() {
  for (int i = 0; i < 4; i++) EEPROM.write(ipAddr + i, myIP[i]);
  EEPROM.commit();
}

// Ethernet functions
void startEthernet() {
  Ethernet.begin(mac, myIP, gateway, subnet);
  delay(1000);
  IPAddress ip = Ethernet.localIP();
  if (ip != INADDR_NONE && ip != IPAddress(255,255,255,255)) {
    ethernetOk = true;
    String msg = "Ethernet started. IP: " + ipToString(ip);
    Serial.println(msg);
    Serial1.println(msg);
  } else {
    ethernetOk = false;
    String msg = "Ethernet init failed. Check cable?";
    Serial.println(msg);
    Serial1.println(msg);
  }
}

void restartEthernet() {
  Serial1.println("Restarting Ethernet...");
  startEthernet();
  ethernetNeedsRestart = false;
}

// Forward a SEND command to LED display
void forwardToLED(String sevtCommand, String source) {
  if (!ethernetOk) {
    Serial1.print("[ERROR] Ethernet not available, cannot forward ");
    Serial1.println(source);
    return;
  }
  EthernetClient client;
  if (client.connect(ledIP, ledPort)) {
    client.print(sevtCommand);
    client.stop();
    Serial1.print("[OK] Forwarded to LED (");
    Serial1.print(source);
    Serial1.print("): ");
    Serial1.println(sevtCommand);
  } else {
    Serial1.print("[FAIL] Could not connect to LED at ");
    Serial1.print(ipToString(ledIP));
    Serial1.print(":");
    Serial1.println(ledPort);
  }
}

// Process manual commands from YAT (Serial1)
void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("GETUID")) {
    Serial1.println("\n=== CURRENT UID ===");
    Serial1.print("MAC: ");
    for (int i = 0; i < 6; i++) {
      if (mac[i] < 16) Serial1.print("0");
      Serial1.print(mac[i], HEX);
      if (i < 5) Serial1.print(":");
    }
    Serial1.println();
    Serial1.print("IP: "); Serial1.println(ipToString(myIP));
    Serial1.print("UID: "); Serial1.println(companyUID);
    Serial1.println("=================");
    return;
  }

  if (cmd.startsWith("SETUID ")) {
    companyUID = cmd.substring(7);
    saveUID();
    Serial1.print("UID set to: "); Serial1.println(companyUID);
    return;
  }

  if (cmd.startsWith("SETIP ")) {
    String ipStr = cmd.substring(6);
    int parts[4], part = 0, val = 0;
    for (int i = 0; i < ipStr.length() && part < 4; i++) {
      if (ipStr[i] == '.') { parts[part++] = val; val = 0; }
      else if (isDigit(ipStr[i])) val = val * 10 + (ipStr[i] - '0');
    }
    parts[part] = val;
    if (part == 3) {
      myIP = IPAddress(parts[0], parts[1], parts[2], parts[3]);
      saveIP();
      Serial1.print("IP set to: "); Serial1.println(ipToString(myIP));
      ethernetNeedsRestart = true;
    } else {
      Serial1.println("Invalid IP. Use: SETIP 13.22.0.213");
    }
    return;
  }

  if (cmd.equalsIgnoreCase("RESTART")) {
    restartEthernet();
    return;
  }

  // Manual SEND command from YAT
  if (cmd.startsWith("SEND ")) {
    String sevt = cmd.substring(5);
    // Parse LED code for logging
    int count = 0, fifth = -1, sixth = -1;
    for (int i = 0; i < cmd.length(); i++) {
      if (cmd[i] == '|') {
        count++;
        if (count == 5) fifth = i;
        if (count == 6) { sixth = i; break; }
      }
    }
    if (fifth != -1 && sixth != -1) {
      String ledCode = cmd.substring(fifth + 1, sixth);
      ledCode.trim();
      Serial1.print("[Manual] LED code: ");
      Serial1.println(ledCode);
      forwardToLED(sevt, "manual");   // Forward only if well‑formed
    } else {
      Serial1.println("[Manual] Malformed SEND command – NOT forwarded");
    }
    return;
  }

  Serial1.print("Unknown command: "); Serial1.println(cmd);
}

void setup() {
  // USB debug (optional, you can ignore)
  Serial.begin(115200);
  // YAT on Serial1 (GP0 TX, GP1 RX) – both input and output
  Serial1.begin(115200);
  // RAK on Serial2 (GP9 RX, GP8 TX not used)
  Serial2.setRX(9);
  Serial2.setTX(8);
  Serial2.begin(115200);

  delay(100);
  EEPROM.begin(512);
  loadUID();
  loadIP();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // Always ON

  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);
  delay(100);

  startEthernet();

  Serial1.println("Pico Ready. Listening on Serial1 (YAT commands) and Serial2 (RAK).");
  Serial1.println("Commands: GETUID, SETUID xxxx, SETIP x.x.x.x, RESTART, SEND |SEVT|...");
}

void loop() {
  // Restart Ethernet if needed
  if (ethernetNeedsRestart) restartEthernet();

  // Read manual commands from YAT (Serial1)
  static String cmdLine = "";
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (cmdLine.length() > 0) {
        processCommand(cmdLine);
        cmdLine = "";
      }
    } else {
      cmdLine += c;
    }
  }

  // Read automatic commands from RAK (Serial2)
  while (Serial2.available()) {
    String rakCmd = Serial2.readStringUntil('\n');
    rakCmd.trim();
    if (rakCmd.length() == 0) continue;

    // Echo to YAT so you see what RAK sent
    Serial1.print("[RAK] ");
    Serial1.println(rakCmd);

    if (rakCmd.startsWith("SEND ")) {
      // Parse LED code and forward only if well‑formed
      int count = 0, fifth = -1, sixth = -1;
      for (int i = 0; i < rakCmd.length(); i++) {
        if (rakCmd[i] == '|') {
          count++;
          if (count == 5) fifth = i;
          if (count == 6) { sixth = i; break; }
        }
      }
      if (fifth != -1 && sixth != -1) {
        String ledCode = rakCmd.substring(fifth + 1, sixth);
        ledCode.trim();
        Serial1.print("[RAK] LED code: ");
        Serial1.println(ledCode);
        String sevt = rakCmd.substring(5); // remove "SEND "
        forwardToLED(sevt, "RAK");
      } else {
        Serial1.println("[RAK] Malformed SEND command – NOT forwarded");
      }
    } else {
      Serial1.print("[RAK] Ignored (not a SEND command): ");
      Serial1.println(rakCmd);
    }
  }
}