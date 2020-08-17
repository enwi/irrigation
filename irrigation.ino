#ifdef ESP32
#include <ESPmDNS.h>
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPUI.h>
#include <ESP_EEPROM.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#include "WiFiConfig.h" // ssid and password, not to be shown on stream :D

// Settings stored in eeprom
struct Settings
{
    uint8_t irrigationHour;
    uint8_t irrigationMinute;
    uint8_t irrigationDuration;
    bool irrigationEnabled;
} settings;
// Have we already irrigated today
bool irrigated = false;
// Water pump
const uint8_t pump = D5;
// Did this irrigation controller start for the first time aka does not have a config
bool firstStart = false;

// Our hostname
const char* hostname = "irrigation";

// The last minute we updated
uint8_t lastMinute = 0;

// GUI
int irrigationHourNI, irrigationMinuteNI, irrigationDurationNI, enableButton, resetButton;

// Time update
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

time_t getNTPTime()
{
    return timeClient.getEpochTime();
}

bool forceTimeSync()
{
    Serial.print("Forcing time sync! ");
    timeClient.forceUpdate();
    setSyncInterval(0);
    time_t time = now();
    setSyncInterval(300);
    const bool success = year(time) > 2019;
    Serial.println(success ? "Successful" : "Failed");
    return success;
}

/**
 * @brief Connect to an access point with the given ssid and password
 *
 * @param ssid The ssid of the AP
 * @param password The password of the AP
 * @return true If the connection was successfull
 * @return false If the connection failed
 */
bool connectToAP(String ssid, String password)
{
    if (WiFi.isConnected())
    {
        WiFi.disconnect();
    }
    else
    {
        WiFi.softAPdisconnect();
    }
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
          would try to act as both a client and an access-point and could cause
          network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);
    // try to connect to existing network
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait until we connected or failed to connect
    uint8_t state = WiFi.status();
    while (true)
    {
        if (state == WL_CONNECTED || state == WL_CONNECT_FAILED || state == WL_NO_SSID_AVAIL)
        {
            break;
        }
        delay(500);
        state = WiFi.status();
    }
    const bool connected = state == WL_CONNECTED;
    if (connected)
    {
        WiFi.setAutoReconnect(true);
    }
    return connected;
}

void beginEEPROM()
{
    // The begin() call will find the data previously saved in EEPROM if the same size
    // as was previously committed. If the size is different then the EEEPROM data is cleared.
    // Note that this is not made permanent until you call commit();
    EEPROM.begin(sizeof(Settings));

    firstStart = EEPROM.percentUsed() < 0;

    if (!firstStart)
    {
        loadSettings();
    }

    // EEPROM.commitReset(); // "Factory defaults"
}

/**
 * @brief Load the settings
 *
 */
void loadSettings()
{
    EEPROM.get(0, settings);
}

/**
 * @brief Store the settings
 *
 * @return true Settings have been stored
 * @return false Settings could not be stored
 */
bool storeSettings()
{
    EEPROM.put(0, settings);
    return EEPROM.commit();
}

void enabledPump()
{
    Serial.println("Enabling pump");
    digitalWrite(D4, LOW);
    digitalWrite(pump, HIGH);
}

void disabledPump()
{
    Serial.println("Disabling pump");
    digitalWrite(D4, HIGH);
    digitalWrite(pump, LOW);
}

void updateIrrigationHour(Control* sender, int value)
{
    Serial.println("Updating IrrigationHour to " + sender->value);
    settings.irrigationHour = sender->value.toInt();
    storeSettings();
}

void updateIrrigationMinute(Control* sender, int value)
{
    Serial.println("Updating IrrigationMinute to " + sender->value);
    settings.irrigationMinute = sender->value.toInt();
    storeSettings();
}

void updateIrrigationDuration(Control* sender, int value)
{
    Serial.println("Updating IrrigationDuration to " + sender->value);
    settings.irrigationDuration = sender->value.toInt();
    storeSettings();
}

void updateEnableButton(Control* sender, int value)
{
    Serial.println("Updating EnableButton to " + String(value));
    settings.irrigationEnabled = sender->value.toInt();
    storeSettings();
}

void updateResetButton(Control* sender, int value)
{
    if (value == B_DOWN)
    {
        Serial.println("Updating ResetButton to " + String(value));
        irrigated = false;
    }
}

void updateTestButton(Control* sender, int value)
{
    if (value == B_DOWN)
    {
        enabledPump();
    }
    else
    {
        disabledPump();
    }
}

void setupGUI()
{
    // TODO load config from eeprom
    irrigationHourNI = ESPUI.number(
        "Irrigation hour (UTC)", updateIrrigationHour, ControlColor::Peterriver, settings.irrigationHour, 0, 24);
    irrigationMinuteNI = ESPUI.number(
        "Irrigation minute", updateIrrigationMinute, ControlColor::Peterriver, settings.irrigationMinute, 0, 60);
    irrigationDurationNI = ESPUI.number(
        "Irrigation duration", updateIrrigationDuration, ControlColor::Peterriver, settings.irrigationDuration, 0, 255);
    enableButton = ESPUI.switcher(
        "Irrigation enabled", updateEnableButton, ControlColor::Peterriver, settings.irrigationEnabled);
    resetButton = ESPUI.button("Reset irrigation trigger", updateResetButton, ControlColor::Alizarin, "reset");
    ESPUI.button("Test pump", updateTestButton, ControlColor::Alizarin, "test");

    ESPUI.begin("Irrigation controller");
}

void setup()
{
    WiFi.hostname(hostname);

    Serial.begin(115200);

    Serial.println("My ssid: " + String(ssid));

    // Setup pump pin
    pinMode(D4, OUTPUT);
    pinMode(pump, OUTPUT);
    disabledPump();

    // setup eeprom and check if this is the first start
    beginEEPROM();

    if (firstStart)
    {
        settings.irrigationHour = 18;
        settings.irrigationMinute = 0;
        settings.irrigationDuration = 10;
        settings.irrigationEnabled = false;
        storeSettings();
    }

    // Setup wifi
    bool connected = false;
    const String ssidString(ssid);
    const String ssidPassword(password);
    while (!connected)
    {
        connected = connectToAP(ssidString, ssidPassword);
    }

    // Setup DNS so we don't have to find and type the ip address
    MDNS.begin(hostname);

    setupGUI();

    // Setup time client and force time sync
    timeClient.begin();
    setSyncProvider(getNTPTime);
    bool synced = false;
    while (!synced)
    {
        synced = forceTimeSync();
        delay(500);
    }
}

void loop()
{
    const time_t currentTime = now();
    const uint8_t currentMinute = minute(currentTime);

    if (settings.irrigationEnabled && currentMinute != lastMinute)
    {
        lastMinute = currentMinute;
        const uint8_t currentHour = hour(currentTime);
        Serial.println("Current time: " + String(currentHour) + ":" + String(currentMinute));

        if (currentHour == settings.irrigationHour && currentMinute == settings.irrigationMinute && irrigated == false)
        {
            irrigated = true;

            enabledPump();
            delay(settings.irrigationDuration * 1000);
            disabledPump();
        }

        if (currentHour == 0)
        {
            irrigated = false;
        }
    }

    // put your main code here, to run repeatedly:
    timeClient.update(); // Handle NTP update
    // handle dns
    MDNS.update();
    // handle wifi or whatever the esp is doing
    yield();
}
