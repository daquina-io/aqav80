#include <Arduino.h>
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>         //https://github.com/daquina-io/WiFiManager
//#define DEBUGGING

#ifdef  DEBUGGING
#define DMSG(args...)     Serial.print(args)
#define DMSGf(args...)    Serial.printf(args)
#define DMSGln(args...)   Serial.println(args)
#else
#define DMSG(args...)
#define DMSGf(args...)
#define DMSGln(str)
#endif

int contador = 0;

  unsigned long previousMillis = 0;
  unsigned long interval = 10000;
  
   void conexion() {
      unsigned long currentMillis = millis();
      // si el WiFi no funciona, intenta volver a conectarte cada CHECK_WIFI_TIME segundos
      if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
          DMSG(millis());
          DMSGln("  Reconectando a la red WiFi...");
          WiFi.disconnect();
          WiFi.reconnect();
          previousMillis = currentMillis;
      } 

  if ((WiFi.status() == WL_CONNECTED)){
      DMSG(" Estas conectado a la red ");
      DMSG(" - IP address: ");
      DMSGln(WiFi.localIP());

      contador = 0;
      delay(1000);
  } else{
      contador++;
      DMSG(" No stas conectado a la red ");
      DMSG(" - IP address: ");
      DMSG(WiFi.localIP());
      DMSG(" - CONTADOR ");
      DMSGln(contador);  
      delay(1000);
          if(contador >=60){
            contador = 0;
            ESP.restart();

          }
   
      
  }


  }


