# irrigation
Simple time based irrigation controller using the  ESP32 or ESP8266

![ui image](https://raw.githubusercontent.com/enwi/irrigation/master/img/UI.png)

## Features
* Web UI for configuration
* NTP time based watering
* Configurable watering duration
* Configuration stored in EEPROM


## Prerequisites
* Install [Time library](https://github.com/PaulStoffregen/Time)
* Install [NTPClient](https://github.com/arduino-libraries/NTPClient.git)
* Install [ESP_EEPROM](https://github.com/jwrw/ESP_EEPROM.git
)
* Install [ESPUI](https://github.com/s00500/ESPUI.git)

## Setup
* Change the WiFi SSID and password inside the `WiFiConfig.h` file according to your local WiFi network.
* Upload the code
* Type in `irrigation.local` in any browser (of a device connected to the same network) to open up the control panel
* Change settings as wished

# Support
If you want to support my work you can store on Amazon via this link: [Amazon Referral Link](https://www.amazon.de/?tag=enwi-21). This is an affiliate link. Through a purchase via this link, I will participate in the turnover. This has no effect on the price for you.
