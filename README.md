# ghetto_BMC
A simple ESP8266 based system for remote turning on/off or restarting a computer.

The server-side code runs on an ESP8266 connected to the target computer as shown in the schematics. The client-side code is a CLI application. It accepts a single parameter:

	STATUS - Query the current status of the server
	START - Turn on the server
	SHUTDOWN - Turn off the server
	REBOOT - Restart the server

The server IP address is defined in Ghetto\_BMC\_client/main.h.
The port is defined in Ghetto\_BMC\_client/main.h and Ghetto_BMC_server/Ghetto_BMC.h.

The communication between the client and the server is AES encrypted with the following library:
https://github.com/kokke/tiny-AES-c
