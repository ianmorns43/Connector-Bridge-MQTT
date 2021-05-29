# Connector-Bridge-MQTT
Hubitat automation for motorised blinds connected to a Dooya Connector (DD7002B) Hub. Also sold as a Decorquip DreamHub or Bloc Blinds Bloc.IQ Smart Hub.

As Hubitat cannot consume the Multicast UDP messages sent from the Connector Hub, this project uses a ESP8266 MCU to act as a Bridge between the Connector Hub and Hubitat. The ESP8266 receives UDP messages from the Hub and publishes to the MQTT Broker, and also subscribes to commands sent to the Broker and forwards them as UDP messages to the Hub

## Hardware requirements
MQTT Broker such as [Eclipse Mosquitto&trade;](https://mosquitto.org/) which can be insalled on a Raspberry PI

A ESP8266 Micro Controller Unit. This project is configured to use a D1 Mini Pro development board version.

A [Hubitat Elevation&trade;](https://hubitat.com/) home automation hub

A Dooya Connector Hub, Decorquip DreamHub or Bloc IQ Smart hub controlling motorised blinds.

## Software
Something to build and flash sowftware onto a D1 Mini ESP8266 MCU. For example [Visual Studio Code with PlatformIO extension](https://platformio.org/install/ide?install=vscode)

## Setup
Setup your Connector Hub, Hubitat Hub and MQTT Broker

Copy /include/mqttParams.h.example to /include/mqttParams.h
update the contents of /include/mqttParams.h with your MQTT Broker details

Build the project and upload to your D1 Mini. Connect to the ESP8266's Wifi endpoint, which will be called DD7002B. Open a browser and enter your Wifi details to logon to your local network. The ESP8266 will login to your MQTT Broker and begin searching for your Connector Hub using UDP multicast messages.

Upload the groovy files from the .\apps and .\drivers folders to Hubitat. On Hubitat select Add a User App and select "Connector Hub Control".

### App Setup
On the App Set up, select your MQTT Broker device <TODO Add Groovy code for Broker Device>

Select 'Connector Hub Setup'
* Give your Hub a name
* Leave the Hub MQTT Topic as its default value
* Enter you Hub's secret key. Find this by opening the Connector App on a phone or tablet, go to the About page and tap the Connector logo 5 times in a row.
* Click Next
* Click Done

The Connector Hub Control App will contain a Connector Hub device which will contact the ESP8266 and get the list of avaialable blind motors which can be added to Hubitat from the installed Connector Hub Control App.




