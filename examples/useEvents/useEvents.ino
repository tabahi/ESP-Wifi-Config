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

int ServerPort = 80;      //port number for HTTP admin server when in access point (AP) mode.
int Config_reset_btn = 0; //GPIO0, D0 on Node32, D3 on NodeMCU8266. Pressing this button for more than 5-10sec will reset the WiFi configuration

boolean debug = true; //prints info on Serial when true

ESPWifiConfig WifiConfig("myESP", ServerPort, Config_reset_btn, false, "emergency_wifi", "emergency_pass", debug);
/*
   fallback wifi (emergency_wifi) is connected when the configured wifi is not found. To disable fallback leave it empty as:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, false, "", "", debug);
   To overwrite the configured wifi to be the same as fallback wifi set the 4th parameter to true:
  WifiConfig WifiConfig("myESP", ServerPort, ESP_reset_btn, true, "fallback_wifi", "fallback_pass", debug);
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

  //WifiConfig.ESP_reset_settings(); //use this to reset the Configuration in EEPROM

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

void registerEvents()
{
  WiFi.onEvent(WiFiEvent);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("WiFi lost connection. Reason: ");
    
    Serial.println(info.wifi_sta_disconnected.reason); //can comment out if gives error
  }, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void WiFiEvent(WiFiEvent_t event) //copied from Example: WiFiClientsEvents for ESP32
{
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {

    case SYSTEM_EVENT_WIFI_READY:
      Serial.println(F("WiFi interface ready"));
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println(F("Completed scan for access points"));
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println(F("WiFi client started"));
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println(F("WiFi clients stopped"));
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println(F("Connected to access point"));
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println(F("Disconnected from WiFi access point"));
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println(F("Authentication mode of access point has changed"));
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println(("Lost IP address and IP address is reset to 0"));
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println(F("WiFi Protected Setup (WPS): succeeded in enrollee mode"));
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println(F("WiFi Protected Setup (WPS): failed in enrollee mode"));
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println(F("WiFi Protected Setup (WPS): timeout in enrollee mode"));
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println(F("WiFi Protected Setup (WPS): pin code in enrollee mode"));
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println(F("IPv6 is preferred"));
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println(F("Ethernet started"));
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println(F("Ethernet stopped"));
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println(F("Ethernet connected"));
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println(F("Ethernet disconnected"));
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println(F("Obtained IP address"));
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println(F("WiFi access point started"));
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println(F("WiFi access point  stopped"));
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      AP_StationConnected++;
      Serial.println(F("Client connected"));
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      if (AP_StationConnected > 0) AP_StationConnected--;
      Serial.println(F("Client disconnected"));
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println(F("Assigned IP address to client"));
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println(F("Received probe request"));
      break;
    default: break;
  }
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}


#elif BOARD_ESP==8266

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
