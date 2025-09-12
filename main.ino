/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-web-server-sent-events-sse/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "HX711.h"
//  adjust pins if needed
uint8_t dataPin = 33;
uint8_t clockPin = 25;

float f;
HX711 scale;
// Replace with your network credentials
const char* ssid = "...";
const char* password = "...";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 300;


float weight;

void getSensorReadings(){
  weight = scale.get_units(5);
}

// Initialize WiFi
void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
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

String processor(const String& var){
  getSensorReadings();
  //Serial.println(var);
  if(var == "WEIGHT"){
    return String(weight);
  }
  return String();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>IoT Scale</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- jQuery -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js" integrity="sha512-v2CJ7UaYy4JwqLDIrZUI/4hqeoQieOmAZNXBeQyjo21dadnwR+8ZaIJVT8EE2iyI61OV8e6M8PP2/4hpQINQ/g==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/typeahead.js/0.11.1/typeahead.jquery.min.js" integrity="sha512-AnBkpfpJIa1dhcAiiNTK3JzC3yrbox4pRdrpw+HAI3+rIcfNGFbVXWNJI0Oo7kGPb8/FG+CMSG8oADnfIbYLHw==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
  <!-- Fontawesome -->
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/7.0.1/css/all.min.css" integrity="sha512-2SwdPD6INVrV/lHTZbO2nodKhrnDdJK9/kg2XD1r9uGqPo1cUbujc+IYdlYdEErWNu69gVcYgdxlmVmzTWnetw==" crossorigin="anonymous" referrerpolicy="no-referrer" />  
  <!-- BS5 -->
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC" crossorigin="anonymous">
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/js/bootstrap.bundle.min.js" integrity="sha384-MrcW6ZMFYlzcLA8Nl+NtUVF0sA7MsXsP1UyJoMp4YLEuNSfAP+JcXn/tWtIaxVXM" crossorigin="anonymous"></script>
  <!-- Typeahead -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/typeahead.js/0.11.1/typeahead.bundle.min.js" integrity="sha512-qOBWNAMfkz+vXXgbh0Wz7qYSLZp6c14R0bZeVX2TdQxWpuKr6yHjBIM69fcF8Ve4GUX6B6AKRQJqiiAmwvmUmQ==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/typeahead.js/0.11.1/bloodhound.min.js" integrity="sha512-kC/4GX7MxhslxDVyJOuyMVjr0uc3c/qp9S/E2ORxkttE07pdeImi5LhdRc5aX6sxnhFuRW/tQrRMjTlxZYC8SQ==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
  
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #43882A; color: white; font-size: 1rem; display: flex; justify-content: center; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
    .reading { font-size: 1.4rem; }
    /* Typeahead styling */
    .bs-example {
        font-family: sans-serif;
        position: relative;
    }
    .typeahead, .tt-query, .tt-hint {
        border: 2px solid #CCCCCC;
        border-radius: 8px;
        font-size: 22px; /* Set input font size */
        height: 30px;
        line-height: 30px;
        outline: medium none;
        padding: 8px 12px;
        width: 100%;
    }
    .typeahead {
        background-color: #FFFFFF;
    }
    .typeahead:focus {
        border: 2px solid #0097CF;
    }
    .tt-query {
        box-shadow: 0 1px 1px rgba(0, 0, 0, 0.075) inset;
    }
    .tt-hint {
        color: #999999;
    }
    .tt-menu {
        background-color: #FFFFFF;
        border: 1px solid rgba(0, 0, 0, 0.2);
        border-radius: 8px;
        box-shadow: 0 5px 10px rgba(0, 0, 0, 0.2);
        margin-top: 12px;
        padding: 8px 0;
        width: 100%;
    }
    .tt-suggestion {
        font-size: 22px;  /* Set suggestion dropdown font size */
        padding: 3px 20px;
    }
    .tt-suggestion:hover {
        cursor: pointer;
        background-color: #0097CF;
        color: #FFFFFF;
    }
    .tt-suggestion p {
        margin: 0;
    }
  </style>
</head>
<body>
  <div class="topnav">
    <i class="fa-solid fa-plate-wheat" style="margin:10px"></i>
    <h1>Vegetable Scale</h1>
    <i class="fa-solid fa-plate-wheat" style="margin:10px"></i>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card">
        <p>
          <i class="fas fa-weight-scale" style="color:#059e8a;"></i> WEIGHT
          <span class="reading">
         </p>
         <p>
          <span id="temp">%WEIGHT%</span> grams</span>
        </p>
      </div>
    </div>
    <br>
    <button type="button" class="btn btn-primary" onclick="tareScale(this)">Tare</button>
    <hr>
    <div class="bs-example">
        <input type="text" class="typeahead tt-query" autocomplete="off" spellcheck="false">
    </div>
    <!-- TODO ass a save form button -->
    <!-- save values to google sheets -->
  </div>
  <script>
    function tareScale(element) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/tare", true);
      xhr.send();
    }
    
    if (!!window.EventSource) {
     var source = new EventSource('/events');
     
     source.addEventListener('open', function(e) {
      console.log("Events Connected");
     }, false);
     source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
     }, false);
     
     source.addEventListener('message', function(e) {
      console.log("message", e.data);
     }, false);
     
     source.addEventListener('weight', function(e) {
      console.log("weight", e.data);
      document.getElementById("temp").innerHTML = e.data;
     }, false);
    }

    // typeahead
    $(document).ready(function(){
        // Defining the local dataset
        var plants = ['Pepper', 'Tomato', 'Basil', 'Thyme', 'Carrots'];
        
        // Constructing the suggestion engine
        var plants = new Bloodhound({
            datumTokenizer: Bloodhound.tokenizers.whitespace,
            queryTokenizer: Bloodhound.tokenizers.whitespace,
            local: plants
        });
        
        // Initializing the typeahead
        $('.typeahead').typeahead({
            hint: true,
            highlight: true, /* Enable substring highlighting */
            minLength: 1 /* Specify minimum characters required for showing result */
        },
        {
            name: 'plants',
            source: plants
        });
    });  
  </script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);


  initScale();  
  initWiFi();


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
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    getSensorReadings();
    Serial.printf("Weight = %.2f ºC \n", weight);
    Serial.println();

    // Send Events to the Web Client with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(weight).c_str(),"weight",millis());
    
    lastTime = millis();
  }
}
