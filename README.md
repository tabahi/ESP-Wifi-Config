# ESP-Wifi-Config
 Easily manage/store WiFi credentials for ESP32 and ESP8266

## Usage

- When WiFi settings are not configured and the fallback WiFi is not in range then ESP will start in AP_Mode (Access Point). Connect your PC/Phone's WiFi to the AP named ESP_XXXXX, then configure the WiFi settings
- Browse to URL `192.168.1.1` to configure the WiFi settings. It will restart after sav
e and will connect to the configured wifi
Default login: admin, pass_ESP

- To reset the already known configurations, hold `Config_reset_btn` to `LOW` for 5 to 10 seconds when already in `CLIENT_MODE`. Add your own button mechanism on `Config_reset_btn` pin. It's not the power reset button. Or use  `WifiConfig.ESP_reset_settings()` to reset config memory, then `initialize()` again.


- Use `WifiConfig.goWild()` to connect to password-less nearby WiFi. In the example, it's used as the last resort when the configured and fallback WiFis are not found. If there is still no WiFi connection then it will automatically Restart after an hour.



## Example

See the `examples\ensureWiFi.ino` for a complete example.


```cpp

#include <ESPWifiConfig.h>

ESPWifiConfig WifiConfig("myESP", 80, Config_reset_btn, false, "fallback_wifi", "fallback_pass", debug_true);

void setup()
{
  Serial.begin(115200);
  delay(100);

  //AP_MODE is activared when there is no wifi configuration in EEPROM memory and the fallback wifi is not found during scan
  if (WifiConfig.initialize() == AP_MODE)
  {
    WifiConfig.Start_HTTP_Server(600000);
    //Start HTTP server when initialized in AP_MODE probably because configuration is not found, and fallback_wifi isn't in range
    //HTTP web server remains active for first 10 minutes (600000 ms) if it's switched into CLIENT_MODE. Set 0 to run it perpetually. It's perpetually ON in AP_MODE.
  }

  //WifiConfig.ESP_reset_settings(); //use this to reset the Configuration in EEPROM

  WifiConfig.print_settings();

  WifiConfig.ESP_debug("Hello");  //this message will show on the Setup web page
}


void loop()
{
  //non-blocking function, 'reconnect_delay' is an interval for reconnection if disconnected
  WifiConfig.handle(reconnect_delay); 
 
  if (ensure_wifi_connectivity(NO_CONNECTION_GO_WILD_DELAY, NO_CONNECTION_RESTART_DELAY))
  {
    //do something when wifi is connected
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
        else if ((millis() - last_wifi_connect_time) > no_conn_go_wild_delay) 
        {
          //Try to connect to password-less WiFis after 1/2 hour of NO Wifi
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
```

## Editing


`ESPWifiConfig` doesn't use SPIFF. It uses virtual EEPROM addresses 0-80 (flash) , which can be lost if compiler configurations are changed during update

Web pages can be edited in 'define_vars.h' where the PROGMEM constants are defined.
'define_vars.h' defines the is the condensed strings of all webpages.

To change the content of webpages, change the html files then paste them as condensed strings in 'define_vars.h'.

## Screenshots
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/1.png =200x)
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/2.png =200x)
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/3.png =200x)
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/4.png =200x)
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/5.png =200x)
![Preview](https://raw.githubusercontent.com/tabahi/ESP-Wifi-Config/master/screenshots/6.png =200x)


