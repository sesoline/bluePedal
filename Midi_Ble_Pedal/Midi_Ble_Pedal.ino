/*
    MIDI BLE PEDAL 

    I took some projects in order to create this one. 

    The idea is create an midi guitar pedal for several iOS apps
    
    
    This is a BLE_MIDI Example by neilbags 
    https://github.com/neilbags/arduino-esp32-BLE-MIDI
    
    Based on BLE_notify example by Evandro Copercini.a
    Creates a BLE MIDI service and characteristic.
    Once a client subscibes, send a MIDI message every 2 seconds
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700" // mac-> 03b80e5a-ede8-4b33-a751-6ce34ec4c700
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// Inputs GPIO D13, D12, D14, D27,  
// Outputs GPIO 15, 2,4,5---- D18 (status) 

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

int currentValue[5], lastValue[5], i;
int midiCC[]= {16,17,18,19,20}; // assign CC midi to 16,17,18,19
int pedalInputs[]={13,12,14,27,25};
int pedalOutputs[]={15,2,4,5};


uint8_t midiPacket[] = {
   0x80,  // header
   0x80,  // timestamp, not implemented 
   0x00,  // status
   0x3c,  // 0x3c == 60 == middle c
   0x00   // velocity
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);

  //*************   ESP32 SETTINGS      *************
  
  pinMode(13, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP); // For tap tempo
  pinMode(15, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  pinMode(18, OUTPUT);  // status

  // ************   BLUETOOTH SETTINGS  *************
  
  BLEDevice::init("Orbit Pedal v1");
    
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {  
      // Midi packet
      midiPacket[2] = 0xb1; // cc control, chan:1
      
      // Scan for a change
      for(i=0;i<=3;i++){
        digitalWrite(pedalOutputs[i],digitalRead(pedalInputs[i]));  // on/off the switch led
        currentValue[i]=digitalRead(pedalInputs[i]);
        if(currentValue[i]!=lastValue[i]){
          if(currentValue[i]==HIGH){                    
          midiPacket[4] = 127;
          }
          else{
          midiPacket[4] = 0;
          }
          // Sending midi packet        
          midiPacket[3] = midiCC[i]; // cc channel
          pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
          pCharacteristic->notify();
          Serial.println("CC #: " + String(midiPacket[3]) + " was press/release it " ); 
          }
        // after scan, save current values
        lastValue[i]=currentValue[i];
        }
        // Tap tempo 
        
        if(digitalRead(pedalInputs[4])==LOW){       
          midiPacket[3] = midiCC[4]; // cc channel
          midiPacket[4] = 127;
          pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
          pCharacteristic->notify();
          delay(100);
        }/* else {
          midiPacket[3] = midiCC[4]; // cc channel
          midiPacket[4] = 0;
          pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
          pCharacteristic->notify();
        }*/
        
  }  
   
   else{
    // Blink led while waiting for connection
    digitalWrite(18,HIGH);
    delay(50);
    digitalWrite(18,LOW);
    delay(1000);
  }
}
