#include <M5StickC.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <Preferences.h>

#define PIN_SEN 26

const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "M5STACK_SETUP";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;

// DNSServer dnsServer;
WebServer webServer(80);

// wifi config store
Preferences preferences;

void setup() {
  Serial.begin(19200);
  m5.begin();
  preferences.begin("wifi-config");

  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();
  // Enaable RIP Senser
  pinMode(PIN_SEN, INPUT);
}

void loop() {
  loggerln("loop");
  if (settingMode) {
    return;
  }
  webServer.handleClient();
  // RIP Sensor
  checkRIPSensor();
  delay(1000);
}

boolean checkRIPSensor() {
  if(digitalRead(PIN_SEN)) {
    loggerln("State:High");
    return true;
  } else {
    loggerln("State:Low");
    return false;
  }
}

boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");
  loggerln("WIFI-SSID: ");
  loggerln(wifi_ssid);
  loggerln("WIFI-PASSWD: ");
  loggerln("****");
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if(wifi_ssid.length() > 0) {
    return true;
} else {
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  logger("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      loggerln("");
      loggerln("Connected!");
      return (true);
    }
    delay(500);
    logger(".");
    count++;
  }
  loggerln("Timed out.");
  return false;
}

void startWebServer() {
  if (settingMode) {
    logger("Starting Web Server at ");
    loggerln(WiFi.softAPIP().toString());
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      String ssid = urlDecode(webServer.arg("ssid"));
      logger("SSID: ");
      loggerln(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      logger("Password: ");
      loggerln(pass);
      loggerln("Writing SSID to EEPROM...");

      // Store wifi config
      loggerln("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);

      loggerln("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    logger("Starting Web Server at ");
    loggerln(WiFi.localIP().toString());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  loggerln("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);
  startWebServer();
  logger("Starting Access Point at \"");
  logger(apSSID);
  loggerln("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

void logger(String message) {
  Serial.print(message);
  M5.Lcd.print(message);
}

void loggerln(String message) {
  Serial.println(message);
  M5.Lcd.println(message);
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
