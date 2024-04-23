#include <WiFi.h>

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *host = "SERVER_IP_ADDRESS"; // The IP address of the NVIDIA AGX Xavier
const uint16_t port = 12345;

void setup()
{
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
}

void loop()
{
  WiFiClient client;

  if (!client.connect(host, port))
  {
    Serial.println("Connection to host failed");
    delay(1000);
    return;
  }

  Serial.println("Connected to server");
  client.println("Hello from Heltec WiFi Kit 32!");

  while (client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println("Disconnecting...");
  client.stop();

  delay(2000);
}