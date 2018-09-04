# gsm_mqtt
GSM MQTT AUTOSTART ARDUINO

BOM:

Arduino Pro Mini (atmega328)

DC-DC 2-5А

SIM800L

5 pcs. relays SRD-12VDC-SL-C

1 pcs. ULN2003A (DIP)

6 pcs. diodies (1N4001 or other standart diode)

2 pcs. electorlitic capacitors 470 uF

2 pcs. ceramic capacitors 100 nF

2 pcs. resistors 4.7 кОм

2 pcs. resistors 10 кОм

1 pcs. temperature sensor DS18B20

2 pcs. resistors 47кОм (Analog pin А1)

2 pcs. resostors 47кОм if you tacho impulse 12V, else you need find resistors value (digital pin 2)

Scheme works 4.15-4.18V (you need configure you DC-DC convertor) this is maxumim stability voltage for SIM800L.
About arduino, don't worry about voltage, because we use RAW input.

Used Libs:

TinyGsmClient

PubSubClient

SoftwareSerial

OneWire

DallasTemperature

Also STRONGLY RECOMENDED change bootloader to Optiboot, because my code use watchdog.
If you don't use Optiboot bootloader, chinise arduino clone going to bootloop after upload my sketch.

link to Download PCB (EagleCad) https://yadi.sk/d/ZKNW_MJOvD854w

Project on Drive2 https://www.drive2.ru/l/511062076634956154/
