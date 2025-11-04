/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-web-server-sent-events-sse/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
// SD CARD
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// WIFI
#include <WiFi.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
HTTPClient http;
#include "HX711.h"
#include "time.h"
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; // time
float sensorWeight; // scale weight
String selectedFood;
int currentRow;

// GOOGLE SHEETS
#include <ESP_Google_Sheet_Client.h>
String spreadsheetId; //DELETE = "16yuLGQIBaB1sbhxDKLsr7hAfya5Y9Ieffzd01VcxAFs";
// Token Callback function
void tokenStatusCallback(TokenInfo info);

//  adjust pins if needed
uint8_t dataPin = 33;
uint8_t clockPin = 25;

const char* FOOD_PARAM = "food";

float f;
HX711 scale;
// Replace with your network credentials
String _GAS_ID; //DELETE = "AKfycbwVclbVWXG9UjzA18AQMx7PE8bbE8qXNjgkVfBHR4_MsWpO3K3kV4m9dCL0aXMmNYwJ";

String ssid_pwd;
String ssid_un;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 300;


float weight;

float getSensorReadings(){
  weight = scale.get_units(5);
  return weight;
}

// Initialize WiFi
void initWiFi() {
    WiFi.mode(WIFI_STA);
    Serial.print(ssid_un);
    Serial.print(ssid_pwd);
    WiFi.begin(ssid_un.c_str(), ssid_pwd.c_str());
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void initScale(){
    ////////init scale////////
  scale.begin(dataPin, clockPin);
  scale.set_scale(410.0983);       // TODO you need to calibrate this yourself.
  scale.tare();
}

void initGS(){
      // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);
    
    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);
    
    // Begin the access token generation for Google API authentication    
    GSheet.begin("/iot-scale-472200-bcfc77255d1e.json", esp_google_sheet_file_storage_type_sd);
}
std::vector<String> plantList;
void initVeggieSheet(){
  }

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}



void sendSheetData(String item, String weight){
    String url = "https://script.google.com/macros/s/" + _GAS_ID;
    String queryString = "/exec?colName="+item+"&value="+weight;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(queryString);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
      } else {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    Serial.println(http.getString());
    http.end();


  }

// Read Files
void initSDReader(){
    Serial.begin(115200);
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
}

String readFile(fs::FS &fs, const char * path){
  // open file for reading
  File file = SD.open(path, FILE_READ);
  if (file) {
    if ( file.available()) {
      String data = file.readString();
      file.close();
      return data;
    }

    file.close();
  } else {
    Serial.print(F("SD Card: error on opening file"));
  }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
%HTML%
</html>)rawliteral";


String processor(const String& var){
  // get weight from sensor
  getSensorReadings();
  if(var == "WEIGHT"){
    return String(weight);
    // load plant list
  } else if (var == "HTML"){
    return readFile(SD, "/home.html");
    }
  return String();
}

void setup() {
  Serial.begin(115200);
  // read data from files
  initSDReader();
  ssid_pwd = readFile(SD, "/ssid_password.txt");
  ssid_pwd.trim();
  
  ssid_un = readFile(SD, "/ssid.txt");
  ssid_un.trim();
  
  _GAS_ID = readFile(SD, "/gas_id.txt");
  _GAS_ID.trim();

  spreadsheetId = readFile(SD, "/spreadsheet_id.txt");
  spreadsheetId.trim();
  
  initScale();  
  initWiFi();
  initGS();

  //Configure time
  configTime(0, 0, ntpServer);

  // Get food list 
  String str(readFile(SD, "/plants_list.txt"));
  char str_array[str.length()];
  str.toCharArray(str_array, str.length());
  char* token = strtok(str_array, ",");
  char *plant_array[100];
  int i = 0;
  while (token != NULL)
    {
        plant_array[i++] = token;
        token = strtok (NULL, ",");
    }

  // Handle Web Server
  //// home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html, processor);
  });

  //// tare 
  server.on("/tare", HTTP_GET, [](AsyncWebServerRequest *request){
    scale.tare();
    request->send(200, "text/html", index_html, processor);
  });

  //// save weight
  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
    String inputFood;
    inputFood = request->getParam(FOOD_PARAM)->value();

    // TODO: redirect to home page again 
    // TODO: add weight to form?
   
    FirebaseJson response;
    Serial.println("\nAppend spreadsheet values...");
    Serial.println("----------------------------");
    FirebaseJson valueRange;

    valueRange.add("majorDimension", "COLUMNS");
    epochTime = getTime();
    sensorWeight = getSensorReadings();

    valueRange.set("values/[0]/[0]", epochTime);
    valueRange.set("values/[2]/[0]", "=EPOCHTODATE(INDIRECT(ADDRESS(ROW(), COLUMN()-1, 4)))");
    valueRange.set("values/[3]/[0]", sensorWeight);
    valueRange.set("values/[4]/[0]", inputFood);

    String bckup;
    bckup += String(epochTime);
    bckup += ",=EPOCHTODATE(INDIRECT(ADDRESS(ROW(), COLUMN()-1, 4))),";
    bckup += String(sensorWeight);
    bckup += ",";
    bckup += String(inputFood);
    bckup += "\n";
    //back up on local SD
    appendFile(SD, "/data.csv", bckup.c_str());
    
    // Append values to the spreadsheet
    bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
    if (success){
        response.toString(Serial, true);
        valueRange.clear();
    }
    else{
        Serial.println(GSheet.errorReason());
    }
    initVeggieSheet();
    request->send(200, "text/html", index_html, processor);
  });



  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  // start server
  server.begin();
  Serial.println(WiFi.localIP());

}


void loop() {
  //check google sheet ready
  bool ready = GSheet.ready();

  //get sensor readings
  if ((millis() - lastTime) > timerDelay) {
    getSensorReadings();
    // Send Events to the Web Client with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(weight).c_str(),"weight",millis());
    
    lastTime = millis();
  }
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
