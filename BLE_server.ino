/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define CTRL_SERVICE_UUID                   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CTRL_CHARACTERISTIC_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SENSOR_SERVICE_UUID                 "4fafc301-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHARACTERISTIC_UUID          "beb5483c-36e1-4688-b7f5-ea07361b26a8"

static BLECharacteristic *pLEDControlCharacteristic = nullptr;
static BLECharacteristic *pSensorCharacteristic = nullptr;
static bool deviceConnected = false;
uint32_t value = 0;

class LEDControlCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pLEDControlCharacteristic)
    {
        std::string value = pLEDControlCharacteristic->getValue();
        Serial.println(pLEDControlCharacteristic->getValue().c_str());

        // if (value.length() > 0) {
        //     Serial.println("*********");
        //     Serial.print("New value: ");
        //     for (int i = 0; i < value.length(); i++)
        //         Serial.print(value[i]);

        //     Serial.println();
        //     Serial.println("*********");
        // }
    }
    void onRead(BLECharacteristic *pLEDControlCharacteristic)
    {

    }
};

class DeviceServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("Device connect");
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("Device disconnect");
    }
};


void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    BLEDevice::init("BLE Testing");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new DeviceServerCallbacks());

    /*LED Control Service*/
    BLEService *pLEDControlService = pServer->createService(CTRL_SERVICE_UUID);

    pLEDControlCharacteristic = pLEDControlService->createCharacteristic(
                          CTRL_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ |
                          BLECharacteristic::PROPERTY_WRITE
                      );

    pLEDControlCharacteristic->setCallbacks(new LEDControlCallbacks());

    /*Sensor Service*/
    BLEService *pSensorService = pServer->createService(SENSOR_SERVICE_UUID);

    pSensorCharacteristic = pSensorService->createCharacteristic(
                                SENSOR_CHARACTERISTIC_UUID,
                                BLECharacteristic::PROPERTY_READ   |
                                BLECharacteristic::PROPERTY_NOTIFY |
                                BLECharacteristic::PROPERTY_INDICATE
                            );


    pSensorCharacteristic->addDescriptor(new BLE2902());


    pSensorService->start();
    pLEDControlService->start();

    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SENSOR_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop()
{
    if (deviceConnected) {
        BLEDescriptor  *des = pSensorCharacteristic->getDescriptorByUUID("2902");
        uint8_t *data = des->getValue();
        if (data) {
            pSensorCharacteristic->setValue((uint8_t *)&value, 4);
            pSensorCharacteristic->notify();
            value++;
        }
        delay(2000);
    }