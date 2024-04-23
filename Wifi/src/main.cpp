#include "WiFi.h"

const char *ssid = "Your_SSID";  // Name of your network
const char *password = "Your_Password";  // Password of your network

void setup() {
  Serial.begin(9600);
  // Setting the ESP32 as an access point
  WiFi.softAP(ssid, password);
    pinMode(14, OUTPUT);
  digitalWrite(14, LOW);



  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Nothing to do here
}