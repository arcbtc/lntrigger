

//////////////LOAD LIBRARIES////////////////

#include <M5Stack.h>
#include "FS.h"
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "SPIFFS.h"

/////////////////LOAD SPLASH///////////////////

#include "image.c"

/////////////////SOME DEFINES///////////////////

#define LED_PIN 25
  
/////////////////SOME VARIABLES///////////////////

char lnbits_server[40] = "lnbits.com";
char invoice_key[500] = "";
char lnbits_description[100] = "";
char lnbits_amount[500] = "1000";
char high_pin[5] = "16";
char time_pin[20] = "3000";
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";
String payReq = "";
String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;

const char* spiffcontent = "";
String spiffing; 


/////////////////////SETUP////////////////////////

void setup() {
  M5.begin();
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)logo_map);
  delay(3000);
  lnbits_screen();

// START PORTAL 

  portal();
}

///////////////////MAIN LOOP//////////////////////

void loop() {
  pinMode (atoi(high_pin), OUTPUT);
  digitalWrite(atoi(high_pin), LOW);
  if(String(invoice_key) == ""){
    error_screen();
  }
  Serial.println(String(invoice_key));
  getinvoice();

  if(down){
  error_screen();
  getinvoice();
  delay(5000);
  }
  if(payReq != ""){
   qrdisplay_screen();
   delay(5000);
  }
  while(paid == false && payReq != ""){
    checkinvoice();
    if(paid){
      complete_screen();
      digitalWrite(atoi(high_pin), HIGH);
      delay(atoi(time_pin));
      digitalWrite(atoi(high_pin), LOW); 
    }
    delay(3000);
  }
  payReq = "";
  dataId = "";
  paid = false;
  delay(4000);
}


//////////////////DISPLAY///////////////////


void processing_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
}

void lnbits_screen()
{ 
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.println("POWERED BY LNBITS");
}

void portal_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(30, 80);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PORTAL LAUNCHED");
}

void complete_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(60, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("COMPLETE");
}

void error_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(70, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("ERROR");
}

void qrdisplay_screen()
{  
  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payReq,45,0,240,14);
  delay(100);
}

//////////////////NODE CALLS///////////////////

void getinvoice() {
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbits_server;
  const char* invoicekey = invoice_key;
  const char* lnbitsamount = lnbits_amount;
  const char* lnbitsdescription = lnbits_description;

  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String topost = "{\"out\": false,\"amount\" : " + String(lnbitsamount) + ", \"memo\" :\""+ String(lnbitsdescription) + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  client.print(String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ invoicekey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* payment_hash = doc["checking_id"];
  const char* payment_request = doc["payment_request"];
  payReq = payment_request;
  dataId = payment_hash;
}


void checkinvoice(){
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbits_server;
  const char* invoicekey = invoice_key;
  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String url = "/api/v1/payments/";
  client.print(String("GET ") + url + dataId +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
   while (client.connected()) {
   String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  bool charPaid = doc["paid"];
  paid = charPaid;
}

void portal(){

  WiFiManager wm;
  Serial.println("mounting FS...");
  while(!SPIFFS.begin(true)){
    Serial.println("failed to mount FS");
    delay(200);
   }

//CHECK IF RESET IS TRIGGERED/WIPE DATA
  for (int i = 0; i <= 100; i++) {
    if (M5.BtnA.wasPressed()){
    portal_screen();
    delay(1000);
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    file.print("placeholder");
    wm.resetSettings();
    i = 100;
    }
    delay(50);
    M5.update();
  }

//MOUNT FS AND READ CONFIG.JSON
  File file = SPIFFS.open("/config.txt");
  
  spiffing = file.readStringUntil('\n');
  spiffcontent = spiffing.c_str();
  DynamicJsonDocument json(1024);
  deserializeJson(json, spiffcontent);
  if(String(spiffcontent) != "placeholder"){
    strcpy(lnbits_server, json["lnbits_server"]);
    strcpy(lnbits_description, json["lnbits_description"]);
    strcpy(invoice_key, json["invoice_key"]);
    strcpy(lnbits_amount, json["lnbits_amount"]);
    strcpy(high_pin, json["high_pin"]);
    strcpy(time_pin, json["time_pin"]);
  }

//ADD PARAMS TO WIFIMANAGER
  wm.setSaveConfigCallback(saveConfigCallback);
  
  WiFiManagerParameter custom_lnbits_server("server", "LNbits server", lnbits_server, 40);
  WiFiManagerParameter custom_lnbits_description("description", "Memo", lnbits_description, 200);
  WiFiManagerParameter custom_invoice_key("invoice", "LNbits invoice key", invoice_key, 500);
  WiFiManagerParameter custom_lnbits_amount("amount", "Amount to charge (sats)", lnbits_amount, 10);
  WiFiManagerParameter custom_high_pin("high", "Pin to turn on", high_pin, 5);
  WiFiManagerParameter custom_time_pin("time", "Time for pin to turn on for (milisecs)", time_pin, 20);
  wm.addParameter(&custom_lnbits_server);
  wm.addParameter(&custom_lnbits_description);
  wm.addParameter(&custom_invoice_key);
  wm.addParameter(&custom_lnbits_amount);
  wm.addParameter(&custom_high_pin);
  wm.addParameter(&custom_time_pin);

//IF RESET WAS TRIGGERED, RUN PORTAL AND WRITE FILES
  if (!wm.autoConnect("⚡lntrigger⚡", "password1")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected :)");
  strcpy(lnbits_server, custom_lnbits_server.getValue());
  strcpy(lnbits_description, custom_lnbits_description.getValue());
  strcpy(invoice_key, custom_invoice_key.getValue());
  strcpy(lnbits_amount, custom_lnbits_amount.getValue());
  strcpy(high_pin, custom_high_pin.getValue());
  strcpy(time_pin, custom_time_pin.getValue());
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["lnbits_server"] = lnbits_server;
    json["lnbits_description"]   = lnbits_description;
    json["invoice_key"]   = invoice_key;
    json["lnbits_amount"] = lnbits_amount;
    json["high_pin"]   = high_pin;
    json["time_pin"]   = time_pin;

    File configFile = SPIFFS.open("/config.txt", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      }
      serializeJsonPretty(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      shouldSaveConfig = false;
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void saveConfigCallback () {
  processing_screen();
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
