/*
Pins

  D0 = 16
  D1 = 5 - Dimmer 1
  D2 = 4 - Dimmer 2
  D3 = Not used
  D4 =  Not used (BUILTIN_LED)
  D5 = 14 - Dimmer 3
  D6 = 12 - Dimmer 4
  D7 = 13 - Dimmer 5
  D8 = 15


WiFi.status() =
  0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
  3 : WL_CONNECTED after successful connection is established
  4 : WL_CONNECT_FAILED if password is incorrect
  6 : WL_DISCONNECTED if module is not configured in station mode
*/

#include <Arduino.h>

// ---------------------------------------- WiFi ----------------------------------------
#include <ESP8266WiFi.h>

const char* WiFi_SSID = "NoInternetHere";
const char* WiFi_Password = "NoPassword1!";
String WiFi_Hostname = "Dimmer";

unsigned long WiFi_Timeout = 5000; // ms before marking connection as failed
unsigned long WiFi_Reconnect_At = 0;
unsigned long WiFi_Reconnect_Delay = 10000; // ms before marking connection as failed

const int Dimmer_Number_Of = 5;
const int Dimmer_Pin[Dimmer_Number_Of] {D1, D2, D5, D6, D7};

byte Dimmer_Strenght[Dimmer_Number_Of];




// ---------------------------------------- ??? ----------------------------------------
WiFiServer server(80);
WiFiClient client;


// ---------------------------------------- HTTP_GET() ----------------------------------------
String HTTP_GET(String Server, int Port, String URL) {

  // Attempt to make a connection to the remote server
  if ( !client.connect(Server, Port) ) {
    return "Connection failed";
  }

  // This will send the request to the server
  client.print(String("GET /") + URL + " HTTP/1.1\r\n" +
               "Host: " + Server + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(50);

  // Read all the lines of the reply from server and print them to Serial
  String Reply;
  while(client.available()){
    Reply += client.readStringUntil('\r');
  }

  return Reply;
}


// ---------------------------------------- Web_Server() ----------------------------------------
void Web_Server() {

  if (server.status() == false) {
    if (WiFi.status() == 3) {
      // Start the server
      server.begin();
      Serial.println("Server started");

      // Print the IP address
      Serial.print("Use this URL : ");
      Serial.print("http://");
      Serial.print(WiFi.localIP());
      Serial.println("/");
    }
  }

  else if (server.status() == true) {
    if (WiFi.status() != 3) {
        server.stop();
    }
  }
} // Web_Server()



// ---------------------------------------- WiFi_Reset() ----------------------------------------
void WiFi_Reset() {
  Serial.println("Erasing WiFi config");
  ESP.eraseConfig();
  delay(1500);


  Serial.println("Configuring WiFi");
  WiFi.enableAP(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.softAPdisconnect(true);
  WiFi.hostname(WiFi_Hostname);
}


// ---------------------------------------- WiFi_Connect() ----------------------------------------
bool WiFi_Connect(const char* SSID, const char* Password) {

  byte WiFi_State = WiFi.status();

  // 3 : WL_CONNECTED after successful connection is established
  // Aparantly someone wanted to reconnecto so resetting and connecting again
  if (WiFi_State == 3) {
    Serial.print("Disconnecting from: ");
    Serial.println(WiFi.SSID());
    WiFi_Reset();
  }

  Serial.print("Current WiFi status: ");
  Serial.println(WiFi.status());

  Serial.print("Connecting to: ");
  Serial.print(SSID);
  Serial.print(" ");

  unsigned long WiFi_Connect_Start = millis() + WiFi_Timeout;

  WiFi.begin(SSID, Password);

  while (WiFi.status() != 3) {
    if (WiFi_Connect_Start < millis()) {
      Serial.println(" connection timeout.");
      return false;
    }
    Serial.print(".");
    delay (500);
  }

  Serial.println(" connected.");
  Web_Server();
  return true;




} // WiFi_Connect

// ---------------------------------------- WiFi_Check() ----------------------------------------

void WiFi_Check() {


  if (WiFi_Reconnect_At > millis()) { // Wait for timer to expire
    return;
  }

  byte WiFi_State = WiFi.status();

  //   0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  if (WiFi_State == 0) {
    Serial.println("WiFi Idle");
    if (WiFi.gatewayIP().toString() == "0.0.0.0") { // No gateway assuming WiFi has lost connection
      Serial.println("No gateway, assuming WiFi has lost connection, resetting WiFi.");
      WiFi_Reset();
      WiFi_Connect(WiFi_SSID, WiFi_Password);
    }
  }

  // 1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
  else if (WiFi_State == 1) {
    Serial.print("SSIID: ");
    Serial.print(WiFi.SSID());
    Serial.println(" - Out of range.");
  }

  // 3 : WL_CONNECTED after successful connection is established
  else if (WiFi_State == 3) {

    if (WiFi.gatewayIP().toString() == "0.0.0.0") { // No gateway assuming WiFi has lost connection
      Serial.println("No gateway, assuming WiFi has lost connection, resetting WiFi.");
      WiFi_Reset();
      WiFi_Connect(WiFi_SSID, WiFi_Password);
    }

    else {
      Serial.println("WiFi Connected.");
      Web_Server();
    }

  }

  // 4 : WL_CONNECT_FAILED if password is incorrect
  else if (WiFi_State == 4) {
    Serial.print("Password incorrect for SSID: ");
    Serial.println(WiFi.SSID());
    WiFi_Reset();
    WiFi_Connect(WiFi_SSID, WiFi_Password);
  }

  // 6 : WL_DISCONNECTED if module is not configured in station mode
  else if (WiFi_State == 6) {
    Serial.println("WiFi not configured");
    WiFi_Connect(WiFi_SSID, WiFi_Password);
  }

  else {
    Serial.print("Current WiFi status: ");
    Serial.println(WiFi.status());
  }

  WiFi_Reconnect_At = millis() + WiFi_Reconnect_Delay;
}


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Booting");

  for (byte i = 0; i < Dimmer_Number_Of; i++) {
    pinMode(Dimmer_Pin[i], OUTPUT);
    analogWrite(Dimmer_Pin[i], 0);
    Dimmer_Strenght[i] = 0;
  }

  WiFi_Check();

  Serial.println("Boot done");
}

void loop() {

  WiFi_Check();


  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  client.flush();

  // Match the request

  if (request.indexOf("GET /Dimmer_") != -1) {

    unsigned int Light_Strength = 0;
    unsigned int Light_Strength_Procent = 0;

    request.replace("GET /", "");
    request.replace(" HTTP/1.1", "");

    String Temp_String = request.substring(0, request.indexOf("-"));

    Temp_String.replace("Dimmer_", "");

    int Selected_Dimmer = Temp_String.toInt();

    Light_Strength_Procent = request.substring(request.indexOf("-") + 1, request.length()).toInt();

    Light_Strength = (Light_Strength_Procent * 0.01) * 1023;


    if (Selected_Dimmer <= Dimmer_Number_Of) {
      analogWrite(Dimmer_Pin[Selected_Dimmer - 1], Light_Strength);
      Dimmer_Strenght[Selected_Dimmer - 1] = Light_Strength_Procent;
      Serial.print("Dimmer ");
      Serial.print(Selected_Dimmer);
      Serial.print(": ");
      Serial.println(Dimmer_Strenght[Selected_Dimmer]);
    }

    else if (Selected_Dimmer == 99) {
      for (byte i = 0; i < Dimmer_Number_Of; i++) {
          analogWrite(Dimmer_Pin[i], 0);
          Dimmer_Strenght[i] = 0;
      }
      Serial.println("All OFF");
    }

  }

  else Serial.println(request);

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.println("Wemos Dimmer v2.0");
  client.println("<br><br>");

  for (byte i = 1; i <= Dimmer_Number_Of; i++) {
    client.print("Dimmer ");
    client.print(i);
    client.print(": ");
    client.print(Dimmer_Strenght[i - 1]);
    client.println("&#37<br>");
  }

  client.println("</html>");

  delay(1);

}
