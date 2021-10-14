/*
    Easily manage/store WiFi credentials using ESPWifiConfig library
    When WiFi settings are not configured and the fallback WiFi is not in range then ESP will start in AP_Mode (Access Point)
    Connect your PC/Phone's WiFi to the AP named ESP_XXXXX, then configure the WiFi settings
    Browse to URL 192.168.1.1 to configure the WiFi settings. It will restart after save and will connect to the configured wifi
    Default login: admin, pass_ESP

    Hold 'Config_reset_btn' to LOW for 5 to 10 seconds to reset the WiFi Config when already in CLIENT_MODE
    Add your own button mechanism on 'Config_reset_btn' pin. It's not the power reset button.
    or use WifiConfig.ESP_reset_settings(); to reset config memory, then initialize() again.

    ESPWifiConfig doesn't use SPIFF. It uses virtual EEPROM addresses 0-80 (flash) , which can be lost if compiler configurations are changed during update

    In this code, ESP goes wild and tries to connect to password-less nearby WiFi after 30 minutes of no connection in both the AP_MODE and CLIENT_MODE
    If there is still no WiFi connection then it will automatically Restart.

    Download latest version from:
    https://github.com/tabahi/ESP-Wifi-Config
    (c) 2021  Tabahi.tech https://tabahi.tech/
*/

#include <ESPWifiConfig.h>

int ServerPort = 80;
int Config_reset_btn = 0; //GPIO0, D0 on Node32, D3 on NodeMCU8266. Pressing this button for more than 5-10sec will reset the WiFi configuration

boolean debug = true; //prints info on Serial when true

ESPWifiConfig WifiConfig("myESP", ServerPort, Config_reset_btn, false, "fallback_wifi", "fallback_pass", debug);
/*
   fallback wifi is connected when the configured wifi is not found. To disable fallback leave it empty as:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, false, "", "", debug);
   To overwrite the configured wifi to be the same as fallback wifi set the 4th parameter to true:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, true, "fallback_wifi", "fallback_pass", debug);
*/

#define NO_CONNECTION_RESTART_DELAY 3600000 //ms, 1 hour
#define NO_CONNECTION_GO_WILD_DELAY 1800000 //ms, 30 minutes


unsigned long last_wifi_connect_time = 0;
unsigned long reconnect_delay = 10000;


void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Start");
  Serial.println(WiFi.macAddress());

  //AP_MODE is activared when there is no wifi configuration in EEPROM memory and the fallback wifi is not found during scan
  if (WifiConfig.initialize() == AP_MODE)
  {
    WifiConfig.Start_HTTP_Server(600000); //Start HTTP server when initialized in AP_MODE probably because configuration is not found, and fallback_wifi isn't in range
    //HTTP web server remains active for first 10 minutes (600000 ms) if it's switched into CLIENT_MODE. Set 0 to run it perpetually. It's perpetually ON in AP_MODE.
  }

  //WifiConfig.ESP_reset_settings(); //use this to reset the Configuration in EEPROM

  WifiConfig.print_settings();

  WifiConfig.ESP_debug("Hello");  //this message will show on the Setup web page
}



void loop()
{
  
  WifiConfig.handle(reconnect_delay); //non-blocking function, 'reconnect_delay' is an interval for reconnection if disconnected


  if (ensure_wifi_connectivity(NO_CONNECTION_GO_WILD_DELAY, NO_CONNECTION_RESTART_DELAY))
  {
    //do something when wifi is connected
    Serial.print(WiFi.status());
    Serial.print("\tConnected to ");
    Serial.print(WiFi.SSID());
    Serial.print("\tStrength: "); //in dB
    Serial.print(WiFi.RSSI()); //in dB
    Serial.print("\tIP: ");
    Serial.println(WiFi.localIP());
    delay(1000);
  }
  else
  {
    //do something else when wifi is not connected
  }

  delay(0);
}






boolean ensure_wifi_connectivity(unsigned long no_conn_go_wild_delay, unsigned long no_conn_restart_delay)
{
  //Access point mode
  if (WifiConfig.ESP_mode == AP_MODE)  //Can't connect to internet while in this mode
  {
    //Connect to the AP wifi name myESP_XXXXXX
  

    //Added feature: Go wild after NO CONNECTION for a while
    //Try to connect to all password-less WiFis in range
    if (millis() > no_conn_go_wild_delay)
    {
      WifiConfig.goWild(); //(it switches to CLIENT_MODE)
    }
  }
  else
  {
    //Client Mode
    if (WifiConfig.ESP_mode == CLIENT_MODE)
    {
      if (WifiConfig.wifi_connected)
      {
        last_wifi_connect_time = millis();
        reconnect_delay = 10000;
        return true;
        //All good ===================================== connect to a web server or use internet
      }
      else
      {
        //No or lost connection to the WiFi
        
        reconnect_delay = 30000; //10s by default

        if ((millis() - last_wifi_connect_time) > no_conn_restart_delay) //Restart after 1 hour of no connection
        {
          Serial.println("Restarting...");
          ESP.restart();          
        }
        else if ((millis() - last_wifi_connect_time) > no_conn_go_wild_delay) //Try to connect to password-less WiFis after 1/2 hour of NO Wifi
        {
          if (WifiConfig.goWild() > 0)
          {
            // found some NEW unsecure WiFis. If they were found in previous scans, then it will still be zero
            Serial.println("Connecting to open WiFi");
          }
        }
      }
    }
  }
  return false;
}
