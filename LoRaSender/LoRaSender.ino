#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <ezTime.h>
#include <WiFi.h>


#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             5        // dBm

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

const char* ssid = "V1";
const char* password = "123456789";

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    delay(5000);
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    
    setInterval(60);
    
    waitForSync();
    String val_1 = UTC.dateTime("l, d-M-y H:i:s.v T");

    Serial.println(val_1);
    
    txNumber=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
   }



void loop()
{
  events();
  delay(500);
	if(lora_idle == true)
	{
    
		txNumber += 1;
    
    if(txNumber<=500){

      int64_t tMili = (UTC.dateTime("sv")).toInt();
      int tmiliMin = (UTC.dateTime("i")).toInt();
      int64_t tmiliH = (UTC.dateTime("H")).toInt();
      tMili = tmiliMin * 60000 + tMili + tmiliH * 3600000;
      Serial.println(tMili);
      
//      tMili.toCharArray(txpacket, 30);
      sprintf(txpacket, "%d", tMili);
//      txpacket = tMili;
      
//		  sprintf(txpacket,"%0.2f",txNumber);  //start a package
//      txpacket = static_cast<char[30]>(tMili);
   
		  Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

		  Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out	
      lora_idle = false;
    }
    
    else if(txNumber <= 550){
      
    sprintf(txpacket,"Tx Done");  //start a package
   
    Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

    Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out 
    lora_idle = false;      
      
    }
    else{
      txNumber = 0; //reset
      
      }
	}
  Radio.IrqProcess( );
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}
