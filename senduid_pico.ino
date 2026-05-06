#include <W55RP20_Ethernet3.h>

// Default values
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress myIP(13, 22, 0, 213);
IPAddress subnet(255, 0, 0, 0);
IPAddress gateway(13, 22, 0, 1);

IPAddress ledIP(13, 22, 255, 15);
const int ledPort = 1884;

String UID = "0d-16";
String input = "";

bool ethernetNeedsRestart = false;

void setup() {
  Serial2.begin(115200, SERIAL_8N1);
  delay(100);
  
  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);
  delay(100);
  
  startEthernet();
  
  Serial2.println("W55RP20 Ready.");
  Serial2.println("");
  Serial2.println("=== COMMANDS ===");
  Serial2.println("GETUID      - Show current MAC, IP, UID");
  Serial2.println("SETUID xxxx - Set new UID");
  Serial2.println("SETIP x.x.x.x - Set new IP address");
  Serial2.println("RESTART     - Restart Ethernet");
  Serial2.println("SEND |SEVT|... - Send command to LED");
  Serial2.println("===========================");
  Serial2.println("");
}

void startEthernet() {
  Ethernet.begin(mac, myIP, gateway, subnet);
  delay(1000);
  Serial2.print("Ethernet started. IP: ");
  Serial2.println(Ethernet.localIP());
}

void restartEthernet() {
  Serial2.println("Restarting Ethernet...");
  Ethernet.begin(mac, myIP, gateway, subnet);
  delay(1000);
  Serial2.print("New IP: ");
  Serial2.println(Ethernet.localIP());
  ethernetNeedsRestart = false;
}

void loop() {
  if (ethernetNeedsRestart) {
    restartEthernet();
  }
  
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (input.length() > 0) {
        processCommand(input);
        input = "";
      }
    } else {
      input += c;
    }
  }
}

void processCommand(String cmd) {
  cmd.trim();
  
  // GETUID - Show current UID
  if (cmd.equalsIgnoreCase("GETUID")) {
    Serial2.println("");
    Serial2.println("=== CURRENT UID ===");
    Serial2.print("MAC Address: ");
    for(int i = 0; i < 6; i++) {
      if(mac[i] < 16) Serial2.print("0");
      Serial2.print(mac[i], HEX);
      if(i < 5) Serial2.print(":");
    }
    Serial2.println();
    Serial2.print("IP Address: ");
    Serial2.println(myIP);
    Serial2.print("Subnet Mask: ");
    Serial2.println(subnet);
    Serial2.print("Gateway: ");
    Serial2.println(gateway);
    Serial2.print("UID: ");
    Serial2.println(UID);
    Serial2.println("=================");
    Serial2.println("");
    return;
  }
  
  // SETUID xxxx - Set new Company UID
  if (cmd.startsWith("SETUID ")) {
    UID = cmd.substring(7);
    Serial2.print("UID set to: ");
    Serial2.println(UID);
    return;
  }
  
  // SETIP x.x.x.x - Set new IP address
  if (cmd.startsWith("SETIP ")) {
    String ipStr = cmd.substring(6);
    int parts[4];
    int part = 0;
    int val = 0;
    
    for(int i = 0; i < ipStr.length() && part < 4; i++) {
      if(ipStr[i] == '.') {
        parts[part++] = val;
        val = 0;
      } else if(ipStr[i] >= '0' && ipStr[i] <= '9') {
        val = val * 10 + (ipStr[i] - '0');
      }
    }
    parts[part] = val;
    
    if(part == 3) {
      myIP = IPAddress(parts[0], parts[1], parts[2], parts[3]);
      Serial2.print("IP address set to: ");
      Serial2.println(myIP);
      ethernetNeedsRestart = true;
    } else {
      Serial2.println("Invalid IP format. Use: SETIP 13.22.0.213");
    }
    return;
  }
  
  // RESTART - Restart Ethernet
  if (cmd.equalsIgnoreCase("RESTART")) {
    restartEthernet();
    return;
  }
  
  // SEND command - Forward to LED display
  if (cmd.startsWith("SEND ")) {
    String sevtCommand = cmd.substring(5);
    EthernetClient client;
    if (client.connect(ledIP, ledPort)) {
      client.print(sevtCommand);
      client.stop();
      Serial2.print("Sent to LED: ");
      Serial2.println(sevtCommand);
    } else {
      Serial2.println("Failed to connect to LED.");
    }
    return;
  }
  
  // Unknown command
  Serial2.print("Unknown command: ");
  Serial2.println(cmd);
  Serial2.println("Type GETUID, SETUID, SETIP, RESTART, or SEND <command>");
}
