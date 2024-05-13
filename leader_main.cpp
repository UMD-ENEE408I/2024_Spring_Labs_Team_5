#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "RobotWifi";
const char* password = "passwordpassword";
const char* host = "192.168.4.2";
const uint16_t port = 8080;
const char* message = "leader:checkin";


void setup() {
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);  
  bool found_host = false;
  bool host_1 = true;
  Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  delay(5000);

  while(!found_host){
    if(host_1){
      WiFiClient client;
      Serial.println("Searching for Host on 192.168.4.2");
      if (client.connect("192.168.4.2", port)) {
        Serial.println("Found central control station");
        client.println(message);
        found_host = true;
        client.stop();
      } else {
      Serial.println("Not found on 192.168.4.2");
        host_1 = !host_1;
      }
      delay(100);
    }
    else{
      WiFiClient client;
      Serial.println("Searching for Host on 192.168.4.4");
      if (client.connect("192.168.4.4", port)) {
        Serial.println("Found central control station");
        client.println(message);
        found_host = true;
        host = "192.168.4.4";
        client.stop();
      } else {
      Serial.println("Not found on 192.168.4.2");
        host_1 = !host_1;
      }
      delay(100);
    }
  }

}

std::string extractMessage(const std::string& input) {
    std::string prefix = "leader:";
    size_t pos = input.find(prefix);
    if (pos != std::string::npos) {
        size_t commaPos = input.find(',', pos);
        if (commaPos != std::string::npos) {
            return input.substr(pos + prefix.length(), commaPos - (pos + prefix.length()));
        }
    }
    return "";
}

void loop() {
  WiFiClient client;
  if (client.connect(host, port)) {
    client.println(message);
    String response = client.readStringUntil('|');
    Serial.println(response);
    Serial.println("-------------- ^^ Something Recieved ^^ -----------");
    Serial.println("Connected to central control station");
    client.stop();
    Serial.println("\nMessage sent to central control station:");
    Serial.println(message);
    std::string recd_message = extractMessage(response.c_str());
    Serial.println("Recieved / Extracted Message: ");
    Serial.println(recd_message.c_str());
    if (recd_message == "begin"){
      message = "leader:audio";
    }
    else if (recd_message == "reset"){
      message = "leader:checkin";
    }
  } else {
    Serial.println("Connection to central control station failed");
  }


  delay(2000);
}
