#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SdaPin 21
#define SclPin 22
#define ScreenWidth 128 
#define ScreenHeight 32
#define OledReset -1 
#define ScreenAddress 0x3C 

const String WifiNetwork    = "XXXXX";
const String WifiPassword   = "XXXXX";
const String ApiUrl_Weather = "https://api.openweathermap.org/data/2.5/weather?q=Toronto&appid=XXXXXXXXXXX";
const String ApiUrl_Time    = "https://www.timeapi.io/api/Time/current/zone?timeZone=America/Toronto";  

HTTPClient HttpClient;
int HttpCode;

String JsonFile, ApiTemperature, ApiHumidity, ApiDescription, ApiYear, ApiMonth, ApiDay, ApiHour, ApiMinute, ApiSeconds;

RTC_DS1307 RealTimeClock;
Adafruit_SSD1306 LedDisplay(ScreenWidth, ScreenHeight, &Wire, OledReset);

String  CurrentTime, CurrentDate;
String SetAlarm = "";
String CurrentAlarm = "No Alarm";

const int Knob1 = 34;
const int Knob2 = 35;
const int Buzzer = 19;
const int ResetButton = 18;
const int SetButton = 5;
 
void setup() {
  Serial.begin(9600);

  WiFi.begin(WifiNetwork, WifiPassword);
  Wire.begin(SdaPin, SclPin);

  if(!LedDisplay.begin(SSD1306_SWITCHCAPVCC, ScreenAddress)) {
  Serial.println(F("SSD1306 allocation failed")); for(;;); 
  } 
  while(WiFi.status() != WL_CONNECTED){
  Serial.println("Connecting to Wifi..."); delay(500);
  }
  pinMode(Knob1, INPUT);
  pinMode(Knob2, INPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(ResetButton, INPUT_PULLUP);
  pinMode(SetButton, INPUT_PULLUP);
  if(WiFi.status() == WL_CONNECTED){ 
    if(!RealTimeClock.begin()){
    Serial.flush(); while(1){ delay(10); }
    }
    if(!RealTimeClock.isrunning()){
      FetchTimeData();
      RealTimeClock.adjust(DateTime(ApiYear.toInt(), ApiMonth.toInt(), ApiDay.toInt(), ApiHour.toInt(), ApiMinute.toInt(), ApiSeconds.toInt()));
    }
    UpdateRealTimeClock();
  }
  //RealTimeClock.adjust(DateTime(2024, 4, 24, 12, 16, 8));
}

void loop() {
  BeepAlarm();

  if(analogRead(Knob1) < 585*1){
    if(digitalRead(ResetButton) == LOW){
      UpdateWeatherInfo();
    }
    WriteText("Toronto\nTemperature:\n" + ApiTemperature + " C");
  }
  else if(analogRead(Knob1) < 585*2){
    if(digitalRead(ResetButton) == LOW){
      UpdateWeatherInfo();
    }
    WriteText("Toronto\nHumidity:\n" + ApiHumidity + " %");
  }
  else if(analogRead(Knob1) < 585*3){
    if(digitalRead(ResetButton) == LOW){
      UpdateWeatherInfo();
    }
    WriteText("Toronto\nDescription:\n" + ApiDescription);
  }
  else if(analogRead(Knob1) < 585*4){
    if(digitalRead(ResetButton) == LOW){
      UpdateRealTimeClock();
    }
    GetDate();
    WriteText("Toronto\nDate:\n" + CurrentDate);
  }
  else if(analogRead(Knob1) < 585*5){
    if(digitalRead(ResetButton) == LOW){
      UpdateRealTimeClock();
    }
    GetTime();
    WriteText("Toronto\nTime:\n" + CurrentTime);
  }
  else if(analogRead(Knob1) < 585*6){
    AlarmSystem();
    if(CurrentAlarm == "No Alarm"){
      WriteText("Set Alarm\n" + SetAlarm + "\nCurrentAlarm\nNo Alarm");
    }
    else{ 
      WriteText("Set Alarm\n" + SetAlarm + "\nCurrentAlarm\n" + CurrentAlarm);
    }
  }
  else{
    if(WiFi.status() == WL_CONNECTED){
    WriteText("Wifi is connected\nNetwork: " + WifiNetwork);
    }
  }
}

void WriteText(String Message) {
  LedDisplay.clearDisplay();

  LedDisplay.setTextSize(1);             // Normal 1:1 pixel scale
  LedDisplay.setTextColor(SSD1306_WHITE);        // Draw white text
  LedDisplay.setCursor(0,0);             // Start at top-left corner
  LedDisplay.println(Message);

  LedDisplay.display();
}

void AlarmSystem(void){
  int SetHour = ceil(analogRead(Knob2)/(170.625));
  if(SetHour == 0){
    SetAlarm = "1:00:00AM";
  }
  else if(SetHour < 12){
    SetAlarm = String(SetHour) + ":00:00AM";
  }
  else if(SetHour == 12){
    SetAlarm = String(SetHour) + ":00:00PM";
  }
  else if(SetHour == 24){
    SetAlarm = String(SetHour-12) + ":00:00AM"; 
  }
  else{
    SetHour = SetHour - 12;
    SetAlarm = String(SetHour) + ":00:00PM";
  }

  if(digitalRead(SetButton) == LOW){
    CurrentAlarm = SetAlarm;
  }

  if(digitalRead(ResetButton) == LOW){
    CurrentAlarm = "No Alarm";
  }
}

void BeepAlarm(void){
  if(CurrentTime.equals(CurrentAlarm)){
    for(int i=0; i<7; i++){
    digitalWrite(Buzzer, HIGH);
    delay(1000);
    digitalWrite(Buzzer, LOW);
    delay(1000);
    }
  }
}

void GetTime(void){
  char ClockTimePM[] = ":mm:ssPM";
  char ClockTimeAM[] = ":mm:ssAM";
  char Hour[] = "hh";
  DateTime ClockNow = RealTimeClock.now();
  String HourRTC = ClockNow.toString(Hour);
  if(HourRTC.toInt() < 12){
    CurrentTime = HourRTC;
    CurrentTime.concat(ClockNow.toString(ClockTimeAM));
  }
  else if(HourRTC.toInt() == 12){
    CurrentTime = HourRTC;
    CurrentTime.concat(ClockNow.toString(ClockTimePM));
  }
  else if(HourRTC.toInt() > 12){
    HourRTC = (HourRTC.toInt()-12);
    CurrentTime = HourRTC;
    CurrentTime.concat(ClockNow.toString(ClockTimePM));
  }
}

void GetDate(void){
  char ClockDate[] = "YYYY:MM:DD";
  DateTime ClockNow = RealTimeClock.now();
  CurrentDate = ClockNow.toString(ClockDate);
}

void FetchTimeData(void){
if(WiFi.status() == WL_CONNECTED){
  HttpClient.begin(ApiUrl_Time);
  HttpCode = HttpClient.GET();
  if(HttpCode > 0){
    JsonFile = HttpClient.getString();
    for(int i=0; i<JsonFile.length(); i++){
      if(JsonFile.substring(i, i+4).equals("year") && JsonFile[i+5] == ':'){
        ApiYear = JsonFile.substring(i+6, i+10);
      }
      if(JsonFile.substring(i, i+5).equals("month") && JsonFile[i+6] == ':'){
        if(JsonFile[i+8] == ','){ ApiMonth = JsonFile[i+7]; }
        else { ApiMonth = JsonFile.substring(i+7, i+9); }
      }
      if(JsonFile.substring(i, i+3).equals("day") && JsonFile[i+4] == ':'){
        if(JsonFile[i+6] == ','){ ApiDay = JsonFile[i+5]; }
        else { ApiDay = JsonFile.substring(i+5, i+7); }
      }
      if(JsonFile.substring(i, i+4).equals("hour") && JsonFile[i+5] == ':'){
        if(JsonFile[i+7] == ','){ ApiHour = JsonFile[i+6]; }
        else { ApiHour = JsonFile.substring(i+6, i+8); }
      }
      if(JsonFile.substring(i, i+6).equals("minute") && JsonFile[i+7] == ':'){
        if(JsonFile[i+9] == ','){ ApiMinute = JsonFile[i+8]; }
        else { ApiMinute = JsonFile.substring(i+8, i+10); }
      }
      if(JsonFile.substring(i, i+7).equals("seconds") && JsonFile[i+8] == ':'){
        if(JsonFile[i+10] == ','){ ApiSeconds = JsonFile[i+9]; }
        else { ApiSeconds = JsonFile.substring(i+9, i+11); }
      }
    }
    Serial.println(JsonFile);
    Serial.println(ApiYear + " " + ApiMonth + " " + ApiDay + " " + ApiHour + " " + ApiMinute + " " + ApiSeconds);
    HttpClient.end();
    }
    else{ 
    Serial.println("Error Code"); 
    }
  }
  else{ 
    Serial.println("Connection Lost");
  }
  delay(500);
}

void UpdateRealTimeClock(void){
  FetchTimeData();
  RealTimeClock.adjust(DateTime(ApiYear.toInt(), ApiMonth.toInt(), ApiDay.toInt(), ApiHour.toInt(), ApiMinute.toInt(), ApiSeconds.toInt()));
}

void FetchWeatherData(void){
if(WiFi.status() == WL_CONNECTED){
  HttpClient.begin(ApiUrl_Weather);
  HttpCode = HttpClient.GET();
  if(HttpCode > 0){
    JsonFile = HttpClient.getString();
    for(int i=0; i<JsonFile.length(); i++){
      if(JsonFile.substring(i, i+4).equals("temp") && JsonFile[i+5] == ':'){
      ApiTemperature = JsonFile.substring(i+6, i+11);
      double RealTemp = ApiTemperature.toDouble() - 273.2;
      ApiTemperature = RealTemp;
      }
      if(JsonFile.substring(i, i+8).equals("humidity") && JsonFile[i+9] == ':'){
        ApiHumidity = JsonFile.substring(i+10, i+12);
      }
      if(JsonFile.substring(i, i+11).equals("description") && JsonFile[i+12] == ':'){
        for(int j=i+14; j<i+54; j++){
          if(JsonFile[j] == '"' && JsonFile[j+1] == ','){ ApiDescription = JsonFile.substring(i+14, j); }
        }
      }
    }
    Serial.println(JsonFile); 
    Serial.println(ApiTemperature + " C"); Serial.println(ApiHumidity + " %"); Serial.println(ApiDescription);
    HttpClient.end();
    }
    else{ 
    Serial.println("Error Code"); 
    }
  }
  else{ 
    Serial.println("Connection Lost");
  }
  delay(500);
} 

void UpdateWeatherInfo(void){
  FetchWeatherData();
}