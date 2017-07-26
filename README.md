# esp-wifieye
ESP8266 Wifi Eye

This is the firmware for the "WiFi Eye" project, documented here: 
https://www.allaboutcircuits.com/projects/wifi-eye-part-1-intro-diagram-bom-features-iot/

It is an application framework for the ESP8266 which provides a web-based admin interface that can be used to remotely do the
"bootstrap" configuration (hostname, network, passwords, ota setup) of a fresh firmware install.

The web server can serve multiple files concurrently, and also provides a websockets layer for real-time communication from the 
microcontroller to (multiple) client browsers.

Most ESP8266 projects have the access point information hard-coded into the firmware, which means to change to a new AP you need
to recompile... not very friendly. They also tend to be "promiscuous" in their enabling of the OTA (over the air) reprogramming. 
Which is really nice when you're developing (there's no passwords or anything!) but you need to be able to switch it off when 
deploying into hostile territory.

This framework reads files from SPIFFS arranged in a familiar unix-like structure:

  * /etc/hostname  - single line entry of the machine hostname
  * /etc/ap        - JSON file containing the station name and password for the Access Point served by the microcontroller
  * /etc/htaccess  - JSON file containing the username and password for http authentication (used by the file manager)
  
  * /wifi/ap/(name) - JSON configuration files for named access points, their passwords and other metadata.
  * /wifi/mac/(address) - JSON configuration files for identified access points, their passwords and other metadata.

To pre-configure the device to connect to a particular network, just create a file in /wifi/ap with the same name as
the network, and JSON contents in the following format:

{
	"password":"shamballa",
	"priority":2,
	"auto":true,
	"ota":true,
	"fx":"pulse"
}

On startup, if no hard-coded SSID/password is provided (that's still allowed) a scan is done and the configured access point with
the highest priority and signal strength (that is marked "auto":true) is picked.

If the connection fails, other (less high-scored) access points should be tried in sequence. Currently it only performs this "fallback" during
initial and manual "search" connections. Once it succeeds in connecting to an access point, it will not automatically change to another. 
(That needs fixing with some kind of timeout.) 

In this way you can pre-configure your devices to connect to a range of networks without needing to be in range to do it.


You can also configure the network from the Web interface. 

Connect to the access point (The default SSID/password is "Agomotto"/"shamballa") from a mobile device, and then go to the web page:
http://192.168.4.1

This access point is always running, even when the ESP8266 is connected to a "host" AP of it's own. So you can configure the WiFi 
_from_ WiFi, which is a really neat trick.




built on top of the AsyncTCP Library

