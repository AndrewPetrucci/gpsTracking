#include "Adafruit_FONA.h"
#include <Time.h>
#include <SoftwareSerial.h>

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4


SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// ---------- data for sparkfun database ----------
String sparkfun_url = "data.sparkfun.com/input/";
String sparkfun_publicKey = "********************";
String sparkfun_privateKey = "********************";
String device_name = "kat";


// ---------- data for google maps api ----------
String google_url = "maps.googleapis.com/maps/api/distancematrix/";
String google_privateKey = "***************************************";
String google_responseType = "json";
String transportation_mode ="walking";

// ---------- data for computer science integration ----------
String our_token="****************";
String our_url = "*******************";




String _lat, _long, _time, _latp, _longp;
float latitude, longitude;
int dist;
unsigned long ATtimeOut = 10000;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t type;

void setup() {
  initSerial(115200);
  initFona(4800);
  initGps();
  initSystemTime();
  initStatusLight();
  initCreativePiece();
}

void loop() {
  stall_10();
  Serial.println(F("Checking for Cell network..."));
  if (fona.getNetworkStatus() == 1) {
    locAndSend();
  }
}

// ---------- setup helpers ----------
void initSerial(long br){
  while (! Serial);
  Serial.begin(br);
}

void initFona(long br){
  Serial.println(F("Initializing Fona..."));
  fonaSerial->begin(br);
  while(! fona.begin(*fonaSerial));
}

void initGps(){
  Serial.println(F("Enabling GPS..."));
  fona.enableGPS(true);
}

void initSystemTime(){
  initNetworkTime();
  char tbuffer[23];
  getNetworkTime(tbuffer);
  char *temp = strtok(tbuffer, "\"");
  char *yearp = strtok(temp, "/");
  char *monthp = strtok(NULL, "/");
  char *dayp = strtok(NULL, ",");
  char *hourp = strtok(NULL, ":");
  char *minp = strtok(NULL, ":");
  char *secp = strtok(NULL, "+");
  int year = atoi(yearp);
  int month = atoi(monthp);
  int day = atoi(dayp);
  int hour = atoi(hourp);
  int mins = atoi(minp);
  int sec = atoi(secp);
  setTime(hour, mins, sec, day, month, year);
}

void initNetworkTime(){
  Serial.println(F("Enabling Network Time..."));
  while(!fona.enableNetworkTimeSync(true));
}

void getNetworkTime(char* buff){
  fona.getTime(buff, 23);
  Serial.print("Network Time is ");
  Serial.println(buff);
}

void initStatusLight(){
  pinMode(13, OUTPUT); //init red led
  digitalWrite(13, HIGH); //led onn
}

void initCreativePiece(){
  pinMode(11, OUTPUT); //init red led
  pinMode(12, OUTPUT); //init red led
  pinMode(14, OUTPUT); //init red led
  pinMode(15, OUTPUT); //init red led
  pinMode(16, OUTPUT); //init red led
  pinMode(17, OUTPUT); //init red led
  pinMode(18, OUTPUT); //init red led
  pinMode(19, OUTPUT); //init red led
  disableCreativeDisplay();
  dist=0;
}

// ---------- loop helpers ----------
void stall_10(){
  enableCreativeDisplay();
  delay(2000);
  disableCreativeDisplay();
  Serial.print(".");
  delay(2000);
  Serial.print(".");
  delay(2000);
  Serial.print(".");
  delay(2000);
  Serial.print(".");  
  delay(2000);
  Serial.println(".");
}

void enableCreativeDisplay(){
  if(dist>1500) digitalWrite(11, HIGH);
  if(dist>3000) digitalWrite(12, HIGH);
  if(dist>4500) digitalWrite(14, HIGH);
  if(dist>6000) digitalWrite(15, HIGH);
  if(dist>7500) digitalWrite(16, HIGH);
  if(dist>9000) digitalWrite(17, HIGH);
  if(dist>10500) digitalWrite(18, HIGH);
  if(dist>12000) digitalWrite(19, HIGH);
}

void disableCreativeDisplay(){ 
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(14, LOW);
  digitalWrite(15, LOW);
  digitalWrite(16, LOW);
  digitalWrite(17, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
}

void locAndSend(){
    boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);
    if (gsmloc_success) {
      printGsmLoc();
      gpsLoc();

      updatePrevLoc();
      updateNewLoc();
      updateTimestamp();

      printLoc();

      httpRequests();
      
    } else {
      gsmRetry();
    }
}

void printGsmLoc(){
  Serial.print("GSMLoc lat:");
  Serial.println(latitude, 6);
  Serial.print("GSMLoc long:");
  Serial.println(longitude, 6);
}

void gpsLoc(){
  Serial.print("Getting gps fix");
  boolean gps_success = fona.getGPS(&latitude, &longitude);
  if (gps_success) printGpsLoc();
}

void printGpsLoc(){
  Serial.print("GPS lat:");
  Serial.println(latitude, 6);
  Serial.print("GPS long:");
  Serial.println(longitude, 6);
}

void updatePrevLoc(){
  _latp = _lat;
  _longp = _long;
}

void updateNewLoc(){
  updateLat();
  updateLong();
}

void updateLat(){
  char temp[16];
  dtostrf(latitude, 6, 11, temp);
  _lat = "";
  for(int i=0;i<11;i++) _lat += temp[i];
}

void updateLong(){
  char temp[16];
  dtostrf(longitude, 6, 11, temp);
  _long = "";
  for(int i=0;i<11;i++) _long += temp[i];
}

void updateTimestamp(){ _time = now();
}

void printLoc(){
  Serial.print(F("time = ")); 
  Serial.println(_time);
  Serial.print("_lat:");
  Serial.println(_lat);
  Serial.print("_long:");
  Serial.println(_long);
}

void httpRequests(){
  fona.setHTTPSRedirect(false);
  our_sendData();
  sparkfun_sendData();
  fona.setHTTPSRedirect(true);
  dist+=getDistance();
  Serial.println(dist);
}

void sparkfun_sendData(){
  uint16_t statuscode;
  int16_t length;
  char data[80];
  flushSerial();
  Serial.println(F("****"));
  Serial.println(sparkfun_url+sparkfun_publicKey+"?private_key="+sparkfun_privateKey+"&device_name="+device_name+"&lat="+_lat+"&long="+_long);
  if (!fona.HTTP_POST_start(const_cast<char*>((sparkfun_url+sparkfun_publicKey+"?private_key="+sparkfun_privateKey+"&device_name="+device_name+"&lat="+_lat+"&long="+_long).c_str ()), F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
    return;
  }
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();
      #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
          loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
          UDR0 = c;
      #else
          Serial.write(c);
      #endif
  
      length--;
      if (! length) break;
    }
  }
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
}

void our_sendData(){
  uint16_t statuscode;
  int16_t length;
  char data[80];
  flushSerial();
  Serial.println(F("****"));
  if (!fona.HTTP_POST_start(const_cast<char*>((our_url+"api/runner/ensc/?longitude="+_long+"&latitude="+_lat+"&timestamp="+_time+"&token="+our_token).c_str ()), F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
    return;
  }
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();
      #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
          loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
          UDR0 = c;
      #else
          Serial.write(c);
      #endif
  
      length--;
      if (! length) break;
    }
  }
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
}

int getDistance(){
  if(_latp==NULL) return 0;
  if(_longp==NULL) return 0;
  uint16_t statuscode;
  int16_t length;
  flushSerial();
  Serial.println(F("****"));
  if (!fona.HTTP_GET_start(const_cast<char*>((google_url+google_responseType+"?origins="+_lat+","+_long+"&destinations="+_latp+","+_longp+"&mode="+transportation_mode+"&key="+google_privateKey).c_str ()), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
    return 0;
  }
  char r[10];
  char target[10]="value\" : ";
  int mode = 0;
  int i = 0;
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();
      #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
          loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
          UDR0 = c;
      #else
          Serial.write(c);
      #endif
      if(mode==0){//searching for match
        if(target[i]=='\0'){
          mode = 1;
          i=0;
        }
        else{
          if(c==target[i])i++;
          else i=0;
        }
      }
      if(mode==1){//grab characters
        if(c<='9' && c>='0'){
          r[i++] = c;
        }else{
          mode=2;
        }
      }
      length--;
      if (! length) break;
    }
  }
  Serial.println(F("\n****"));
  fona.HTTP_GET_end();
  r[i]='\0';
  return atoi(r);
}

void gsmRetry(){
  Serial.println("GSM location failed...");
  Serial.println(F("Disabling GPRS"));
  fona.enableGPRS(false);
  Serial.println(F("Enabling GPRS"));
  if (!fona.enableGPRS(true)) Serial.println(F("Failed to turn GPRS on"));  
}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}

void flushFONA() {
  char inChar;
  while (fonaSS.available()){
    inChar = fonaSS.read();
    Serial.write(inChar);
    delay(20);
  }
}
