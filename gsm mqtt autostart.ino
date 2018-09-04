#define TINY_GSM_MODEM_SIM800 // указываем тип модуля
#define ACC 3 // пин реле АСС
#define IGN1 4  // пин реле Зажигание 1
#define IGN2 7  // пин реле Зажигание 2
#define STARTER 8 // пин реле стартера
#define IMMO 9  // пин реле активации обходчика, ну либо можно еще что то повесить на это реле
#define TahometrPin 2 // пин тахометра (+)
#define Brake A1  // пин лягушки педали тормоза (+)
#define ONE_WIRE_BUS 12 //пин датчика температуры

#include <avr/wdt.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

SoftwareSerial SerialAT(11, 10);   //RX, TX
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

String MQTT_inputtext;
const char apn[]  = "internet";//APN сотового оператора
const char user[] = "";// логин если надо
const char pass[] = "";// пароль если надо

const char cluser[]  = "mylogin";//логин MQTT
const char clpass[]  = "mypassword";//Пароль MQTT
const char clid[]  = "mycarID";// ID MQTT
const char* broker = "mqtt.server.blablabla";// сервер MQTT

long lastReconnectAttempt = 0;
unsigned long Time1 = 0;
int interval = 1;
int stat = 0;

unsigned long Check_time = 0;
float Taho_ChekTime = 0;

int Tahometr_impulse_count;
int RPM;

float V_brake;

void setup() {
  pinMode(ACC, OUTPUT);
  pinMode(IGN1, OUTPUT);
  pinMode(IGN2, OUTPUT);
  pinMode(STARTER, OUTPUT);
  pinMode(IMMO, OUTPUT);

  pinMode(TahometrPin, INPUT_PULLUP);
  Check_time = millis();
  attachInterrupt(0, TahometrImpulse_on, RISING); //RISING или FALLING - определяется на месте, с чем стабильней
  Serial.begin(9600);
  delay(10);
  SerialAT.begin(9600);
  delay(3000);
  sensors.begin();
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, 9);

  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    wdt_enable(WDTO_1S);
    while (true);
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    wdt_enable(WDTO_1S);
    while (true);
  }
  Serial.println(" OK");

  // MQTT Broker setup
  mqtt.setServer(broker, 18830);    //порт MQTT брокера\сервера
  mqtt.setCallback(mqttCallback);
  wdt_enable(WDTO_8S);
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  if (!mqtt.connect(clid, cluser, clpass)) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  mqtt.subscribe("command30");
  return mqtt.connected();
}

void printTemperature(DeviceAddress deviceAddress)
{
  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.println(tempC);
  char tempchar1[8];
  dtostrf(tempC, 6, 0, tempchar1);
  mqtt.publish("temp30", tempchar1);
}


float BrakeRead()
{
  float DCC = analogRead(Brake);
  return (DCC);
}

float Calc_RPM() {
  
  detachInterrupt(0); // запретили считать, на всякий случай, шоб не рыпался
  Taho_ChekTime = millis() - Check_time; // время между снятиями показаний счетчика

  if (Tahometr_impulse_count > 0) { // и наличия насчитанных импульсов (на нолик делить не желательно.. будут запредельные значения в RPM)
    RPM = (1000 / Taho_ChekTime) * (Tahometr_impulse_count * 30); // (1000/Taho_ChekTime) - вычисляем множитель (1 секунду делим на наше "замерочное" время) и умножаем на (Tahometr_impulse_count*30) - тут можно было написать *60/2 (* 60 секунд, т.к. у нас есть количество вспышек катушки в секунду и / на 2 т.к. на 1 оборот коленвала 2 искры для 4ёх цилиндрового дрыгателя), но к чему эти сложности
  } else {
    RPM = 0;            // если нет импульсов за время измерений  то  RPM=0
  }
  
  Tahometr_impulse_count = 0; //сбросили счетчик.
  Check_time = millis(); // новое время
  attachInterrupt(0, TahometrImpulse_on, RISING); // разрешили считать, RISING или FALLING - определяется на месте, с чем стабильней
  return (RPM); 
}

void TahometrImpulse_on() {
  Tahometr_impulse_count++;
}

void enginestart()
{
  if (Calc_RPM() < 600)
  {
    Serial.println ("Запуск двигателя");
    digitalWrite(IMMO, HIGH), delay (500);
    digitalWrite(ACC, HIGH), delay (500);
    digitalWrite(IGN1, HIGH), delay (500);
    digitalWrite(IGN2, HIGH), delay (500);
    digitalWrite(STARTER, HIGH), delay (1000);
    digitalWrite(STARTER, LOW), delay (3000);
   if (Calc_RPM() > 600) {
      Serial.println ("Двигатель запущен");
      stat = 1;
      delay (200);
    }
  else if (Calc_RPM() < 600) {
      Serial.println ("Запуск не удался");
      digitalWrite(IGN2, LOW);
      delay (200);
      digitalWrite(IGN1, LOW);
      delay (200);
      digitalWrite(ACC, LOW);
      delay (200);
      digitalWrite(IMMO, LOW);
      delay (200);
      stat = 0;
      
}
    Serial.println ("Выход из запуска") ;
  }
  else if (Calc_RPM() > 600) {
    Serial.println ("Двигатель уже запущен");
    Serial.println ("Выход из запуска") ;
  }
}

void enginestop() {
  if (RPM > 600)
  {
    Serial.println ("Остановка двигателя") ;
    digitalWrite(IGN2, LOW), delay (200);
    digitalWrite(IGN1, LOW), delay (200);
    digitalWrite(ACC, LOW), delay (200);
    digitalWrite(IMMO, LOW), delay (200);
    stat = 0;
  }
  if (RPM < 600) {
    Serial.println ("Двигатель не запущен");
  }
}

void commandstart()
{
  if  (RPM < 600)
  {
    enginestart();
  }

 else if (RPM > 600)
  {
    enginestop();
  }

}

void reqtemp()
{
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  printTemperature(insideThermometer);
  //delay (1000);
}

void detection()
{
  Serial.println ("Детекция");
  reqtemp();
  Serial.println (Calc_RPM());
  if (RPM < 600)
  {
    mqtt.publish("status30", "0");
    mqtt.publish("V_bat30", "0");
  }

  if (RPM > 600)
  {
    int rpm1 = RPM;
    char tempchar[8];
    dtostrf(rpm1, 6, 0, tempchar);
    mqtt.publish("V_bat30", tempchar);
    mqtt.publish("status30", "1");
  }
}


void brake_check()
{
  if ((BrakeRead() > 900) && (stat >= 1))
  {
    enginestop();
  }
}

void loop()
{
  brake_check();
  wdt_reset();
  if (mqtt.connected())
  {
    mqtt.loop();
    if (millis() > Time1 + 10000) Time1 = millis(), detection();

  }
  else {
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
      else if (!modem.gprsConnect(apn, user, pass)) {
        while (true);
      }
    }
  }

}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  MQTT_inputtext = "";
  for (int i = 0; i < len; i++) {
    MQTT_inputtext += (char)payload[i];
  }
  if (String(topic) == "command30") {
    if ((MQTT_inputtext) == "start1" ) {
      Serial.println ("Команда получена");
      commandstart();
    }
  }
}
