#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include "arduino_secrets.h"

/////// WiFi Settings ///////
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password

char serverAddress[] = "158.179.207.196";  // server address
int port = 8080;

WiFiClient wifi;
WebSocketClient client = WebSocketClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

void sendLargeValue(int value) {
  byte highByte = value >> 8;   // Shift right 8 bits to get the high byte
  byte lowByte = value & 0xFF;  // Mask with 0xFF to get the low byte

  Serial1.write(highByte);
  Serial1.write(lowByte);
}

void sendData(int hue0, int brightness0, int hue1, int brightness1) {
  Serial1.write(0xFF);  // Data message header

  sendLargeValue(hue0);
  Serial1.write(brightness0);
  sendLargeValue(hue1);
  Serial1.write(brightness1);
}

void sendMode(byte mode) {
  Serial1.write(0xFE);  // Mode message header
  Serial1.write(mode);
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200); // Initialize Serial1

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  Serial.println("---------------------------------------");
  Serial.println("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println("---------------------------------------");
}

void loop() {
  Serial.println("Starting WebSocket client");
  client.begin();

  while (client.connected()) {
    int messageSize = client.parseMessage();

    if (messageSize > 0) {
      Serial.println("Received a message:");
      String message = client.readString();
      Serial.println(message);

      // Check if the message is for mode change
      if (message.length() == 1 && message[0] >= '0' && message[0] <= '3') {
        // It's a mode message
        byte mode = message.toInt();
        sendMode(mode);
      } else {
        // It's a color and brightness message
        int hue0, brightness0, hue1, brightness1;
        int partsIndex = 0;
        String parts[4];
        for (int i = 0; i < message.length(); i++) {
          if (message.charAt(i) == ',') {
            partsIndex++;
          } else {
            parts[partsIndex] += message.charAt(i);
          }
        }

        // Convert the string parts to integers
        hue0 = parts[0].toInt();
        brightness0 = parts[1].toInt();
        hue1 = parts[2].toInt();
        brightness1 = parts[3].toInt();

        // Send the parsed values
        sendData(hue0, brightness0, hue1, brightness1);
      }
    }

    // Wait for some time before sending the next message
    delay(5);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect");
    WiFi.begin(ssid, pass);
  }

  if (!client.connected()) {
    Serial.println("Disconnected, attempting to reconnect");
    client.begin();
  }
}
