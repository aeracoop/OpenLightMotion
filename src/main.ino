//#define USE_FAKE_READER

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include <PixySPI_SS.h>

#define LOG_ACTIVE
#define LOG_USE_SERIAL
#define LOG_USE_SERIAL_SPEED 9600
#include "logging.hpp"

#include "wifi_connector.hpp"
#include "block_reader.hpp"
#include "fake_reader.hpp"
#include "udp_writer.hpp"
#include "common.hpp"
#include "config_manager.hpp"
#include "config.hpp"
#include "web.hpp"

#ifndef USE_FAKE_READER
BlockReader reader;
#else
FakeReader reader;
#endif

Config config;
WebManager web(config);
WiFiConnector wifi(config, web);
UDPWriter writer;

void setup() {
    log_init();
    log("Open Light Motion sensor module");

    SPIFFS.begin();

    config.resetDefaults();
    log("Reading configuration: ", config.fromFile() ? "ok" : "failed");
    config.printTo(Serial);

    web.init();
    wifi.init([](IPAddress gateway) {
        if (config.base_ip == IPAddress()) {
            log("No destination IP configured. Sending to gateway IP:", gateway.toString());
            writer.init(gateway);
        } else {
            log("Sending to configured destination IP:", config.base_ip.toString());
            writer.init(config.base_ip);
        }

        // Send a 0,0 point just to notify that we have connected successfully,
        // then begin reading data from the source.
        writer.write(Point(0, 0));
        reader.begin();
    });

    reader.init();
    reader.setCallback([](Point point) {
        writer.write(point);
    });
}

void loop() {
    wifi.loop();
    web.loop();
    reader.loop();
}
