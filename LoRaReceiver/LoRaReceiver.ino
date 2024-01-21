#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ezTime.h>


#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       10         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

const char* ssid = "F1";
const char* password = "123456789";


//Your Domain name with URL path or IP address with path
String serverName = "https://script.google.com/macros/s/AKfycbw-wMymnF2OF8pAk8F9bZfUUv9As4V5gIElEwZSvlqQ9ReXue7TDOHRFCOjldNnGsaoPQ/exec";

bool txSheet = false;

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int64_t txNumber;
int64_t rxNumber;

int64_t maxRSSI=-1000;
int64_t minRSSI=1000;
int64_t totalTimeOnAir;
int64_t minTimeOnAir=100000;
int64_t maxTimeOnAir;
int64_t tolRSSI;
int64_t tolrxSize;
int64_t rxMili,nowMili,defMili;
int64_t tmiliMin,tmiliH;
int64_t rssi,rxSize;

bool lora_idle = true;

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    setInterval(60);
    waitForSync();
    
    String val_1 = UTC.dateTime("l, d-M-y H:i:s.v T");
    int sec_1 = (val_1.substring(24,27)).toInt();
    int msec_1 = (val_1.substring(27,30)).toInt();
    Serial.println(val_1);
  
    
    
    txNumber=0;
    rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
}

void sendDataToSheet(){
   if(WiFi.status()== WL_CONNECTED){
     HTTPClient http;

     String serverPath = serverName + "?SF=" + String(LORA_SPREADING_FACTOR) + "&CR=" + String(LORA_CODINGRATE) + "&txP=" + String(TX_OUTPUT_POWER) + "&tolRxNum=" + String(rxNumber) + "&avgRssi=" + String(tolRSSI/rxNumber) + "&maxRssi=" + String(maxRSSI) + "&minRssi=" + String(minRSSI) + "&avgtoa=" + String(totalTimeOnAir/rxNumber) + "&minton=" + String(minTimeOnAir) + "&maxton=" + String(maxTimeOnAir);
     Serial.println("Total RSSI: " + String(tolRSSI) + " Total Rx Number: " + String(rxNumber) + " Average RSSI: " + String(tolRSSI/rxNumber) + " Max RSSI: " + String(maxRSSI) + " Min RSSI: " + String(minRSSI) + " Total Time on Air: " + String(totalTimeOnAir) + " Min Time on Air: " + String(minTimeOnAir) + " Max Time on Air: " + String(maxTimeOnAir));
     // Your Domain name with URL path or IP address with path
     http.begin(serverPath.c_str());
     // If you need Node-RED/server authentication, insert user and password below
     //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
     // Send HTTP GET request
     int httpResponseCode = http.GET();
     if (httpResponseCode>0) {
       Serial.print("HTTP Response code: ");
       Serial.println(httpResponseCode);
       String payload = http.getString();
       Serial.println(payload);
       delay(1000);
       txSheet = true;
       rxNumber = 0;
       tolRSSI = 0;
       maxRSSI = -1000;
       minRSSI = 1000;
       totalTimeOnAir =0;
       maxTimeOnAir = 0;
       tolrxSize = 0;
       minTimeOnAir=100000;
       
     }
     else {
       Serial.print("Error code: ");
       Serial.println(httpResponseCode);
     }
     // Free resources
     http.end();
   }
   else {
     Serial.println("WiFi Disconnected");
   }
   
 }
  

void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );
//    Serial.print(rxpacket);/
    
    nowMili = (UTC.dateTime("sv")).toInt();
    tmiliMin = (UTC.dateTime("i")).toInt();
    tmiliH= (UTC.dateTime("H")).toInt();
    nowMili = nowMili + tmiliMin*60000 + tmiliH*3600000;
    
    
    if (strcmp(rxpacket, "Tx Done") == 0){
      Serial.print("The test Is done!");
      if (txSheet == false){
        sendDataToSheet();
      }
      else{
        Serial.print("The data has already been sent to the sheet!");
      }
      }
     else{

      txSheet = false;
//      /Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
      rxMili = String(rxpacket).toInt();
      defMili =nowMili-rxMili;
      
      rxNumber += 1;
      tolRSSI += rssi;
      tolrxSize += rxSize;
      totalTimeOnAir += defMili;
      Serial.print(rxNumber);

//      set min max RSSIs

      if(rssi > maxRSSI){
        maxRSSI = rssi;
        }

      if (rssi < minRSSI){
        minRSSI = rssi;
        }
        
      if(defMili > maxTimeOnAir){
        maxTimeOnAir = defMili;
        }

      if (defMili < minTimeOnAir){
        minTimeOnAir = defMili;
        }
         
      }
      
     lora_idle = true;
}
