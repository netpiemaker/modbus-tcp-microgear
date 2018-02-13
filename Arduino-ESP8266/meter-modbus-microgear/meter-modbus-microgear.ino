#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <MicroGear.h>

const char* ssid     = "AP_SSID";
const char* password = "AP_PASSWD";

#define APPID   "APPID"
#define KEY     "KEY"
#define SECRET  "SECRET"
#define ALIAS   "meter"

#define METER_ADDR   0x48     // address 72
#define ADDR_START   0x0064   // ตำแหน่ง address ของ register ที่ต้องการอ่านค่าเริ่มต้นตั้งแต่ 100 ขึ้นไป
                              // http://www.hexadecimaldictionary.com/hexadecimal/0x0064/
#define ADDR_NUMBER  16       // จำนวน register ที่ต้องการทั้งหมด

ModbusMaster nodemeter;

WiFiClient client;

int timer_publish = 0;
int timer_reconnect = 0;
MicroGear microgear(client);

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial1.print("Incoming message --> ");
  msg[msglen] = '\0';
  Serial1.println((char *)msg);
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial1.println("Connected to NETPIE...");
  /* Set the alias of this microgear ALIAS */
  microgear.setAlias(ALIAS);
  timer_reconnect = 0;
  timer_publish = 0;
}

void setup()
{
  Serial.begin(1200, SERIAL_8E1);
  Serial1.begin(9600, SERIAL_8E1);
  
  // mitsubishi meter
  nodemeter.begin(METER_ADDR, Serial);
  
  /* Call onMsghandler() when new message arraives */
  microgear.on(MESSAGE,onMsghandler);
  
  Serial1.println("Starting...");

  /* Initial WIFI, this is just a basic method to configure WIFI on ESP8266.                       */
  /* You may want to use other method that is more complicated, but provide better user experience */
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial1.print(".");
    }
  }

  Serial1.println("WiFi connected");  
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());

  /* Initial with KEY, SECRET and also set the ALIAS here */
  microgear.init(KEY,SECRET,ALIAS);
  
  /* connect to NETPIE to a specific APPID */
  microgear.connect(APPID);
}

void loop()
{
  /* To check if the microgear is still connected */
  if (microgear.connected()) {

    /* Call this method regularly otherwise the connection may be lost */
    microgear.loop();
    
    if (timer_publish >= 5000){
      Serial1.println(".........................");
    
      uint8_t result = nodemeter.readHoldingRegisters(ADDR_START, ADDR_NUMBER);
      
      if (result == nodemeter.ku8MBSuccess) {
        
          Serial1.print("Line Voltage (RMS) (V) >>>>> ");
          String line_voltage = String(nodemeter.getResponseBuffer(0x02)/100.0f);
          Serial1.println(line_voltage); // Line Voltage (RMS) (V)
          
          Serial1.print("Frequency (Hz) >>>>> ");
          String frequency = String(nodemeter.getResponseBuffer(0x05)/10.0f);
          Serial1.println(frequency); // Frequency (Hz)
          
          Serial1.print("Active Energy (Wh) imp+exp >>>>> ");
          String active_energy = String(nodemeter.getResponseBuffer(0x0A));
          Serial1.println(active_energy); // Active Energy (Wh) imp+exp
          
          Serial1.print("Line Current (RMS) (A) >>>>> ");
          String line_current = String(nodemeter.getResponseBuffer(0x0C)/100.0f);
          Serial1.println(line_current); // Line Current (RMS) (A)
    
          Serial1.print("Active Power (W) >>>>> ");
          String active_power = String(nodemeter.getResponseBuffer(0x0F)/1000.0f);
          Serial1.println(active_power); // Active Power (W)

          // รวมค่าทั้งหมดเป็นสตริงไว้ที่ตัวแปร msg สำหรับส่งค่าผ่าน netpie
          String msg = active_energy+","+active_power+","+line_current+","+line_voltage+","+frequency;
          
          // ส่งค่า Active Energy, Active Power, Line Current, Line Voltage และ Frequency ไปที่ topic : /meter/modbus
          microgear.publish("/meter/modbus",msg);
      }
      timer_publish = 0;
    }else timer_publish += 100;
  }else{
    Serial1.println("connection lost, reconnect...");
    if (timer_reconnect >= 5000) {
        microgear.connect(APPID);
        timer_reconnect = 0;
        timer_publish = 0;
    }
    else timer_reconnect += 100;
  }
  delay(100);
}


