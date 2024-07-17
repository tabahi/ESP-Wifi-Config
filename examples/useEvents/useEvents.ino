/*  
  Last tested on 2024-07-16 using ESP32S3 Dev Module (RISC-V ESP32 module by Waveshare)
  Easily manage/store WiFi credentials using ESPWifiConfig library
  When WiFi settings are not configured and the fallback WiFi is not in range then ESP will start in AP_Mode (Access Point)
  Connect your PC/Phone's WiFi to the AP named ESP_XXXXX, then configure the WiFi settings
  Browse to URL 192.168.1.1 to configure the WiFi settings. It will restart after save and will connect to the configured wifi
  Default login: admin, pass_ESP

  Hold 'Config_reset_btn' to LOW for 5 to 10 seconds to reset the WiFi Config when already in CLIENT_MODE
  Add your own button mechanism on 'Config_reset_btn' pin. It's not the power reset button.
  or use WifiConfig.ESP_reset_settings(); to reset config memory, then initialize() again.

  ESPWifiConfig doesn't use SPIFF. It uses virtual EEPROM addresses 0-120 (flash) , which can be lost if compiler configurations are changed during update

  In this code, ESP goes wild and tries to connect to password-less nearby WiFi after 30 minutes of no connection in both the AP_MODE and CLIENT_MODE
  If there is still no WiFi connection then it will automatically Restart.

  Download latest version from:
  https://github.com/tabahi/ESP-Wifi-Config
  (c) 2024  Tabahi.tech https://tabahi.tech/
*/

#include <ESPWifiConfig.h>

int ServerPort = 8080;      //port number for HTTP admin server when in access point (AP) mode.
int Config_reset_btn = -1; //Set to -1 for no reset button pin. Otherwise, setting a pin number here will create a reset button. Pressing this button for more than 5-10sec will reset the WiFi configuration

boolean debug = true; //prints info on Serial when true

ESPWifiConfig WifiConfig("myESP", ServerPort, Config_reset_btn, false, "wifi name", "wifi pass", debug);
/*
   fallback_wifi (second priority wifi) is connected when the configured wifi is not found. To disable fallback leave it empty as:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, false, "", "", debug);
   To overwrite the configured wifi to be the same as fallback wifi set the 4th parameter to true:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, true, "one_and_only_wifi", "wifi_pass", debug);
*/


unsigned int AP_StationConnected = 0;
unsigned long last_wifi_connect_time = 0;
unsigned long reconnect_delay = 10000;


void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Start");
  Serial.println(WiFi.macAddress());

  //AP_MODE is activared when there is no wifi configuration in EEPROM memory and the fallback wifi is not found during scan
  if (WifiConfig.initialize() == AP_MODE) registerEvents(); //Events can be used in CLIENT_MODE too but here we are only using them in AP_MODE
  else AP_StationConnected = 0;

  //WifiConfig.ESP_reset_settings(); //use this to reset the Configuration in flash

  WifiConfig.print_settings();

  if (WifiConfig.ESP_mode == AP_MODE) //uncomment this line to turn on HTTP web server only during AP_MODE
  {
    Serial.println("ESP is running in AP MODE");
    Serial.print("Connect to it's Wifi named: ");
    Serial.println(WifiConfig.get_AP_name());
  }
  WifiConfig.Start_HTTP_Server(600000);//HTTP web server remains active for first 10 minutes (600000 ms) in CLIENT_MODE. Set 0 to run it perpetually. It's perpetually ON in AP_MODE.

  WifiConfig.ESP_debug("Hello");  //this message will show on the Setup web page and on Serial Monitor (if debug = true)
}



void loop()
{
  WifiConfig.handle(reconnect_delay); //non-blocking function, reconnects if disconnected after reconnect_delay


  //Access point mode
  if (WifiConfig.ESP_mode == AP_MODE)  //Can't connect to internet while in this mode
  {
    //Connect to the AP wifi name myESP_XXXXXX, then configure the WiFi

    //Added feature: command String entered on Setup page
    if (WifiConfig.input.length() > 0)
    {
      Serial.println(WifiConfig.input);
      WifiConfig.input = "";
      WifiConfig.ESP_debug("Got cmd");
    }

    if (millis() > 600000) //10 minutes
      ESP.restart();          //Restart after 10 minutes - just in case

  }
  else
  {
    //Client Mode
    if (WifiConfig.ESP_mode == CLIENT_MODE)
    {
      if (WifiConfig.wifi_connected)
      {
        last_wifi_connect_time = millis();
        //All good ===================================== connect to a web server or use internet
        //do something when wifi is connected
        Serial.print(WiFi.status());
        Serial.print("\tConnected to ");
        Serial.print(WiFi.SSID());
        Serial.print("\tStrength: "); //in dB
        Serial.print(WiFi.RSSI()); //in dB
        Serial.print("\tIP: ");
        Serial.println(WiFi.localIP());
        delay(5000);
      }
      else
      {
        //No or lost connection to the WiFi
        reconnect_delay = 20000; //10s by default

        if ((millis() - last_wifi_connect_time) > 3600000) //Restart when no connection for 1 hour
        {
          ESP.restart();
        }
        else if ((millis() - last_wifi_connect_time) > 1800000) // Go wild when no connection for 30 minutes
        {
          //Try to connect all available password-less WiFis. Do it once to avoid extra load on WiFi
          if(WifiConfig.goWild() > 0) // found some NEW unsecure WiFis. If they were found in previous scans, then it will still be zero
          {
            Serial.println("Connecting to open WiFi");
          }
        }
      }
    }
  }



  delay(0);
}








#if BOARD_ESP==32 //WiFi Events are managed differently for ESP32 vs ESP8266. Use the part you need, remove the other board's part

//in case of errors, see the latest version of: Exmples > Wifi >  WiFiClientsEvents
void registerEvents()
{
  // Examples of different ways to register wifi events;
  // these handlers will be called from another thread.
  WiFi.onEvent(WiFiEvent);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFiEventId_t eventID = WiFi.onEvent(
    [](WiFiEvent_t event, WiFiEventInfo_t info) {
      Serial.print("WiFi lost connection. Reason: ");
      Serial.println(info.wifi_sta_disconnected.reason);
    },
    WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );
}



// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiEvent(WiFiEvent_t event) { 
 //copied from Example: WiFiClientsEvents for ESP32
  Serial.printf("[WiFi-event] event: %d\n", event);

  
  //in case of errors, see the latest version of: Exmples > Wifi >  WiFiClientsEvents
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    Serial.println("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        Serial.println("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  Serial.println("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         Serial.println("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        Serial.println("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             Serial.println("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               Serial.println("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                Serial.println("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           Serial.println("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        Serial.println("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              Serial.println("Obtained IP address"); break;
    default:                                    break;
  }
}

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}






#elif BOARD_ESP==8266
//in case of errors, see the latest version of: Exmples > Wifi >  WiFiClientsEvents

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

void registerEvents()
{
  stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
  stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
}

//Wifi Event functions, ignore
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt)
{
  AP_StationConnected++;
  Serial.print(AP_StationConnected);
  Serial.println(F(" WIFI ST CONN"));
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt)
{
  AP_StationConnected--;
  Serial.print(AP_StationConnected);
  Serial.println(F(" WIFI ST DISCONN"));
}

#endif
