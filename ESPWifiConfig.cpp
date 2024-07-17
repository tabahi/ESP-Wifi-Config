
#include "ESPWifiConfig.h"



int ESPWifiConfig::initialize(void)
{
	
	if (reset_btn>=0)
		pinMode(reset_btn, INPUT_PULLUP);
	debug_log.reserve(50);
	
	WiFi.persistent(false);
	
	ESP_read_settings();
	//print_settings();
	ESP_debug(sys_name);
	WiFi.disconnect();
	delay(100);
	wifiscan();
	delay(100);
	if((!known_ssid_available) && (!fallback_ssid_available))
	{
		delay(2000);	
		wifiscan();
	}
	
	if(fix_ssid)
	{
		ESP_debug(F("WiFi mem overwrite"));
		for(uint8_t i =0; i<VALUE_MAX_SIZE; i++)
		{ setting[WIFI_SSID].value[i] = fallback_ssid[i]; if(fallback_ssid[i]=='\0') break; }
		for(uint8_t i =0; i<VALUE_MAX_SIZE; i++)
		{ setting[WIFI_PASS].value[i] = fallback_ssid_pass[i]; if(fallback_ssid_pass[i]=='\0') break; }
		ESP_save_settings();
	}
	
	if(((setting[WIFI_SSID].value[0] >= 32) && (setting[WIFI_SSID].value[0] <= 122)) || ((fallback_ssid[0] >= 32) && (fallback_ssid[0] <= 122) && (fallback_ssid_available)))	//ascii 0
	{
		ESP_mode = CLIENT_MODE;
		wifi_connected = false;
		ESP_debug(CLIENT_MODE_NAME);
		delay(500);
		WiFi.mode(WIFI_STA);
		
		
		if ((setting[WIFI_SSID].value[0] >= 32) && (setting[WIFI_SSID].value[0] <= 126))
		{
			ESP_debug(F("Configured WiFi:"));
			ESP_debug(setting[WIFI_SSID].value);
			ESP_debug(setting[WIFI_PASS].value);
			wifiMulti.addAP(setting[WIFI_SSID].value, setting[WIFI_PASS].value);
			
			if(!known_ssid_available) { ESP_debug(F("Not found:")); ESP_debug(setting[WIFI_SSID].value);}
		}
		
		if((fallback_ssid[0] >= 32) && (fallback_ssid[0] <= 126) )
		{
			ESP_debug(F("Fallback:"));
			ESP_debug(fallback_ssid);
			ESP_debug(fallback_ssid_pass);
			wifiMulti.addAP(fallback_ssid, fallback_ssid_pass);
			if(!fallback_ssid_available) {ESP_debug(F("Not found:")); ESP_debug(fallback_ssid);}
		}
		try_wifi_connect();
		
	}
	else
	{
		ESP_mode = AP_MODE;
		WiFi.mode(WIFI_AP);
		ESP_debug(AP_MODE_NAME);
		//ESP_debug("AP Mode. SSID: " + setting[AP_SSID].value + "\tPASS: " + setting[AP_PASS].value);
		/* You can remove the password parameter if you want the AP to be open. */
		WiFi.disconnect();
		delay(500);
		
		WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
		
		char this_ap[30];
		strcpy(this_ap, sys_name);
		strcat(this_ap, "_");
		strcat(this_ap, String(getmacID()).c_str());
		
		ESP_debug("AP Wifi name:");
		ESP_debug(this_ap);
		//if(setting[WEB_PASS].value[0]!='\0') WiFi.softAP(this_ap, setting[WEB_PASS].value);
		WiFi.softAP(this_ap);
		
		dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer.start(DNS_PORT, "*", apIP);
		delay(1000);
		ESP_IP = WiFi.softAPIP();
		ESP_IP_addresss = "";
		ESP_IP_addresss += String(ESP_IP[0]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[1]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[2]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[3]);
		ESP_debug(ESP_IP_addresss);
		//wifiscan();
		ESP_debug("Setup URL:\r\nhttp://" + ESP_IP_addresss + ":" + String(http_port));
	}
	
	// Setup MDNS responder
	if (!MDNS.begin(sys_name))
	{
	  ESP_debug("Warn: Can't setup MDNS");
	}
	else
	{
	  ESP_debug("Setup URL:\r\nhttp://" +String(sys_name) + ".local:" + String(http_port));
	  // Add service to MDNS-SD
	  MDNS.addService("http", "tcp", http_port);
	}
	
	return ESP_mode;
}

String ESPWifiConfig::get_AP_name()
{
	return String(sys_name)+"_"+String(getmacID());
}

int ESPWifiConfig::goWild(void)
{
	if((millis() - wild_scan_timer) > 60000) //wait 60s between wild scans
	{
		wild_scan_timer = millis();
		ESP_debug("Going wild");

		if(ESP_mode!=CLIENT_MODE)
		{
			ESP_mode = CLIENT_MODE;
			WiFi.mode(WIFI_STA);
			ESP_debug(CLIENT_MODE_NAME);
			delay(500);
		}
		else WiFi.disconnect();
		
		
		wifi_connected = false;
		delay(500);
		wifiscan();
		int count = 0;
		if (wifi_scan_count > 0)
		{
			for (int i = 0; i < wifi_scan_count; ++i)
			{
	#if BOARD_ESP==32
				if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
				{
					if(wild_wifis_list.indexOf(WiFi.SSID(i))<0)
					{
						wifiMulti.addAP(WiFi.SSID(i).c_str(), NULL);
						ESP_debug("No pass for: " + WiFi.SSID(i));
						count++;
						wild_wifis_list += WiFi.SSID(i);
					}
				}
	#elif BOARD_ESP==8266
				if (WiFi.encryptionType(i) == ENC_TYPE_NONE)
				{
					if(wild_wifis_list.indexOf(WiFi.SSID(i))<0)
					{
						wifiMulti.addAP(WiFi.SSID(i).c_str(), NULL);
						ESP_debug("No pass for: " + WiFi.SSID(i));
						count++;
						wild_wifis_list += WiFi.SSID(i);
					}
				}
	#endif
				delay(100);
			}
		}
		if((count > 0) && (wild_wifis_list.length()>100)) wild_wifis_list = wild_wifis_list.substring(0, 99);
		
		if (count > 0) try_wifi_connect();
		
		return count;
	}
	return 0;
}

boolean ESPWifiConfig::try_wifi_connect()
{
	ESP_debug("WIFI-CONN-TRY");
	if(wifiMulti.run() == WL_CONNECTED) 
	{
		ESP_debug(F("WiFi connected"));
		ESP_debug(F("IP: "));
		ESP_IP = WiFi.localIP();
		ESP_IP_addresss = "";
		ESP_IP_addresss += String(ESP_IP[0]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[1]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[2]);
		ESP_IP_addresss += ".";
		ESP_IP_addresss += String(ESP_IP[3]);
		ESP_debug(ESP_IP_addresss);
		wifi_connected = true;
	}
	else
	{
		if(wifi_connected) ESP_debug("WiFi disconnected!");
		wifi_connected = false;
	}
	return wifi_connected;
}

void ESPWifiConfig::wifiscan()
{

	wifi_scan_count = WiFi.scanNetworks();

	ESP_debug(F("Scanning Wifi"));
	ESP_debug("Found: " + String(wifi_scan_count));

	if (wifi_scan_count > 0)
	{
		for (int i = 0; i < wifi_scan_count; ++i)
		{
			ESP_debug(WiFi.SSID(i));
			/*
			// Print SSID and RSSI for each network found
			Serial.printf("%d", i + 1);
			Serial.printf(": ");
			Serial.printf("%s", WiFi.SSID(i).c_str());
			Serial.printf(" (");
			Serial.printf("%d", WiFi.RSSI(i));
			Serial.printf(")");
			Serial.printf("%s", (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
			Serial.printf("\n");
			*/
			if((setting[WIFI_SSID].value[0] >= 32) && (setting[WIFI_SSID].value[0] <= 126))	//ascii 0
			{
				if( WiFi.SSID(i).indexOf(setting[WIFI_SSID].value)>=0)
					known_ssid_available = true;
			}
			if((fallback_ssid[0] >= 32) && (fallback_ssid[0] <= 126) )
			{
				if( WiFi.SSID(i).indexOf(fallback_ssid)>=0)
					fallback_ssid_available = true;
			}
		}
	}
}


void ESPWifiConfig::Start_HTTP_Server(unsigned long client_mode_active_time_x)
{
	client_mode_active_time = client_mode_active_time_x;
	server.on("/", std::bind(&ESPWifiConfig::handleRoot, this));
	server.on("/styles", std::bind(&ESPWifiConfig::print_css_file, this));
	server.on("/error", std::bind(&ESPWifiConfig::handle_error, this));
	server.on("/login", std::bind(&ESPWifiConfig::handle_login, this));

	server.on("/setup", std::bind(&ESPWifiConfig::handle_setup, this));
	server.on("/readSettings", std::bind(&ESPWifiConfig::handle_read_data, this));
	server.on("/readSSID", std::bind(&ESPWifiConfig::handle_ssid_list, this));
	server.on("/readCout", std::bind(&ESPWifiConfig::handle_cout, this));


	server.onNotFound(std::bind(&ESPWifiConfig::handleNotFound, this));
	  const char * headerkeys[] = {"User-Agent", "Cookie"} ;
	  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

	  //ask server to track these headers
	server.collectHeaders(headerkeys, headerkeyssize );

	server.begin();
	isHTTPserverRunning = true;
	ESP_debug(F("HTTP server started\n"));
}

void ESPWifiConfig::handle(unsigned long reconnect_delay)
{
	if((ESP_mode != AP_MODE) && (((millis() - last_try_wifi_connect) > reconnect_delay) || (wifi_connected &&(WiFi.status() != WL_CONNECTED))))
	{
		
		if((reset_btn>=0) && (is_reset_pressed(reset_btn)))
		{
			reset_pressed_count++;
			if(reset_pressed_count>=2)
			{
				ESP_debug(F("ESP:Settings RESET"));
				ESP_reset_settings();
				ESP.restart();
				//initialize();
			}
		}
		else
			reset_pressed_count=0;
		last_try_wifi_connect = millis();
		
		if (WiFi.status() != WL_CONNECTED)
		{
			if(try_wifi_connect()) fails_try_wifi_connect = 0;
			else fails_try_wifi_connect++;
		}
		else
		{
			wifi_connected = true;
			fails_try_wifi_connect = 0;
		}
	}
	if(isHTTPserverRunning)
	{
		dnsServer.processNextRequest();
		server.handleClient();
		if((ESP_mode == CLIENT_MODE) && (client_mode_active_time>0) && (millis() > client_mode_active_time))
		{
			isHTTPserverRunning = false;
			ESP_debug(F("HTTP server closed\n"));
			server.close();
			dnsServer.stop();
		}
	}
}

void ESPWifiConfig::handle_setup()
{
  //ESP_debug(F("ESP:req_setup"));
  delay(0);
  yield();
  if (!is_authentified())
  {
    server.sendContent(F("HTTP/1.1 301 OK\r\nLocation: ./login\r\nCache-Control: no-cache\r\n\r\n"));
    return;
  }
  if (server.hasArg("WIFI_SSID_un") && (server.arg("WIFI_SSID_un").length() > 0))
  {
	server.arg("WIFI_SSID_un").toCharArray(setting[WIFI_SSID].value, VALUE_MAX_SIZE);
	
    if (server.hasArg("WIFI_PASS"))
    {
		server.arg("WIFI_PASS").toCharArray(setting[WIFI_PASS].value, VALUE_MAX_SIZE);
    }
    else
    {
      setting[WIFI_PASS].value[0] = '\0';
    }
    delay(0);
    yield();
    ESP_save_settings();
	ESP.restart();
  }
  else if ((server.hasArg("WIFI_SSID_list")) && (server.arg("WIFI_SSID_list").length() > 0))
  {
	server.arg("WIFI_SSID_list").toCharArray(setting[WIFI_SSID].value, VALUE_MAX_SIZE);
	
	
    if (server.hasArg("WIFI_PASS"))
    {
		server.arg("WIFI_PASS").toCharArray(setting[WIFI_PASS].value, VALUE_MAX_SIZE);
    }
    else
    {
      setting[WIFI_PASS].value[0] = 0;
    }
    delay(0);
    yield();
    ESP_save_settings();
	ESP.restart();
  }


  for (int i = 2; i < ESP_settings_size; i++)
  {
    if (server.hasArg(setting[i].name))
    {
		server.arg(setting[i].name).toCharArray(setting[i].value, VALUE_MAX_SIZE);
		ESP_save_settings();
    }
  }
  delay(0);
  yield();

  print_setup_page();
  delay(0);
  yield();
}




bool ESPWifiConfig::is_authentified(void) {

	last_conn_to_http = millis();
  //Serial.printf("Enter is_authentified\n");
  if (server.hasHeader("Cookie"))
  {
    //Serial.printf("Found cookie: ");
    String cookie = server.header("Cookie");
    //Serial.printf(cookie.c_str());
    //Serial.printf("\r\n");
    if (cookie.indexOf("ESPSESSIONID=1") != -1)
    {
      //Serial.println("Authentification Successful");
      return true;
    }
  }
  //Serial.printf("Authentification Failed\n");
  return false;
}

void ESPWifiConfig::handle_login(void)
{
  yield();
  //ESP_debug(F("ESP:handleLogin"));
  if (server.hasHeader("Cookie"))
  {
    //Serial.printf("Found cookie: ");
    String cookie = server.header("Cookie");
    //Serial.printf(cookie.c_str());
    //Serial.printf("\r\n");
  }
  if (server.hasArg("DISCONNECT"))
  {
    //Serial.printf("Disconnection\n");
    server.sendContent(F("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: ./login\r\nCache-Control: no-cache\r\n\r\n"));
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
  {
    if (server.arg("USERNAME") == setting[WEB_USER].value &&  server.arg("PASSWORD") == setting[WEB_PASS].value)
    {
      server.sendContent(F("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n"));
      //Serial.printf("Log in Successful\n");
      return;
    }
    error_msg += F("Wrong username/password! try again. ");
    ESP_debug("Log in Failed\n");
	ESP_debug(setting[WEB_USER].value);
	ESP_debug(setting[WEB_PASS].value);
  }
  yield();
  print_login_page();
  yield();
}

boolean check_valid_ascii(String strr)
{
	int this_strr_len = strr.length();
	if(this_strr_len>32)
		return false;
	for(int k=0; k<this_strr_len; k++)
	{
		if((strr[k]<32) || (strr[k]>126))
		{
			return false;
		}
	}
	return true;
}

void ESPWifiConfig::handle_ssid_list()
{
  //ESP_debug(F("ESP:req_ssid_list"));

  if(ESP_mode!=AP_MODE) wifiscan();	//doesn't work in AP mode for both 8266 and 32
	  
  
  String content =  "[ ";
  delay(0);
  yield();
	if (wifi_scan_count > 0)
	{
		for (int i = 0; i < wifi_scan_count; ++i)
		{
			boolean skip_this_ssid = false;
			for(int k=0; k<i; k++)
			{
				if(WiFi.SSID(k)==WiFi.SSID(i))
				{
				  skip_this_ssid = true;
				}
			}
			if((!skip_this_ssid)  &&  (check_valid_ascii(WiFi.SSID(i))))
			{
				content += F("{\"SSID\" : \"");
				content += String(WiFi.SSID(i));
				content += F("\"},");
			}
		}
	}
  content[content.length()-1] = ' ';

  content += "]";
  //ESP_debug(F("\nSSID list:"));
  //ESP_debug(content);
  delay(0);
  yield();
  server.send(200, F("application/json"), content);
  delay(0);
  yield();
}


void ESPWifiConfig::handle_read_data(void)
{
	//ESP_debug(F("ESP:req_read_data"));
	String content =  F("data = [{");
	content += F("\"IPAddress\" : \"");
	content += ESP_IP_addresss;
	content += F("\",\"MAC\":\"");
	content += String(getmacID());
	content += F("\",\"Port\":\"");
	content += String(http_port);
	content += F("\",\"System\":\"");
	content += sys_name;
	content += "\"";
	content += F(",\"Wmode\":\"");
	if (ESP_mode == 0)
	{
		content += F("Access Point\"");
	}
	else
	{
		content += F("Wifi Client\"");
	}
	delay(0);
	yield();
  for (int i = 0; i < ESP_settings_size; i++)
  {
    if (setting[i].value[0]!='\0')
    {
		content += ", \"";
		content += setting[i].name;
		content += F("\" : \"");
		
		content += setting[i].value;
		content += "\"";
    }
  }
  content += "}];";
  delay(0);
  yield();
  server.send(200, F("application/json"), content);
  delay(0);
  yield();

}


void ESPWifiConfig::handle_cout(void)
{
  delay(0);
  yield();
  if (server.hasArg("c_in")  && (server.arg("c_in").length()>0))
  {
	input += server.arg("c_in");
	server.send(200, F("text/plain"),  "\nOK");
  }
  else
	server.send(200, F("text/plain"),  "\n.");
  //Serial.println(debug_log);
  //debug_log = "";
  
  delay(0);
  yield();
}





















//root page can be accessed only if authentification is ok
void ESPWifiConfig::handleRoot(void)
{
  //ESP_debug(F("ESP:handleRoot\n"));
  delay(0);
  yield();
  if (!is_authentified())
  {
    server.sendContent(F("HTTP/1.1 301 OK\r\nLocation: ./login\r\nCache-Control: no-cache\r\n\r\n"));
    return;
  }
  else
  {
    server.sendContent(F("HTTP/1.1 301 OK\r\nLocation: ./setup\r\nCache-Control: no-cache\r\n\r\n"));
    return;
  }

  delay(0);
  yield();
}

//no need authentification
void ESPWifiConfig::handleNotFound(void)
{
  delay(0);
  yield();
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
  delay(0);
  yield();
}

void ESPWifiConfig::print_login_page(void)
{
  delay(0);
  yield();
  server.send(200, F("text/html"), FPSTR(login_page_file));
  delay(0);
  yield();
}


void ESPWifiConfig::print_setup_page(void)
{
  delay(0);
  yield();
  server.send(200, F("text/html"), FPSTR(setup_page_file));
  delay(0);
  yield();
}


void ESPWifiConfig::print_css_file(void)
{
  delay(0);
  yield();
  server.send(200, F("text/css"), FPSTR(css_file));
  delay(0);
  yield();
}

void ESPWifiConfig::handle_error(void)
{
  delay(0);
  yield();
  server.send(200, F("text/html"), "var msg = \"" + error_msg + "\"");
  error_msg = "";
  delay(0);
  yield();
}






























boolean ESPWifiConfig::is_reset_pressed(int reset_button)
{
	for (int i = 0; i < 50; i++)
	{
		if (digitalRead(reset_button) == 1)
		{
			return false;
		}
		delay(1);
	}
	return true;
}

void ESPWifiConfig::ESP_reset_settings()
{
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < ESP_settings_size; i++)
  {
    uint8_t kx = 0;
    for (unsigned int k = setting[i].addr; k < (VALUE_MAX_SIZE + setting[i].addr); k++)
    {
		
		EEPROM.write(k, 0);
		//if(setting[i].value[kx]=='\0') break;
		//else setting[i].value[kx] = '\0';
		kx++;
    }
  }
  EEPROM.end();
  ESP_read_settings();
}


void ESPWifiConfig::ESP_read_settings()
{
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < ESP_settings_size; i++)
  {
	
	//Serial.print("Read: ");
	//Serial.print(setting[i].name);
	
	uint8_t first_char = EEPROM.read(setting[i].addr);
	if( ((first_char>=32) && ( first_char<=126)) || (i==WIFI_SSID) || (i==WIFI_PASS))
	{
		//Serial.print("=");
		uint8_t ix = 0;
		for (uint16_t k = setting[i].addr; k < (VALUE_MAX_SIZE + setting[i].addr); k++)
		{
		  char chr = (char)EEPROM.read(k);
		  if((chr >= 32) && (chr <= 126))
		  {
			  setting[i].value[ix] = chr;
			  //Serial.print(setting[i].value[ix]);
		  }
		  else if (chr == '\0') { setting[i].value[ix] = chr; break; }
		  else setting[i].value[ix] = '\0';
		  ix++;
		}
	}
	//else Serial.print(F(" N/A"));
	//Serial.println(";");
  }
  EEPROM.end();
}



void ESPWifiConfig::ESP_save_settings()
{
  EEPROM.begin(EEPROM_SIZE);
  for (uint8_t i = 0; i < ESP_settings_size; i++)
  {
    
    uint8_t kx = 0;
	//Serial.print("Save: ");
	//Serial.print(setting[i].name);

	//Serial.print("=");
	int ee = setting[i].addr;
	
    for (uint8_t k = 0; k < VALUE_MAX_SIZE; k++)
    {
		//Serial.print(setting[i].value[k]);
		EEPROM.write(ee, setting[i].value[k]);
		if(setting[i].value[k]=='\0') break;
		ee++;
    }
	//Serial.println(";");
	if (!EEPROM.commit()) ESP_debug(F("ERROR_MEM"));
  }
  
  EEPROM.end();
	
}

void ESPWifiConfig::print_settings()
{

  uint8_t ESP_settings_count = 0;
  for (uint8_t i = 0; i < ESP_settings_size; i++)
  {
    Serial.printf(setting[i].name); Serial.printf("="); Serial.printf(setting[i].value); Serial.printf(";\n");
    ESP_settings_count++;
  }
  Serial.printf("EEPROM used:%d-%d\n", setting[0].addr, setting[ESP_settings_count-1].addr + setting[ESP_settings_count-1].max_size);
}



unsigned long ESPWifiConfig::getmacID()
{
	unsigned long mac_addr_short = 0;
	String mac_real = WiFi.macAddress();
	unsigned long power = 1;
	for(int i = 3; i < mac_real.length(); i++)
	{
		if((mac_real[i]!=':') && (mac_real[i]!=' '))
		{
			mac_addr_short += (((unsigned char)mac_real[i])*(power));
		}
		power = power * 10;
	}
	return mac_addr_short%1000000000;
}

void ESPWifiConfig::ESP_debug(String line)
{
  if(show_debug)	Serial.println(line);
  
  debug_log += line;
  debug_log += "\n";
  if (debug_log.length() > 50)
  {
    debug_log = debug_log.substring(10);
  }
}

