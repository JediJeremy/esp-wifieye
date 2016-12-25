# esp-wifieye
ESP8266 Wifi Eye

This is application framework for the ESP8266 which provides a web-based admin interface that can be used to remotely do the
"bootstrap" configuration (hostname, network, passwords, ota setup) of a fresh firmware install.

The web server can serve multiple files concurrently, and also provides a websockets layer for real-time communication from the 
microcontroller to (multiple) client browsers.

Most ESP8266 projects have the access point information hard-coded into the firmware, which means to change to a new AP you need
to recompile... not very friendly. They also tend to be "promiscuous" in their enabling of the OTA (over the air) reprogramming. 
Which is really nice when you're developing (there's no passwords or anything!) but you need to be able to switch it off when 
deploying into hostile territory.

This framework reads files from SPIFFS arranged in a familiar unix-like structure:

/etc/hostname  - single string entry of the machine hostname
/etc/ap        - JSON file containing the station name and password for the Access Point served by the microcontroller
/etc/htaccess  - JSON file containing the username and password for http authentication (used by the file manager)

/wifi/ap/<name> - JSON configuration files for named access points, their passwords and other metadata.
/wifi/mac/<address> - JSON configuration files for identified access points, their passwords and other metadata.


built on top of the AsyncTCP Library

