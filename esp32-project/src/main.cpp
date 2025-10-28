#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>


/*
  WiFi Setings
*/
const char* ssid = " ?PLACEHOLDER? ";
const char* password = " ?PLACEHOLDER? ";

WebServer server(1337);

/*
  App ~ HTML
*/
const char* htmlContent = R"rawliteral( ?PLACEHOLDER? )rawliteral";

/*
  Globals
*/

/*
  Setup
*/
void handleRoot() { server.send(200, "text/html", htmlContent); }
void setup()
{
  Serial.begin(115200);
}
void loop() { server.handleClient(); }