//#include <ArduinoBLE.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//**********************************************************************************************************
//*    clock_BLE_radio_with_weekly_schedule --  RDA5807 FM Radio which is controlled by weekly schedule using ESP32S3. 
//*                                             You can control ESP32S3 using BLE(Bluetooth LE) GATT. 
//**********************************************************************************************************
//  
//
//  2024/7/1 created by asmaro
//  XIAO ESP32S3 Version
// 

#include "Wire.h"
#include <Adafruit_GFX.h>       // install using lib tool
#include <Adafruit_SSD1306.h>   // install using lib tool
#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time
#include <RDA5807.h>            // install using lib tool
#include <Preferences.h>        //For permanent data

#define LOCAL_NAME "ESP32S3_X"

#define LED_BUILTIN 8           // LED
#define PIN_SDA 5
#define PIN_SCL 6
#define OLED_I2C_ADDRESS 0x3C   // Check the I2C bus of your OLED device
#define SCREEN_WIDTH 128        // OLED display width, in pixels
#define SCREEN_HEIGHT 64        // OLED display height, in pixels
#define OLED_RESET  -1          // Reset pin # (or -1 if sharing Arduino reset pin)
#define MAXSTNIDX    7          // station index 0-6          
#define MAXSCEDIDX   8          // schedule table index 0-8  

#define discriptor_UUID "2901"

//static BLEUUID CTSserviceUUID("00001805");     // Current time service
//static BLEUUID CTSserviceUUID("00002a2b");     // Current time service characteristic
//static BLEUUID CTScharDOWUUID("00002a09");     // Current time service characteristic Day of Week
//static BLEUUID CTScharPPCPUUID("00002a04");    // GAP Peripheral Preferred Connection Parameters
static BLEUUID CTSserviceUUID("00001805-0000-1000-8000-00805f9b34fb");
static BLEUUID CTScharUUID("00002a2b-0000-1000-8000-00805f9b34fb");
static BLEUUID CTScharDOWUUID("00002a09-0000-1000-8000-00805f9b34fb");
static BLEUUID CTScharPPCPUUID("00002a04-0000-1000-8000-00805f9b34fb");
//static BLEUUID descriptorUUID("00002901-0000-1000-8000-00805f9b34fb");
#define descriptorUUID "00002901-0000-1000-8000-00805f9b34fb"
//#define CTSSUUID "00001805-0000-1000-8000-00805f9b34fb"
//#define CTSCUUID "00002a2b-0000-1000-8000-00805f9b34fb"
static bool connected = false;  // connection is active 
static bool advertise = false;  // advertise is active 
static bool wrote = false;      // onwrite() of decriptorcallback
static char strbuff[24];

BLEServer  *pServer = NULL;
BLEClient  *pClient = NULL;
BLEAddress *pBLEAddress = NULL;
BLEDescriptor *pDescriptor = new BLEDescriptor((uint16_t)0x2901);  

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RDA5807 radio;

const char* time_zone  = "JST-9";
struct tm *tm;
int d_mon ;
int d_mday ;
int d_hour ;
int d_min ;
int d_sec ;
int d_wday ;
static const char *weekStr[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; //3文字の英字
const long  gmtOffset_sec = 32400;
int last_d_sec = 99;

int  vol;
int  lastvol;
int  stnIdx;
int  laststnIdx;
int  stnFreq[] = {8040, 8250, 8520, 9040, 9150, 7620, 7810, 7860}; // frequency of radio station
String  stnName[] = {"AirG", "NW", "NHK", "STV", "HBC", "sanka", "karos", "nosut"}; // name of radio station max 5 char
//                      0      1     2      3      4       5        6       7
static const char *stnStr[7] = {"ST0","ST1","ST2","ST3","ST4","ST5","ST6"}; // 3 char

bool bassOnOff = false;
bool vol_ok = true;
bool stn_ok = true;
bool p_onoff_req = false;
bool p_on = false;

uint32_t currentFrequency;
float lastfreq;
struct elm {  // program
   int stime; // strat time(min)
   int fidx;  // frequency table index
   int duration; // length min
   int volstep; // volume
   int poweroff; // if 1, power off after duration
   int scheduled; // if 1, schedule done for logic
};
struct elm entity[7][MAXSCEDIDX + 1] = {
{{390,1,59,2,1,0},{540,6,59,1,0,0},{600,0,59,1,0,0},{660,3,119,1,0,0},{780,1,59,1,0,0},{840,0,59,1,0,0},{900,6,59,1,0,0},{1140,3,119,1,0,0},{1410,0,29,1,1,0}}, // sun
{{390,4,59,2,1,0},{480,3,119,1,0,0},{600,6,59,1,0,0},{720,2,119,1,0,0},{840,1,119,1,0,0},{0,0,0,0,0,0},{1020,1,119,1,0,0},{1200,6,89,1,0,0},{1410,0,29,1,1,0}}, // mon
{{390,4,59,2,1,0},{480,3,119,1,0,0},{720,2,89,1,0,0},{840,1,119,1,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{1020,1,119,1,0,0},{1200,0,89,1,0,0},{1410,0,29,1,1,0}}, // tue
{{390,4,59,2,1,0},{480,3,119,1,0,0},{720,2,89,1,0,0},{840,1,119,1,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{1020,1,119,1,0,0},{1200,0,89,1,0,0},{1410,0,29,1,1,0}}, // wed
{{390,4,59,2,1,0},{480,3,119,1,0,0},{600,6,59,1,0,0},{720,2,119,4,0,0},{840,1,119,1,0,0},{960,1,59,1,0,0},{1080,1,119,1,0,0},{1200,0,89,1,0,0},{1290,3,59,1,1,0}}, // thu
{{390,4,59,2,1,0},{480,3,119,1,0,0},{660,0,59,1,0,0},{720,2,119,1,0,0},{840,6,119,1,0,0},{0,0,0,0,0,0},{1080,1,119,1,0,0},{1200,1,89,1,0,0},{1290,3,59,1,1,1}}, // fri
{{390,0,29,2,0,0},{420,2,119,1,0,0},{540,2,110,1,0,0},{720,2,119,1,0,0},{840,2,119,1,0,0},{960,2,119,1,0,0},{1080,4,59,1,0,0},{1140,3,119,1,0,0},{1260,0,89,1,1,0}}  // sat
};
int last_d_min = 99;
int currIdx = 99;
int pofftm_h = 0;
int pofftm_m = 0;
int bdow = 0; // current browser day of week
int bstn = 0; // current browser radio station

String msg = "";
String wstr = "";

Preferences preferences; // Permanent data
int setWeeksced(String wstr);
int setStation(String wstr);

class MyDescriptorCallbacks: public BLEDescriptorCallbacks {
  void onWrite(BLEDescriptor *pDesciptor) {
    uint8_t *value = pDescriptor->getValue();
    int vl = pDescriptor->getLength();
    int i;
    wrote = true;
    if (vl >= sizeof(strbuff)) vl = 22;
    Serial.print("***** New value: ");
    memcpy(strbuff, value, vl);
    strbuff[vl] = '\n';
    strbuff[vl+1] = 0;
    if (strbuff[0]=='W') { // Day of Week selection
       bdow = strbuff[2] - '1';
       if (bdow < 0 || bdow >= 7) {
         Serial.println("Description err data");
         bdow=0;
        }
    }
    else if (strbuff[0]=='S') { // Radio Staion selection
       bstn = strbuff[2] - '0';
       if (bstn < 0 || bstn >= MAXSTNIDX) {
         Serial.println("Description err data");
         bstn=0;
        }
    }
    Serial.println(strbuff);
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  int typ;
  public:
   MyCallbacks(int t){ typ = t;};
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("MyCallbacks Write");
    wstr = "";
    msg = "";
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("***** New value: ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
        wstr += value[i];
      }
      Serial.println();
      if (value.length() < 20) {
        setStation(wstr);   // register new station
      } else {
        setWeeksced(wstr);  // register new schedule
      }
      Serial.println(msg);
    }
  }
  void onRead(BLECharacteristic *pCharacteristic) {
    std::string wsced ="";
    char htstr[180];
    Serial.println("MyCallbacks Read");
    if (typ == 0)  return; // nop if not necessary
    int i=bdow;
      wsced = weekStr[bdow];
      wsced += ";";
      for(int j = 0; j <= MAXSCEDIDX; j++) {
        sprintf(htstr,"%d:%02d,%d,%d,%d,%d",entity[i][j].stime / 60,entity[i][j].stime % 60,entity[i][j].fidx,entity[i][j].duration,entity[i][j].volstep,entity[i][j].poweroff);
        wsced += htstr;
        if (j != MAXSCEDIDX) wsced += ";";
      }
      wsced += ";";
    wsced += "\n\0";
    pCharacteristic->setValue(wsced);
  }
};
class MyCallbacks2: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("MyCallbacks2 Write");
    wstr = "";
    msg = "";
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("***** New value: ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
        wstr += value[i];
      }
      Serial.println();
      if (value.length() < 20) {
        setStation(wstr);   // registor new station
      } else {
        setWeeksced(wstr);  // register new schedule
      }
      Serial.println(msg);
    }
  }
  void onRead(BLECharacteristic *pCharacteristic) {
    std::string stn ="";
    char htstr[80];
    float tf;
    Serial.println("MyCallbacks2 Read");
    int i=bstn;
      stn = stnStr[bstn];
      stn += ",";
      tf = stnFreq[bstn]/100.0;
      sprintf(htstr,"%3.1f",tf);
      stn += htstr;
      stn += ",";
      stn += stnName[bstn].c_str();
    stn += "\n\0";
    pCharacteristic->setValue(stn);
  }
};

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    pBLEAddress = new BLEAddress(param->connect.remote_bda);
    Serial.printf("MyServerCallbacks Server Connected: %s\n", pBLEAddress->toString().c_str());
    connected = true;
    advertise = false;
    pServer->getAdvertising()->stop();  // stop Advertise
    Serial.println("MyServerCallbacks Stop Advertising");
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("MyServerCallbacks Server Disconnected");
    connected = false;
  }
};

int split(String data, char delimiter, String *dst){
  int index = 0;
  int arraySize = (sizeof(data))/sizeof((data[0]));
  int datalength = data.length();
  
  for(int i = 0; i < datalength; i++){
    char tmp = data.charAt(i);
    if( tmp == delimiter ){
      index++;
      if( index > (arraySize - 1)) return -1;
    }
    else dst[index] += tmp;
  }
  return (index + 1);
}

int dayofWeek(String dow) {
  dow.trim();
  //Serial.println(dow);
  if (dow.equals("Sun") || dow.equals("ST0")) return 0; 
  else if (dow.equals("Mon") || dow.equals("ST1")) return 1;
  else if (dow.equals("Tue") || dow.equals("ST2")) return 2;
  else if (dow.equals("Wed") || dow.equals("ST3")) return 3;
  else if (dow.equals("Thu") || dow.equals("ST4")) return 4;
  else if (dow.equals("Fri") || dow.equals("ST5")) return 5;
  else if (dow.equals("Sat") || dow.equals("ST6")) return 6;
  else return 99;
}

int setWeeksced(String val1){
  String instr[12] = {"\n"};
  String instr2[8] = {"\n"};
  String instr3[4] = {"\n"};
  int ix = split(val1,';',instr);
  if (ix != 11) {
    msg = "different number of arguments.";
    return 4;
  } else {
    //msg = "arguments. ok.";
    int down = dayofWeek(instr[0]);
    if (down > 6) { msg = "invalid day of week."; return 4;}
    else {
      // normal process
      msg = "normal process.";
      instr[0].trim();
      Serial.println(instr[0]);
      for(int j = 0; j <= MAXSCEDIDX; j++) {
        instr[j+1].trim();
        msg = "normal process 2.";
        //Serial.println(instr[j+1]);
        String val2 = instr[j+1];
        ix = split(val2,',',instr2);
        if (ix != 5) { 
            msg = "different number of  2nd level arguments.";
            return 4;
        } else {
          msg = "OK! Processing.";
          //Serial.println(instr2[i]);
          val2 = instr2[0];
          ix = split(val2,':',instr3);
          if (ix != 2) {
            msg = "different number of  3rd level arguments.";
            return 4;
          }
          instr3[0].trim();
          instr3[1].trim();
          entity[down][j].stime = instr3[0].toInt() * 60 + instr3[1].toInt();
          instr3[0] = "";
          instr3[1] = "";
          instr2[0] = "";

          entity[down][j].fidx = instr2[1].toInt();
          instr2[1] = "";
          entity[down][j].duration = instr2[2].toInt();
          instr2[2] = "";
          entity[down][j].volstep = instr2[3].toInt();
          instr2[3] = "";
          entity[down][j].poweroff = instr2[4].toInt();
          instr2[4] = "";
          preferences.putString(weekStr[down], val1);  // save permanently
          
        }
        
      }
      msg = "OK! Done.";
      return 0;
    }
  } 

}
int setStation(String val1){
  String instr[12] = {"\n"};
  int ix = split(val1,',',instr);
  if (ix != 3) {
    msg = "different number of arguments.";
    return 4;
  } else {
    //msg = "arguments. ok.";
    String str = instr[0].substring(2,1);
    int stn = str.toInt();
    if (stn > MAXSTNIDX-1) { msg = "invalid number of stations."; return 4;}
    else {
      // normal process
      msg = "OK! Processing.";
      instr[0].trim();
      Serial.println(instr[0]);
              instr[1].trim();
              float freq = instr[1].toFloat();
              if (freq < 50 || freq > 108) {
                msg = "invalid frequency.";
                return 4;
              }
              stnFreq[stn] = freq * 100;
              instr[2].trim();
              stnName[stn] = instr[2];
             preferences.putString(stnStr[stn], val1);  // save permanently
              Serial.println("Regiter new station: " + val1);
      msg = "OK! Done.";
      return 0;
    }
  } 

}
void setup()
{
  struct tm tm_init;
  struct timeval tv = { 1710000000 + gmtOffset_sec , 0 };  // initial value is 2024/3/9 16:00 + 9:00
  std::string stnList = "";
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); 

  Serial.begin(115200);
  Serial.println("start");
  //Serial.println(entity[0][0].stime);
  pinMode(2, INPUT_PULLUP);  // vol_setting
  pinMode(3, INPUT_PULLUP);  // station_setting
  pinMode(4, INPUT_PULLUP);  // power on_off
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  attachInterrupt(2, vol_setting_2, FALLING); // 
  attachInterrupt(3, station_setting, FALLING); // 
  attachInterrupt(4, power_onoff_setting, FALLING); // 

  Wire.begin(); //
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  // Permanent data check
  preferences.begin("week_sced", false);
  for (int i = 0; i < 7; i++){
      String val1 = preferences.getString(weekStr[i],"");       
      if (val1 != "") {
        //Serial.println(val1);
        int rc = setWeeksced(val1);
        if (rc != 0) Serial.println(msg);
      }
  }
  for (int j = 0; j < MAXSTNIDX ; j++){
      String val1 = preferences.getString(stnStr[j],"");    
      stnList += val1.c_str();
      stnList += ";";   
      if (val1 != "") {
        //Serial.println(val1);
        int rc = setStation(val1);
        if (rc != 0) Serial.println(msg);
      }
  }
  splash();
  radio.setup(); // Start the receiver with default valuses. Normal operation
  delay(500);
  radio.setBand(2); // 
  radio.setSpace(0); //
  delay(300);
  p_on = true;
  radio.setVolume(1);
  vol=1;
  lastvol=1;
  stnIdx = 3;
  laststnIdx = 3;
  radio.setFrequency(9040);  // Tune on 90.4 MHz. Change it to start with your favorite station.
  lastfreq=9040;
  Serial.println("Setup done");
  Serial.println("\nBLEServer");

  settimeofday(&tv, NULL); // Set temp time

  BLEDevice::init(LOCAL_NAME);
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);  //

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Service and Characteristic
  BLEService *pService = pServer->createService(CTSserviceUUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
   CTScharUUID,
   BLECharacteristic::PROPERTY_READ |
   BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCallbacks(0));
  pCharacteristic->setValue(stnList);  // Radio Staions List
  // Descriptor
  pDescriptor->setValue("1710032400");
  pDescriptor->setCallbacks(new MyDescriptorCallbacks());
  pCharacteristic->addDescriptor(pDescriptor);
  // DOW Char
  BLECharacteristic *pCharacteristic2 = pService->createCharacteristic(
   CTScharDOWUUID,
   BLECharacteristic::PROPERTY_READ |
   BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic2->setCallbacks(new MyCallbacks(1));
  // PPCP Char
  BLECharacteristic *pCharacteristic3 = pService->createCharacteristic(
   CTScharPPCPUUID,
   BLECharacteristic::PROPERTY_READ |
   BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic3->setCallbacks(new MyCallbacks2());

  String wsced = "";
  char htstr[180];
  char sstr[480];
  sstr[0] = 'g';
  sstr[1] = 's';
  sstr[2] = 0;
  std::string cstr;
  Serial.println("Set Char for Read");
  int i=bdow;
  wsced = weekStr[bdow];
  wsced += ";";
  for(int j = 0; j <= MAXSCEDIDX; j++) {
    sprintf(htstr,"%d:%02d,%d,%d,%d,%d",entity[i][j].stime / 60,entity[i][j].stime % 60,entity[i][j].fidx,entity[i][j].duration,entity[i][j].volstep,entity[i][j].poweroff);
    wsced += htstr;
    if (j != MAXSCEDIDX) wsced += ";";
  }
  wsced += ";";
  wsced += "\n\0";
  Serial.println(wsced);
  //  pCharacteristic->setValue(wsced);
  cstr = wsced.c_str();
  pCharacteristic2->setValue(cstr);
  pCharacteristic3->setValue("ST1,80.4,AirG"); // Radio Staion Parameter
  // start service 
  pService->start();

}
void splash()
{
  oled.setTextSize(2); // Draw 2X-scale text
  oled.setCursor(0, 0);
  oled.print("Clock");
  oled.setCursor(0, 15);
  oled.print("Radio");
  oled.display();
  delay(500);
  oled.setCursor(0, 30);
  oled.print("BLE"); 
  oled.setCursor(0, 45);
  oled.print("V0.5.2");
  oled.display();
  delay(1000);
}

void dispClock()
{
  //display RTC
  char ts[80];
  float tf;
  time_t t = time(NULL);
  tm = localtime(&t);
  d_mon  = tm->tm_mon+1;
  d_mday = tm->tm_mday;
  d_hour = tm->tm_hour;
  d_min  = tm->tm_min;
  d_sec  = tm->tm_sec;
  d_wday = tm->tm_wday;
  //Serial.print("time ");
  sprintf(ts, "%02d-%02d %s", d_mon, d_mday, weekStr[d_wday]);
  oled.setTextSize(2); // Draw 2X-scale text
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print(ts);
  //Serial.println(ts);
  sprintf(ts,"%02d:%02d:%02d",d_hour,d_min,d_sec);
  oled.setCursor(0, 15);
  oled.print(ts);
  //Serial.println(ts);
  int pi = p_on ? 1 : 0;
  sprintf(ts, "%s%02d %s%01d", "Vol:", vol, "P:", pi);
  oled.setCursor(0, 30);
  oled.print(ts);
  tf = lastfreq/100.0;
  sprintf(ts, "%3.1f %s", tf, "MHz");
  oled.setCursor(0, 45);
  oled.print(ts);
  oled.display();
}
void vol_setting() {
  if (vol_ok) {  // wait last req
    vol++;
    vol_ok = false;
    if (vol > 8) vol = 1; // turn around to support single button
    //radio.setVolume(vol);
  }
}
void vol_setting_2() { 
  if (vol_ok) {  // wait last req
    vol--;
    vol_ok = false;
    if (vol < 0) { vol = 0; lastvol = 0; vol_ok = true;}
  }
}
void station_setting_2() {
  if (stn_ok) {  // wait last req
  if (stnIdx == 0) stnIdx = MAXSTNIDX; // turn around to support single button
    stnIdx--;
    stn_ok = false;
    //if (stnIdx < 0) stnIdx = 0;
  }
}
void station_setting() {
  if (stn_ok) {  // wait last req
    stnIdx++;
    stn_ok = false;
    if (stnIdx > MAXSTNIDX) stnIdx = 0;  // turn around to support single button
  }
}
void power_onoff_setting() {
  if (p_onoff_req==false) {  // wait last req
     p_onoff_req = true;  // req
  }
}

void loop()
{
  char ts[80];
  dispClock();
  if (connected) {
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);   // turn pilot LED on 
    delay(300);                        // wait for a while
    digitalWrite(LED_BUILTIN, HIGH);    // turn pilot LED off 
  }
  if (!connected && !advertise) {  // connection is not active
    advertise = true;
    pServer->getAdvertising()->start();
    Serial.println("Start Advertising");
  }
  if (wrote) { // If there is a signal, run it.
    wrote = false;
    if ((strbuff[0] - '0' >= 0) && (strbuff[0] - '9' <= 9)) { // Unix time(sec) ?
      long t_sec = 0;
      String *tstr = new String(strbuff);
      t_sec = tstr->toInt();
      struct timeval tv = { t_sec + gmtOffset_sec, 0 };  // set current time
      settimeofday(&tv, NULL);
    } else { // command input
      if (strbuff[0]=='v') {  // v+ or v- : volume
        if (strbuff[1]=='+') { // on
          vol_setting(); // v+
        } else { // other off
          vol_setting_2();  // maybe v-
        }  
      } else if (strbuff[0]=='s') { // s+ or s- : station select
        if (strbuff[1]=='+') { // on
          station_setting(); // s+
        } else { // other off
          station_setting_2(); // maybe s-
        }  
      } else if (strbuff[0]=='o') {
          power_onoff_setting(); // on or off req 
      }
    }
  }

  if (p_onoff_req) {
    if (p_on) {
      radio.powerDown();
      Serial.println("pw off");
      p_on = false;
    } else {
      radio.powerUp();
      delay(300);
      radio.setVolume(vol);
      radio.setFrequency(stnFreq[stnIdx]);
      delay(300);
      p_on = true;
    }
    p_onoff_req = false;
  }
  if (lastvol != vol) {
    Serial.println("vol changed");
    radio.setVolume(vol);
    lastvol = vol;
    vol_ok = true;
  }
  if (laststnIdx != stnIdx) {
    Serial.println("stn changed");
    radio.setFrequency(stnFreq[stnIdx]);
    lastfreq = stnFreq[stnIdx];
    laststnIdx = stnIdx;
    stn_ok = true;
  }
 
  if (last_d_min != d_min) {
    last_d_min = d_min;
    if (pofftm_h == d_hour && pofftm_m == d_min && p_on) { // power off time ?
      p_onoff_req = true;
      pofftm_h = 0;
      pofftm_m = 0;
    } else {
      for(int i = 0; i <= MAXSCEDIDX; i++) {
        if (entity[d_wday][i].stime == 0 ) {     
          //nop
          //Serial.println(d_min);
        } else {
          //Serial.println(entity[d_wday][i].stime);
          if ((entity[d_wday][i].stime <= d_hour * 60 + d_min) && 
              ((entity[d_wday][i].stime + entity[d_wday][i].duration) >= (d_hour * 60 + d_min ))
              && (entity[d_wday][i].scheduled != 1)) {
            if (lastfreq == stnFreq[entity[d_wday][i].fidx]) {
              //entity[d_wday][i].scheduled = 1; // mark it scheduled
              
            } else {          
              //radio.setFrequency(stnFreq[entity[d_wday][i].fidx]);
              stnIdx =  entity[d_wday][i].fidx;                     
              //lastfreq = stnFreq[stnIdx];
            }
            //radio.setVolume(entity[d_wday][i].volstep);
            vol = entity[d_wday][i].volstep;
            currIdx = i;
            entity[d_wday][i].scheduled = 1; // mark it scheduled
            Serial.println("scheduled");
            if (entity[d_wday][i].poweroff==1) { // power off ?
              pofftm_h = (entity[d_wday][i].stime + entity[d_wday][i].duration) / 60; // set power off time
              pofftm_m = (entity[d_wday][i].stime + entity[d_wday][i].duration) % 60;
              sprintf(ts,"%02d:%02d %s",pofftm_h,pofftm_m,"poff scheduled");
              Serial.println(ts);
            }
            if (p_on==false) {
              p_onoff_req = true;  //  if power off currently then power on req
              pofftm_h = 0;        // reset
              pofftm_m = 0;
              Serial.println("pw on req");
            }  
          }
        }
      }
    }
  }
  if (connected) delay(400);  else   delay(1000);   // Comment out when you use sleep API below 
  /* esp_sleep_enable_timer_wakeup(1000 * 1000); // wake after 1 sec 
  esp_light_sleep_start();   // sleep and wake up here */
}
