#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/// PARAMETRY DO DOSTOSOWANIA ///
const char* ssid     = "SSID";
const char* password = "PASSWORD";
unsigned int server_port = 4202; //nr portu serwera
unsigned int controller_port = 4210;  //nr portu sterownika
const char* controller_ip = "192.168.43.14";
const char* server_ip = "192.168.43.12";
IPAddress ip(192, 168, 43, 12);  //ustalenie adresu IP serwera
IPAddress gateway(192, 168, 43, 1);
IPAddress subnet(255, 255, 255, 0);
/////////////////////////////////////////////////////////////

int i;
int humidity;
int threshold;
int error;
int operatingMode = 0; //0 auto, 1 ręczny
int state = 0; //0 wylaczony, 1 wlaczony
String fanState = "Nieustalony";
String currentMode = "Nieustalony";

WiFiServer server(80);
WiFiUDP Udp;

char incomingPacket[255];  // bufor dla pakietów przychodzących
String command_toSend;
String header;
String output1State = "off";
String output2State = "off";

void Send(){ //wysyłanie komendy w protokole UDP
   char send_command[command_toSend.length()+1];
   command_toSend.toCharArray(send_command, command_toSend.length()+1);
   Udp.beginPacket(controller_ip, controller_port);
   Udp.write(send_command); 
   Udp.endPacket();
}

void setup() {
  Serial.begin(115200);
 
  pinMode(D1, OUTPUT);
  digitalWrite(D1, LOW);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED){
    digitalWrite(D1, HIGH); //dioda sygnalizująca poł. Wi-Fi
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  
    Udp.begin(server_port);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), server_port);
    server.begin();
  }
}
 
void loop() {
  int packetSize = Udp.parsePacket();
  
    if (packetSize)
    {
      int len = Udp.read(incomingPacket, 255);
        if (len > 0)
        {
          incomingPacket[len] = 0;
        }
        
          char buf[4];
          char buf2[2];

          for(i=0; i<4; i++){
            buf[i]=incomingPacket[i];}
          
          if(buf[0]=='y'){ fanState = "W&#322&#261czony"; output1State = "on"; state=1;}
          else if(buf[0]=='n'){ fanState = "Wy&#322&#261czony"; output1State = "off"; state=0;}
          else if(buf[0]=='a'){ currentMode = "Automatyczny"; output2State = "off"; operatingMode=0;}
          else if(buf[0]=='r'){ currentMode = "R&#281czny"; output2State = "on"; operatingMode=1;}
          else if(buf[0]=='p'){ 
            buf2[0]=buf[2]; 
            buf2[1]=buf[3];
            threshold = atoi(buf2);
          }
          else if(buf[0]=='h'){ 
            char* buf3;
            buf3 = &buf[2];
            error = atoi(buf3);
          }
          else{ humidity = atoi(buf); } 
       }
    
  WiFiClient client = server.available();   
 
  if (client) {                             
    Serial.println("New Client.");          
    String currentLine = "";                
    while (client.connected()) {            
      if (client.available()) {             
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                    
          
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println("Refresh: 5"); // odświeżanie strony co 5 sekund
            client.println();

            if (header.indexOf("GET /1/on") >= 0) { //włączanie wentylatora (w trybie ręcznym)
              Serial.println("GPIO 1 on");
              output1State = "on";
              command_toSend = "wl";
              Send();
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
             
            } else if (header.indexOf("GET /1/off") >= 0) { //wyłączanie wentylatora (w trybie recznym)
              Serial.println("GPIO 1 off");
              output1State = "off";
              command_toSend = "wy";
              Send();
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /3/on") >= 0) { //tryb reczny
              Serial.println("GPIO 3 on");
              output2State = "on";
              command_toSend = "r";
              Send();
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /3/off") >= 0) { //tryb automatyczny
              Serial.println("GPIO 3 off");
              output2State = "off";
              output1State = "off";
              command_toSend = "a";
              Send();
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /threshold/40") >= 0) {
              if(threshold != 40){
              command_toSend = "p=40";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /threshold/50") >= 0) {
              if(threshold != 50){
              command_toSend = "p=50";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /threshold/60") >= 0) {
              if(threshold != 60){
              command_toSend = "p=60";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /threshold/70") >= 0) {
              if(threshold != 70){
              command_toSend = "p=70";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /threshold/80") >= 0) {
              if(threshold != 80){
              command_toSend = "p=80";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/1") >= 0) {
              if(error != 1){
              command_toSend = "h=1";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/2") >= 0) {
              if(error != 2){
              command_toSend = "h=2";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/3") >= 0) {
              if(error != 3){
              command_toSend = "h=3";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/4") >= 0) {
              if(error != 4){
              command_toSend = "h=4";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/5") >= 0) {
              if(error != 5){
              command_toSend = "h=5";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/6") >= 0) {
              if(error != 6){
              command_toSend = "h=6";
              Send(); 
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/7") >= 0) {
              if(error != 7){
              command_toSend = "h=7";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/8") >= 0) {
              if(error != 8){
              command_toSend = "h=8";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
              
            } else if (header.indexOf("GET /odch/9") >= 0) {
              if(error != 9){
              command_toSend = "h=9";
              Send();
              }
              client.println("<script type=\"text/javascript\">  history.back(); </script>");
            }   
            
            // Początek HTML
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // Style CSS
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".dropbtn { background-color: gray; color: white; padding: 16px;font-size: 16px; border: none;}");
            client.println(".dropdown { position: relative; display: inline-block;}");
            client.println(".dropdown-content {display: none; position: absolute; background-color: #f1f1f1; min-width: 160px; box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2); z-index: 1;}");
            client.println(".dropdown-content a { color: black; padding: 16px 40px; text-decoration: none; display: block;}");
            client.println(".dropdown-content a:hover {background-color: #ddd;}");
            client.println(".dropdown:hover .dropdown-content {display: block;}");
            client.println(".dropdown:hover .dropbtn {background-color: gray;}");
            client.println(".button { background-color: gray; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("select { width: 20%;padding: 16px 40px; border: none; border-radius: 4px; background-color: gray;");
            client.println(".button2 {background-color: #1ca3df;}</style></head>");

            // Tytuł strony
            client.println("<body><h1>Sterowanie wentylacj&#261</h1>");
            
            
             if (output2State == "off" ) { 
              client.println("<p><a href=\"/3/on\"><button class=\"button\">W&#322&#261cz tryb r&#281czny</button></a></p>");
             } else {
              client.println("<p><a href=\"/3/off\"><button class=\"button button2\">W&#322&#261cz tryb automatyczny</button></a></p>");
             }

            if (output2State == "on" ) { 
            if (output1State == "off" ) { 
              client.println("<p><a href=\"/1/on\"><button class=\"button\">W&#322&#261cz wentylator</button></a></p>");
            } else {
              client.println("<p><a href=\"/1/off\"><button class=\"button button2\">Wy&#322&#261cz wentylator</button></a></p>");
              }
            }
//Rozwijane przyciski do zmiany thresholdu i odchylenia
  client.println("<div class=\"dropdown\">");
  client.println("<button class=\"dropbtn\">Zmie&#324 pr&#243g</button>");
  client.println("<div class=\"dropdown-content\">");
  client.println("<a href=\"/threshold/40\">40</a>");
  client.println("<a href=\"/threshold/50\">50</a>");
  client.println("<a href=\"/threshold/60\">60</a>");
  client.println("<a href=\"/threshold/70\">70</a>");
  client.println("<a href=\"/threshold/80\">80</a>");
  client.println("</div>");
  client.println("</div>");

  client.println("<div class=\"dropdown\">");
  client.println("<button class=\"dropbtn\">Zmie&#324 odchylenie</button>");
  client.println("<div class=\"dropdown-content\">");
  client.println("<a href=\"/odch/1\">1</a>");
  client.println("<a href=\"/odch/2\">2</a>");
  client.println("<a href=\"/odch/3\">3</a>");
  client.println("<a href=\"/odch/4\">4</a>");
  client.println("<a href=\"/odch/5\">5</a>");
  client.println("<a href=\"/odch/6\">6</a>");
  client.println("<a href=\"/odch/7\">7</a>");
  client.println("<a href=\"/odch/8\">8</a>");
  client.println("<a href=\"/odch/9\">9</a>");
  client.println("</div>");
  client.println("</div>");
  
// Lista aktualnych parametrów
  client.print("<p>Tryb pracy:  ");
  client.print(currentMode);
  client.println("</p>");

  client.print("<p>Stan wentylatora:  ");
  client.print(fanState);
  client.println("</p>");

  client.print("<p>Wilgotno&#347&#263:  ");
  client.print(humidity);
  client.println("%</p>");

  client.print("<p>Pr&#243g:  ");
  client.print(threshold);
  client.println("%</p>");

  client.print("<p>Odchylenie:  ");
  client.print(error);
  client.println("%</p>");

  client.print("<p>Uruchomienie wentylatora gdy wilgotno&#347&#263 > ");
  client.print(threshold + error);
  client.print("%, wy&#322&#261czenie gdy wilgotno&#347&#263 < ");
  client.print(threshold - error);
  client.println("%</p>");
  
  client.println("</body>");
  client.println();
            
  break;
  } else { 
    currentLine = "";
    }
  } else if (c != '\r') {  
    currentLine += c;     
    }
  }
 }
    header = "";
    client.stop();
  }
  memset(incomingPacket, 0, 100 * sizeof(char));  //zerujemy bufor
  delay(10);
}
