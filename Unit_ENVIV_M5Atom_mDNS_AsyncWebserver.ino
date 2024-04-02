/*
*******************************************************************************
* Copyright (c) 2023 by M5Stack
*                  Equipped with M5Core sample source code
*                          配套  M5Core 示例源代码
* Visit to get information: https://docs.m5stack.com/en/unit/ENV%E2%85%A3%20Unit
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/ENV%E2%85%A3%20Unit
*
* Product: ENVIV_SHT40_BMP280.  环境传感器
* Date: 2023/8/24
*******************************************************************************
  Please connect to Port,Read temperature, humidity and atmospheric pressure and
  display them on the display screen
  请连接端口,读取温度、湿度和大气压强并在显示屏上显示
    Libraries:
  - [Adafruit_BMP280](https://github.com/adafruit/Adafruit_BMP280_Library)
  - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
  - [arduino-i2c-sht4x](https://github.com/Sensirion/arduino-i2c-sht4x)
*/

/*
  [ESPAsyncWebServer] https://github.com/me-no-dev/ESPAsyncWebServer?tab=readme-ov-file
  [ASyncTCP]  https://github.com/me-no-dev/AsyncTCP

  Document
  // http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc214/doc21403.html
  // http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc215/doc21501.html
*/

/*
 * You can use ESPtouch
 * https://play.google.com/store/apps/details?id=com.dparts.esptouch&hl=en_US&pli=1
 * https://apps.apple.com/us/app/espressif-esptouch/id1071176700
 */

#include "M5Atom.h"
//#include <M5Stack.h>
/*
#include <SensirionI2CSht4x.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_Sensor.h"
*/
#include <SensirionI2CSht4x.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#define HTTP_PORT 80
#define FASTLED_ALL_PINS_HARDWARE_SPI

const char* MDNS_NAME="M5Atom-0001";


// 初始化传感器
/*Adafruit_BMP280 bmp;
SensirionI2CSht4x sht4x;
*/
SensirionI2CSht4x sht4x;
Adafruit_BMP280 bmp;

float pressure,t,h;  // Store the vuale of pressure and Temperature.  存储压力和温度

AsyncWebServer server(HTTP_PORT);

/*----
  HTML
*/
const char* strHtml = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto;text-align: center;} 
      h1 {font-size:28px;}
      body {text-align: center;} 
      table { border-collapse: collapse; margin-left:auto; margin-right:auto;}
      th { padding: 12px; background-color: #0000cd; color: white; border: solid 2px #c0c0c0;}
      tr { border: solid 2px #c0c0c0; padding: 12px;}
      td { border: solid 2px #c0c0c0; padding: 12px;}
      .value { color:blue; font-weight: bold; padding: 1px;}
    </style>
  </head>
  <body>
    <h1>Asynchronous Server</h1>
    <p style='color:brown; font-weight: bold'>計測値は10秒ごとに自動更新されます!</p>
    <p><table>
      <tr><th>ELEMENT</th><th>VALUE</th></tr>
      <tr><td>Temperature</td><td><span id="temperature" class="value">%TEMPERATURE%</span>
      <tr><td>Humidity</td><td><span id="humidity" class="value">%HUMIDITY%</span>
      <tr><td>Pressure</td><td><span id="pressure" class="value">%PRESSURE%</span>
      <tr><td>Current time</td><td><span id="curtime" class="value">yyyy/mm/dd<br>hh:nn:ss</span>
      </td></tr>
    </table></p>
  </body>
  <script>
    var getTemperature = function () {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("temperature").innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/temperature", true);
      xhr.send(null);
    }
    var getHumidity = function () {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("humidity").innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/humidity", true);
      xhr.send(null);
    }
    var getPressure = function () {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("pressure").innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/pressure", true);
      xhr.send(null);
    }
    var getCurTime = function () {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("curtime").innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/curtime", true);
      xhr.send(null);
    }
    setInterval(getTemperature, 10000);
    setInterval(getHumidity, 10000);
    setInterval(getPressure, 10000);
    setInterval(getCurTime, 500);
  </script>
</html>)rawliteral";

/*----
 */

void wifi_connect(void){
  Serial.println("WiFi begin");
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if ( 10000 < millis() ) {
      break;
    }
  }
  Serial.println("");
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    Serial.println("Waiting for SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print("#");
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    // Wi-fi接続
    Serial.println("");
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    Serial.println("");
    Serial.println("WiFi Connected.");
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

String getTemperature() {
    /*uint16_t error;
    char errorMessage[256];
    float t,h;
    error = sht4x.measureHighPrecision(t, h);
    if (error) {
      errorToString(error, errorMessage, 256);
    } else {
      if (isnan(t)) {    
       //   Serial.println("Failed to get temperature!");
          return "--";
      }
      else {
      //    Serial.println(t);
          return String(t) + " &#8451;";
      }
    }*/
    return String(t) + " &#8451;";
}
String getHumidity() {
//    float h = dht.readHumidity();
    /*float t,h ;
    uint16_t error;
    if (error) {
      //h = sht4x.measureHighPrecision(t,h);
      if (isnan(h)) {
        //  Serial.println("Failed to get humidity!");
          return "--";
      }
      else {
       // Serial.println(h);
          return String(h) + " &#37;";
    }
  }*/
  return String(h) + " &#37;";
}
String getPressure() {
    float p ;
    p = bmp.readPressure();
    p = p * 0.01;
    if (isnan(p)) {
      //  Serial.println("Failed to get humidity!");
        return "--";
    }
    else {
        //Serial.println(p);
        return String(p) + " hPa";
    }
}
String getCurTime(){
    struct tm timeinfo;
    char buf[64];
 
    if(getLocalTime(&timeinfo)) {
        sprintf(buf,"%04d/%02d/%02d %02d:%02d:%02d",
              timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
       // Serial.println(buf);
        return buf;
    }
}
String editPlaceHolder(const String& var){
    if(var == "TEMPERATURE"){
        return getTemperature();
    }
    else if(var == "HUMIDITY"){
        return getHumidity();
    }
    //Pressure
    else if(var == "PRESSURE"){
        return getPressure();
    }
    //ntp
    else if(var == "CURTIME"){
        return getCurTime();
    }
    return "??";
}

void setup() {
    // 初始化传感器
    M5.begin(true,false,true);
/*    M5.Power.begin();  // Init power  初始化电源模块
    M5.Lcd.setTextSize(2);
*/  
    pinMode(0,OUTPUT);
    digitalWrite(0,LOW);
    M5.dis.drawpix(0, 0x000000);
    Wire.begin();  // SDA = 16, SCL = 34
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
  WiFi.beginSmartConfig();
  wifi_connect();
  configTime(9*3600L, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  if(!MDNS.begin(MDNS_NAME)){
    Serial.print("Error MDNS_NAME:");
    Serial.println(MDNS_NAME);
    delay(10000);
    ESP.restart();
  }
    uint16_t error;
    char errorMessage[256];
    sht4x.begin(Wire);
    uint32_t serialNumber;
    error = sht4x.serialNumber(serialNumber);
    if (error) {
        Serial.print("Error trying to execute serialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Serial Number: ");
        Serial.println(serialNumber);
    }
  unsigned status;
  status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  //status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    
    // GETリクエストに対するハンドラーを登録して
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", strHtml, editPlaceHolder);
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", getTemperature().c_str());
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", getHumidity().c_str());
    });
    //pressure
    server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", getPressure().c_str());
    });
    //ntp
    server.on("/curtime", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", getCurTime().c_str());
    });
    // サーバーを開始する
    server.begin();
    M5.dis.drawpix(0, 0x00ff00);
}


void loop() {
  float p;
  uint16_t error;
  static char pressure2[10];
  //error=sht4x.measureHighPrecision(temperature, humidity);
        Serial.print("Temperature:");
        Serial.print(t);
        Serial.print(",");
        Serial.print("Humidity:");
        Serial.print(h);
        Serial.print(",");
        p=bmp.readPressure();
        p=p*0.01;
        dtostrf(p,7,2,pressure2);
        Serial.print("Pressure:");
        Serial.println(pressure2);
        
    
  uint16_t error_sht4x;
  float temperature, humidity;
  char errorMessage[256];
  error_sht4x=sht4x.measureHighPrecision(temperature,humidity);
  if(error_sht4x){
    errorToString(error_sht4x,errorMessage,256);
    Serial.println(errorMessage);
  }else{
    t=temperature;
    h=humidity;
  }
  /*  M5.Lcd.setCursor(0, 0);  // 将光标设置在(0 ,0).  Set the cursor to (0,0)
    M5.Lcd.printf("Pressure:%2.0fPa\nTemperature:%2.0f^C", pressure,
                  temperature);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("humidity:");
    M5.Lcd.print(humidity);
    M5.Lcd.print("%");
    */
    delay(2000);
}
