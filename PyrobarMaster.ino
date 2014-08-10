#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include "PyrobarConstants.h"
#include "PyrobarFireCannon.h"
#include "PyrobarLightMaster.h"
#include "PyrobarHTTPRequestHandler.h"
#include "PyrobarUDPRequestHandler.h"
#include "PyrobarFireController.h"
#include "PyrobarPulseLightSet.h"
#include "PyrobarLightMap.h"

uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 101);
unsigned int localPortUDP = 8888;

EthernetUDP Udp;

unsigned char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

uint8_t firePins[CANNON_COUNT] = {52, 48, 44};
uint8_t ledPins[] = {2, 3, 4, 5, 6, 7};
uint8_t digitalLedPins[] = {38};
uint8_t soundLevelPin = A0;
uint8_t freqBfrPos;
uint8_t sndBfrPos;

static PyrobarLightMap lightMap = PyrobarLightMap();
static PyrobarPulseLightSet pulseLightSet = PyrobarPulseLightSet();
static PyrobarFireSequence fireSequence = PyrobarFireSequence();

static PyrobarLightMaster MasterCtrl = PyrobarLightMaster(&lightMap, &pulseLightSet, ledPins, soundLevelPin);
static PyrobarHTTPRequestHandler PBHTTPRequestHandler = PyrobarHTTPRequestHandler(&lightMap, &fireSequence);
static PyrobarUDPRequestHandler PBUDPRequestHandler = PyrobarUDPRequestHandler(&lightMap, &pulseLightSet, &fireSequence);

static PyrobarFireController FireCtrl = PyrobarFireController(CANNON_COUNT, firePins, &fireSequence);

EthernetServer server(80);

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  Udp.begin(localPortUDP);
  MasterCtrl.begin();
  FireCtrl.begin();
  pinMode(32, INPUT);

  Serial.println(Ethernet.localIP());
}

void loop() {
  if (int packetSize = Udp.parsePacket()) {
    if (DEBUG_UDP) {
      Serial.print("UDP packet received with packet size: ");
      Serial.println(packetSize);
    }
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    PBUDPRequestHandler.handleRequest(packetBuffer, packetSize);
  }
  
  if(digitalRead(32) && _DEBUG) {
    printDiagnostics();
  }

  if (!FireCtrl.play()) {
    if (EthernetClient client = server.available()) {
      if (_DEBUG) Serial.println("\nClient exists!");
      PBHTTPRequestHandler.handleRequest(client);
    } 
  }
  
  MasterCtrl.calculateBufferPositions(&freqBfrPos, &sndBfrPos);
  MasterCtrl.sendLightProgramInfo(freqBfrPos, sndBfrPos);

  delay(20);
}

void printDiagnostics() {
  Serial.println("\nFrequency buffer values (every 4th):");
  for(int ptr = 0; ptr < 256; ptr += 4) {
    for(int zone = 0; zone < TOTAL_ZONE_COUNT; zone++) {
      Serial.print("(");
      Serial.print(lightMap.read("frequency", zone, ptr, 0));
      Serial.print(", ");
      Serial.print(lightMap.read("frequency", zone, ptr, 1));
      Serial.print(", ");
      Serial.print(lightMap.read("frequency", zone, ptr, 2));
      Serial.print(")\t");
    }
    Serial.println();
  }
  Serial.println("\nSound buffer values:");
  for(int ptr = 0; ptr < 16; ptr++) {
    for(int zone = 0; zone < TOTAL_ZONE_COUNT; zone++) {
      Serial.print("(");
      Serial.print(lightMap.read("sound", zone, ptr, 0));
      Serial.print(", ");
      Serial.print(lightMap.read("sound", zone, ptr, 1));
      Serial.print(", ");
      Serial.print(lightMap.read("sound", zone, ptr, 2));
      Serial.print(")\t");
    }
    Serial.println();
  }  
  
  Serial.print("Sound senstivity: ");
  Serial.print(lightMap.soundSensitivity());
  Serial.print(", Frequency: ");
  Serial.println(lightMap.frequency() * 1000.0);
}
