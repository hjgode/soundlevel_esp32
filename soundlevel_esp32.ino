#include <FS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "sl814.h"
#define RXD2 16
#define TXD2 17

SL814class* sl814;
splDaten sDaten={ false, 0.0, true, true };

HardwareSerial mySerial(2);

const char* ssid = "Horst1";
const char* password = "1234567890123";

WiFiServer telnetserver(23); 
#define MAX_SRV_CLIENTS 4
WiFiClient serverClients[MAX_SRV_CLIENTS];

AsyncWebServer server(80);
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 1.5rem; }
    p { font-size: 1.5rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <p>ESP32 SoundLevel Server</p>
  <p>    
    <span id="soundlevel">%SOUNDLEVEL%</span>
    <class="units">dbA</>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("soundlevel").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/soundlevel", true);
  xhttp.send();
}, 1000 ) ;

</script>
</html>)rawliteral";

// Replace placeholder with soundlevel value
String processor(const String& var){
  //Serial.println(var);
  if(var == "SOUNDLEVEL"){
      String s=String(sl814->getDaten().level,1);
    return s;
  }
  return String();
}

const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
int second;
int minute;
int hour;
int day;
int month;
int year;
int weekday;
long current;
struct tm timeinfo;
  /*
struct tm
{
int    tm_sec;   //   Seconds [0,60]. 
int    tm_min;   //   Minutes [0,59]. 
int    tm_hour;  //   Hour [0,23]. 
int    tm_mday;  //   Day of month [1,31]. 
int    tm_mon;   //   Month of year [0,11]. 
int    tm_year;  //   Years since 1900. 
int    tm_wday;  //   Day of week [0,6] (Sunday =0). 
int    tm_yday;  //   Day of year [0,365]. 
int    tm_isdst; //   Daylight Savings flag. 
}
 */  

void printLocalTime()
{

  if(!getLocalTime(&timeinfo, 500)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
}

void doTelnet(){
    uint8_t i;
    if (telnetserver.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = telnetserver.available();
        if (!serverClients[i]) Serial.println("available broken");
        Serial.print("New client: ");
        Serial.print(i); Serial.print(' ');
        Serial.println(serverClients[i].remoteIP());
        break;
      }
    }
    if (i >= MAX_SRV_CLIENTS) {
      //no free/disconnected spot so reject
      telnetserver.available().stop();
    }
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
        while(serverClients[i].available()) Serial2.write(serverClients[i].read());
      }
    }
    else {
      if (serverClients[i]) {
        serverClients[i].stop();
      }
    }
  }
  //send data
  char msg[40];
  sprintf(msg, "%.1f\r\n", sl814->getDaten().level);
  size_t len = strlen(msg);
  //push data to all connected telnet clients
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      serverClients[i].write(msg, len);
      delay(1);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  mySerial.begin(9600, SERIAL_8E1);//, RXD2, TXD2);
  sl814=new SL814class(&mySerial);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
  }
  
  Serial.println(WiFi.localIP());

    //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

/*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      char response[80];
      sprintf(response, "%.1f", sl814->getDaten().level);
    request->send(200, "text/html", response);// "<p>This is HTML!</p>");
  });
*/
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/soundlevel", HTTP_GET, [](AsyncWebServerRequest *request){
      char response[80];
      sprintf(response, "%.1f", sl814->getDaten().level);
    request->send_P(200, "text/plain", response);
  });
 
  server.begin();
  telnetserver.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
/*
  float f = sl814->getMeasure();
  if(f != 0.0) {
    Serial.print("#################### ");
    Serial.println(f);
  }
  else{
    Serial.print(".");
  }
*/
//  splDaten sDaten;
  char msg[64];
  sDaten=sl814->getDaten();
  if(sDaten.bValid){
//    Serial.print(sDaten.level, DEC);
    //OK: sprintf(msg, "%.1f\n", sDaten.level);
    char sFast, sA;
    if(sDaten.isFast)
      sFast='F';
    else
      sFast='s';
    if(sDaten.isA)
      sA='A';
    else
      sA='c';
    sprintf(msg, "%.1f", sDaten.level);
    Serial.print(msg);
    Serial.print(" ");Serial.print(sA);Serial.print(" ");Serial.println(sFast);
  }
  else{
    Serial.print(".");
  }
  delay(1000);
  doTelnet();
//  printLocalTime();
}
