/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <DHT.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define BOOT_PIN        0
#define LED_PIN         16
#define DTH_SENSOR_PIN  22
#define ADC_PIN         32
#define DHTTYPE DHT11 // DHT 22  (AM2302), AM2321
DHT dht(DTH_SENSOR_PIN, DHTTYPE);


#define CTRL_SERVICE_UUID                   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CTRL_CHARACTERISTIC_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SENSOR_SERVICE_UUID                 "4fafc301-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHARACTERISTIC_UUID          "beb5483c-36e1-4688-b7f5-ea07361b26a8"

static BLECharacteristic *pLEDControlCharacteristic = nullptr;
static BLECharacteristic *pSensorCharacteristic = nullptr;
static BLEDescriptor  *pSensorDescriptor = nullptr;

static bool deviceConnected = false;
static bool enableNotify = false;

class SensorDescriptorCallbacks: public BLEDescriptorCallbacks
{
    void onWrite(BLEDescriptor *pDescriptor)
    {
        uint8_t *value = pDescriptor->getValue();
        Serial.print("SensorDescriptorCallbacks:");
        if (value) {
            enableNotify = value ? true : false;
            Serial.println(value[0]);
        }
    }
    void onRead(BLEDescriptor *pDescriptor)
    {

    }
};


class LEDControlCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        Serial.print("LEDControlCallbacks Write-->");
        std::string value = pCharacteristic->getValue();
        Serial.println(pCharacteristic->getValue().c_str());

        uint8_t *data = pCharacteristic->getData();
        if (data)
            digitalWrite(LED_PIN, data[0]);
    }
    void onRead(BLECharacteristic *pCharacteristic)
    {
        Serial.println("LEDControlCallbacks Read-->");
        uint8_t data = digitalRead(LED_PIN);
        pCharacteristic->setValue(&data, 1);
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
        enableNotify = false;
        Serial.println("Device disconnect");
    }
};


void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

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

    //Setting init value
    uint8_t data = digitalRead(LED_PIN);
    pLEDControlCharacteristic->setValue(&data, 1);

    /*Sensor Service*/
    BLEService *pSensorService = pServer->createService(SENSOR_SERVICE_UUID);

    pSensorCharacteristic = pSensorService->createCharacteristic(
                                SENSOR_CHARACTERISTIC_UUID,
                                BLECharacteristic::PROPERTY_READ   |
                                BLECharacteristic::PROPERTY_NOTIFY |
                                BLECharacteristic::PROPERTY_INDICATE
                            );


    pSensorCharacteristic->addDescriptor(new BLE2902());

    pSensorDescriptor = pSensorCharacteristic->getDescriptorByUUID("2902");

    pSensorDescriptor->setCallbacks(new SensorDescriptorCallbacks());


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

uint8_t sensorArray[12];
void loop()
{
    if (deviceConnected) {
        if (enableNotify) {
            *(float *)&(sensorArray[0]) = dht.readHumidity();
            *(float *)&(sensorArray[4]) = dht.readTemperature();
            *(int *)&(sensorArray[9]) = map(analogRead(ADC_PIN), 0, 4096, 100, 0);
            pSensorCharacteristic->setValue((uint8_t *)sensorArray, sizeof(sensorArray) / sizeof(sensorArray[0]));
            pSensorCharacteristic->notify();
        }
    }

    // esp_light_sleep_start();

    delay(1000);
}