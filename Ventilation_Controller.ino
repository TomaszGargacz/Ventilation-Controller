#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DHT.h" 

#define DHT11_PIN D1 
#define RELAY D2
#define BUTTON1 D3 
#define BUTTON2 D4 
#define MODE D6
#define STATE D7 

/// PARAMETERS TO MODIFY ///
const char* ssid     = "SSID";
const char* password = "PASSWORD";
unsigned int server_port = 4202; //nr portu serwera
const char* server_ip = "192.168.43.12"; //adres IP serwera
unsigned int controller_port = 4210;  // port sterownika
IPAddress ip(192,168,43,14);  //ustalenie adresu IP sterownika
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
/////////////////////////////////////////////////////////////

DHT dht;
WiFiUDP Udp;
char incomingPacket[255];  // buf dla pakietów przychodzących
boolean operatingMode = 0; //0 - automatyczny, 1 - reczny  
boolean state = 0;  // stan przycisku włącz-wyłącz wentylator (tryb ręczny)
int p = 70; //p - próg, załączanie dla wilg. > p+h, wyłączanie dla wilg. < p-h
int h = 5; //odchylenie +- 5%
int connectingTime = 0; //odliczanie czasu próby łączenia z WiFi
unsigned int currentTime = 0;    //   __\ do obsługi 
unsigned int rememberedTime = 0; //     / wielowątkowości
String command_toSend;

void Wyslij(){ //wysyłanie komendy w protokole UDP
   char wyslij_komenda[command_toSend.length()+1];
   command_toSend.toCharArray(wyslij_komenda, command_toSend.length()+1);
   Udp.beginPacket(server_ip, server_port);
   Udp.write(wyslij_komenda); 
   Udp.endPacket();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

WiFi.config(ip,gateway,subnet);

dht.setup(DHT11_PIN); 
pinMode(D5, OUTPUT); 
pinMode(MODE, OUTPUT); 
pinMode(STATE, OUTPUT); 
pinMode(RELAY, OUTPUT); 
pinMode(BUTTON1, INPUT_PULLUP); 
pinMode(BUTTON2, INPUT_PULLUP); 

digitalWrite(RELAY, HIGH); //HIGH - wył., LOW - wł.
digitalWrite(D5, LOW); //sygn. poł. wifi
digitalWrite(MODE, LOW); //MODE: LOW - auto., HIGH - ręczny
digitalWrite(STATE, LOW); //STATE: LOW - wył., HIGH - wł.

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    connectingTime++;
    if(connectingTime>60){break;} //warunek przerwania próby łączenia z WiFi (30s)
  }
  if(connectingTime<60){ //jeśli się połączyło
  digitalWrite(D5, HIGH); //dioda sygnalizująca poł. Wi-Fi
  Serial.println(" connected");

  Udp.begin(controller_port);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), controller_port);
  }
  if(connectingTime>60){Serial.printf("No connection with router");} 
}

void loop()
{
int packetSize = Udp.parsePacket();

  if (packetSize) //odbieranie przychodzących pakietów 
  {
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());

    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
       Serial.printf("UDP packet contents: %s\n", incomingPacket);

       if(incomingPacket[0]== 'p' && incomingPacket[1]== '='){
        char buf[2];
        buf[0]=incomingPacket[2];
        buf[1]=incomingPacket[3];
        
        p = atoi(buf);
       }
       
       if(incomingPacket[0]== 'h' && incomingPacket[1]== '='){
        char buf2[1];
        buf2[0]=incomingPacket[2];
        h = atoi(buf2);       
       }
       
       if(incomingPacket[0]== 'a'){ //tryb automatyczny
        operatingMode=0;}
       if(incomingPacket[0]== 'r'){ //tryb ręczny
        operatingMode=1;}
  }
  
  if(digitalRead(BUTTON2) == LOW){ //zmiana trybu pracy: automatyczny i ręczny
    delay(30);
    operatingMode=!operatingMode;
    while(digitalRead(BUTTON2) == LOW);
  }
    
currentTime = millis();
  if(currentTime - rememberedTime >= 1000UL){ //wykonanie kodu co 1 sekundę
    rememberedTime = currentTime;
  
    int humidity_read = dht.getHumidity(); 

    String humidity_toSend = String(humidity_read, DEC);
    char send_humidity[humidity_toSend.length()+1];
    humidity_toSend.toCharArray(send_humidity, humidity_toSend.length()+1);

    if(dht.getStatusString()=="OK"){
      Udp.beginPacket(server_ip, server_port);
      Udp.write(send_humidity); 
      Udp.endPacket();   
    }

    if(operatingMode==1){
      command_toSend = "r";
      Wyslij(); 
    }
  
    if(operatingMode==0){
      command_toSend = "a";
      Wyslij();  
    }

    if(digitalRead(RELAY)==HIGH){
      command_toSend = "n";
      Wyslij(); 
    }

    if(digitalRead(RELAY)==LOW){
      command_toSend = "y";
      Wyslij(); 
    }

      if(p==40){command_toSend = "p=40";}
      if(p==50){command_toSend = "p=50";}
      if(p==60){command_toSend = "p=60";}
      if(p==70){command_toSend = "p=70";}
      if(p==80){command_toSend = "p=80";}
      Wyslij();

      if(h==1){command_toSend = "h=1";}
      if(h==2){command_toSend = "h=2";}
      if(h==3){command_toSend = "h=3";}
      if(h==4){command_toSend = "h=4";}
      if(h==5){command_toSend = "h=5";}
      if(h==6){command_toSend = "h=6";}
      if(h==7){command_toSend = "h=7";}
      if(h==8){command_toSend = "h=8";}
      if(h==9){command_toSend = "h=9";}
      Wyslij();
  }

  if(operatingMode==1){ //tryb ręczny
    digitalWrite(MODE, HIGH); //sygnalizacja trybu ręcznego
    
    if(digitalRead(BUTTON1) == LOW){
         delay(40);
         if(digitalRead(RELAY)==HIGH){state=true;}
         state=!state;
         digitalWrite(RELAY, state);
         digitalWrite(STATE, !state);
         while(digitalRead(BUTTON1) == LOW);
       }
    if(incomingPacket[0]== 'w' && incomingPacket[1]== 'l'){
         digitalWrite(RELAY, LOW);
         digitalWrite(STATE, HIGH);
      }
    if(incomingPacket[0]== 'w' && incomingPacket[1]== 'y'){
         digitalWrite(RELAY, HIGH);
         digitalWrite(STATE, LOW);
      }
    }

  else{ //tryb automatyczny
    digitalWrite(MODE, LOW);
    if(dht.getHumidity()>(p+h)){ //jeśli wilgotność > próg + odchylenie 
      digitalWrite(STATE, HIGH); //to zapal diode 
      digitalWrite(RELAY, LOW); //włącz wentylator
    }
  
    if(dht.getHumidity()<(p-h)){ //jeśli wilgotność < próg - odchylenie 
      digitalWrite(STATE, LOW); //zgaś diodę
      digitalWrite(RELAY, HIGH); // wyłącz wentylator
    }
  }
  memset(incomingPacket, 0, 100 * sizeof(char));  //zerujemy bufor
} //koniec void loop
