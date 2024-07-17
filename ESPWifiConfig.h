
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#if defined(ESP8266)
	#define BOARD_ESP 8266
#elif defined(ESP32)
	#define BOARD_ESP 32
#elif defined(__AVR__)
	#define BOARD_ESP 0
	#error Architecture is AVR instead of ESP8266 OR ESP32
#else
	#define BOARD_ESP 0
	#error Architecture is NOT ESP8266 OR ESP32
#endif

#if BOARD_ESP==32
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiMulti.h>
#elif BOARD_ESP==8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#include <EEPROM.h>
#include <DNSServer.h>
#include  "webpages\\define_vars.h"



#define ESP_settings_size 4
#define EEPROM_SIZE 4096

class ESPWifiConfig;


#define AP_MODE_NAME "AP_MODE"
#define CLIENT_MODE_NAME "CLIENT_MODE"

enum ESP_MODES {
  AP_MODE,
  CLIENT_MODE
};

enum settingsIndex {
  WIFI_SSID,
  WIFI_PASS,
  WEB_USER,
  WEB_PASS
};

#define NAME_MAX_SIZE 10
#define VALUE_MAX_SIZE 30

class SettingsObject
{
	public:
		const char name[NAME_MAX_SIZE];
		char value[VALUE_MAX_SIZE];
		const unsigned char max_size;
		const int addr;
};





class ESPWifiConfig
{
	const char *sys_name;
	char *fallback_ssid;
	char *fallback_ssid_pass;
	boolean fix_ssid;
	
	boolean fallback_ssid_available = false;
	boolean known_ssid_available = false;
	boolean show_debug = false;
	
	int reset_btn = -1;
	int http_port = 80;
#if BOARD_ESP==32
	WebServer server;
#elif BOARD_ESP==8266
	ESP8266WebServer server;
#endif

	
	public:
		ESPWifiConfig(const char *sys_namex, int port, int thisreset_btn, boolean fix_ssidx, char *fallback_ssidx, char *fallback_ssid_passx, boolean debug): server(port)
		{
			sys_name = sys_namex;
			reset_btn = thisreset_btn;
			fix_ssid = fix_ssidx;
			fallback_ssid = fallback_ssidx;
			fallback_ssid_pass = fallback_ssid_passx;
			show_debug = debug;
			http_port = port;
		};
		IPAddress ESP_IP;
		const byte DNS_PORT = 53;
		String get_AP_name();
		IPAddress apIP = {192, 168, 1, 1};
		DNSServer dnsServer;
		int ESP_mode = 0;
		boolean isHTTPserverRunning = false;
		unsigned long last_conn_to_http = 0;
		SettingsObject setting[ESP_settings_size] = {{"WIFI_SSID", "", VALUE_MAX_SIZE, 0}, 
													  {"WIFI_PASS", "", VALUE_MAX_SIZE, VALUE_MAX_SIZE},
													  {"WEB_USER", "admin", VALUE_MAX_SIZE, VALUE_MAX_SIZE*2},
													  {"WEB_PASS", "pass_ESP", VALUE_MAX_SIZE, VALUE_MAX_SIZE*3}};
		

		boolean is_reset_pressed(int);
		void print_settings(void);
		void ESP_reset_settings(void);
		unsigned long getmacID(void);
		int initialize(void);
		boolean wifi_connected = false;
		String error_msg = "";
		unsigned char reset_pressed_count = 0;
		void handle(unsigned long);
		void Start_HTTP_Server(unsigned long);
		int goWild(void);
		void wifiscan(void);
		void ESP_debug(String);
		String debug_log = "";
		String input = "";
		
		
		
	private:
		//void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt);
		//void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt);
		void ESP_read_settings(void);
		void ESP_save_settings(void);
		int wifi_scan_count = 0;
		boolean try_wifi_connect();
		#if BOARD_ESP==32
			WiFiMulti wifiMulti;
		#elif BOARD_ESP==8266
			ESP8266WiFiMulti wifiMulti;
		#endif
		String ESP_IP_addresss = "";
		unsigned long last_try_wifi_connect = 0;
		unsigned long wild_scan_timer = 60000;
		unsigned long client_mode_active_time = 60000;
		unsigned int fails_try_wifi_connect = 0;
		String wild_wifis_list = "";
		
		void handleRoot(void);
		void handleNotFound(void);
		void print_login_page(void);
		void print_setup_page(void);
		void print_css_file(void);
		void handle_error(void);
		bool is_authentified(void);
		
		void handle_login(void);
		void handle_setup(void);
		void handle_ssid_list(void);
		void handle_read_data(void);
		void handle_cout(void);
		
};
