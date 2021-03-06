/*
 * Copyright (c) 2016 RedBear
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/******************************************************
*                      Macros
******************************************************/
/*
 * Defaultly disabled. More details: https://docs.particle.io/reference/firmware/photon/#system-thread
 */
//SYSTEM_THREAD(ENABLED);

/*
 * Defaultly disabled. If BLE setup is enabled, when the Duo is in the Listening Mode, it will de-initialize and re-initialize the BT stack.
 * Then it broadcasts as a BLE peripheral, which enables you to set up the Duo via BLE using the RedBear Duo App or customized
 * App by following the BLE setup protocol: https://github.com/redbear/Duo/blob/master/docs/listening_mode_setup_protocol.md#ble-peripheral
 *
 * NOTE: If enabled and upon/after the Duo enters/leaves the Listening Mode, the BLE functionality in your application will not work properly.
 */
//BLE_SETUP(ENABLED);

/*
 * SYSTEM_MODE:
 *     - AUTOMATIC: Automatically try to connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *     - SEMI_AUTOMATIC: Manually connect to Wi-Fi and the Particle Cloud, but automatically handle the cloud messages.
 *     - MANUAL: Manually connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *
 * SYSTEM_MODE(AUTOMATIC) does not need to be called, because it is the default state.
 * However the user can invoke this method to make the mode explicit.
 * Learn more about system modes: https://docs.particle.io/reference/firmware/photon/#system-modes .
 */
SYSTEM_THREAD(ENABLED);
//#include "photon-thermistor.h"
//#include <Adafruit_DHT.h>
#include <SparkJson.h>


#if defined(ARDUINO)
SYSTEM_MODE(SEMI_AUTOMATIC);
#endif

#define DHTPIN 0
#define DHTTYPE DHT11

/*
 * BLE peripheral preferred connection parameters:
 *     - Minimum connection interval = MIN_CONN_INTERVAL * 1.25 ms, where MIN_CONN_INTERVAL ranges from 0x0006 to 0x0C80
 *     - Maximum connection interval = MAX_CONN_INTERVAL * 1.25 ms,  where MAX_CONN_INTERVAL ranges from 0x0006 to 0x0C80
 *     - The SLAVE_LATENCY ranges from 0x0000 to 0x03E8
 *     - Connection supervision timeout = CONN_SUPERVISION_TIMEOUT * 10 ms, where CONN_SUPERVISION_TIMEOUT ranges from 0x000A to 0x0C80
 */
#define MIN_CONN_INTERVAL          0x0028 // 50ms.
#define MAX_CONN_INTERVAL          0x0190 // 500ms.
#define SLAVE_LATENCY              0x0000 // No slave latency.
#define CONN_SUPERVISION_TIMEOUT   0x03E8 // 10s.

// Learn about appearance: http://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
#define BLE_PERIPHERAL_APPEARANCE  BLE_APPEARANCE_UNKNOWN

#define BLE_DEVICE_NAME            "BLE_Peripheral"

// Length of characteristic value.
#define CHARACTERISTIC1_MAX_LEN    8
#define CHARACTERISTIC2_MAX_LEN    8
#define CHARACTERISTIC3_MAX_LEN    8

//Try to send tempen
#define CHARACTERISTIC_TEMP_MAX_LEN 16

//SYSTEM_THREAD(ENABLED);

//tempen characteristics uuid??
int x = 0;
float temp;
float hum;

bool dataRead = false;
//DHT dht(DHTPIN, DHTTYPE);

//Thermistor *t;

static uint8_t tempen_uuid[16] = { 0x71,0x3d,0x00,0x03,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };

/******************************************************
*               Variable Definitions
******************************************************/
// Primary service 128-bits UUID
static uint8_t service1_uuid[16] = { 0x71,0x3d,0x00,0x00,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
// Characteristics 128-bits UUID


// Primary service 128-bits UUID
static uint8_t service2_uuid[16] = { 0x71,0x3d,0x00,0x00,0x51,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
// Characteristics 128-bits UUID


// GAP and GATT characteristics value
static uint8_t appearance[2] = {
				LOW_BYTE(BLE_PERIPHERAL_APPEARANCE),
				HIGH_BYTE(BLE_PERIPHERAL_APPEARANCE)
};

static uint8_t change[4] = {
				0x00, 0x00, 0xFF, 0xFF
};

static uint8_t conn_param[8] = {
				LOW_BYTE(MIN_CONN_INTERVAL), HIGH_BYTE(MIN_CONN_INTERVAL),
				LOW_BYTE(MAX_CONN_INTERVAL), HIGH_BYTE(MAX_CONN_INTERVAL),
				LOW_BYTE(SLAVE_LATENCY), HIGH_BYTE(SLAVE_LATENCY),
				LOW_BYTE(CONN_SUPERVISION_TIMEOUT), HIGH_BYTE(CONN_SUPERVISION_TIMEOUT)
};

/*
 * BLE peripheral advertising parameters:
 *     - advertising_interval_min: [0x0020, 0x4000], default: 0x0800, unit: 0.625 msec
 *     - advertising_interval_max: [0x0020, 0x4000], default: 0x0800, unit: 0.625 msec
 *     - advertising_type:
 *           BLE_GAP_ADV_TYPE_ADV_IND
 *           BLE_GAP_ADV_TYPE_ADV_DIRECT_IND
 *           BLE_GAP_ADV_TYPE_ADV_SCAN_IND
 *           BLE_GAP_ADV_TYPE_ADV_NONCONN_IND
 *     - own_address_type:
 *           BLE_GAP_ADDR_TYPE_PUBLIC
 *           BLE_GAP_ADDR_TYPE_RANDOM
 *     - advertising_channel_map:
 *           BLE_GAP_ADV_CHANNEL_MAP_37
 *           BLE_GAP_ADV_CHANNEL_MAP_38
 *           BLE_GAP_ADV_CHANNEL_MAP_39
 *           BLE_GAP_ADV_CHANNEL_MAP_ALL
 *     - filter policies:
 *           BLE_GAP_ADV_FP_ANY
 *           BLE_GAP_ADV_FP_FILTER_SCANREQ
 *           BLE_GAP_ADV_FP_FILTER_CONNREQ
 *           BLE_GAP_ADV_FP_FILTER_BOTH
 *
 * Note:  If the advertising_type is set to BLE_GAP_ADV_TYPE_ADV_SCAN_IND or BLE_GAP_ADV_TYPE_ADV_NONCONN_IND,
 *        the advertising_interval_min and advertising_interval_max should not be set to less than 0x00A0.
 */
static advParams_t adv_params = {
				.adv_int_min   = 0x0030,
				.adv_int_max   = 0x0030,
				.adv_type      = BLE_GAP_ADV_TYPE_ADV_IND,
				.dir_addr_type = BLE_GAP_ADDR_TYPE_PUBLIC,
				.dir_addr      = {0,0,0,0,0,0},
				.channel_map   = BLE_GAP_ADV_CHANNEL_MAP_ALL,
				.filter_policy = BLE_GAP_ADV_FP_ANY
};

// BLE peripheral advertising data
static uint8_t adv_data[] = {
				0x02,
				BLE_GAP_AD_TYPE_FLAGS,
				BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,

				0x11,
				BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
				0x1e, 0x94, 0x8d, 0xf1, 0x48, 0x31, 0x94, 0xba, 0x75, 0x4c, 0x3e, 0x50, 0x00, 0x00, 0x3d, 0x71
};

// BLE peripheral scan respond data
static uint8_t scan_response[] = {
				0x08,
				BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
				'R', 'B',  'L', '-', 'D', 'U', 'O'
};

// Characteristic value handle


//tempen handle (type)
static uint16_t tempen_handle = 0x0300;
// Buffer of characterisitc value.


//tempen start??
//static uint8_t tempen_data[CHARACTERISTIC_TEMP_MAX_LEN] = { 0x00 };
String tempen_data = "{\"temp\":25.50}";
// Timer task.
//static btstack_timer_source_t characteristic2;

//Tempen Timer task
static btstack_timer_source_t tempen_timer;

/******************************************************
*               Function Definitions
******************************************************/
/**
 * @brief Connect handle.
 *
 * @param[in]  status   BLE_STATUS_CONNECTION_ERROR or BLE_STATUS_OK.
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void deviceConnectedCallback(BLEStatus_t status, uint16_t handle) {
				switch (status) {
				case BLE_STATUS_OK:
								Serial.println("Device connected!");
								break;
				default: break;
				}
}

/**
 * @brief Disconnect handle.
 *
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void deviceDisconnectedCallback(uint16_t handle) {
				Serial.println("Disconnected.");
				/*Serial.println(handle);
				   Serial.println(dataRead);
				   if(dataRead) {
				    System.sleep(SLEEP_MODE_DEEP, 10);
				   }*/


				//System.sleep(SLEEP_MODE_DEEP, 10);

				/*if(tempen_handle == handle) {
				    Serial.println("Its tempen_handle!!");
				    System.sleep(SLEEP_MODE_DEEP, 10);
				   }*/
				//delayMicroseconds(500);
				//System.sleep(SLEEP_MODE_DEEP, 10);
}

/**
 * @brief Callback for reading event.
 *
 * @note  If characteristic contains client characteristic configuration,then client characteristic configration handle is value_handle+1.
 *        Now can't add user_descriptor.
 *
 * @param[in]  value_handle
 * @param[in]  buffer
 * @param[in]  buffer_size    Ignore it.
 *
 * @retval  Length of current attribute value.
 */
uint16_t gattReadCallback(uint16_t value_handle, uint8_t * buffer, uint16_t buffer_size) {
				uint8_t characteristic_len = 0;
				Serial.println("Data is being read!");
				//Serial.print("Read value handler: ");
				//Serial.println(value_handle, HEX);

				if (tempen_handle == value_handle) {
								/*	ATOMIC_BLOCK() {
								     Serial.println("Reading sensors");
								     //Serial.println(tempen_data[CHARACTERISTIC_TEMP_MAX_LEN]);
								     readData();
								     dataToJSON();
								     //tempen_data = dataToJSON(temp);
								     //Serial.println("Hey");
								     /*uint8_t dataToSend[CHARACTERISTIC_TEMP_MAX_LEN];
								        Serial.println("Heyo");
								        tempen_data.getBytes(dataToSend, sizeof(tempen_data));
								        Serial.println("Heyoooo");*/
								/*

								   uint8_t dataToSend[CHARACTERISTIC_TEMP_MAX_LEN];
								   tempen_data.getBytes(dataToSend, sizeof(tempen_data));
								   //Serial.println("tempen read:");
								   memcpy(buffer, tempen_data, CHARACTERISTIC_TEMP_MAX_LEN);
								   characteristic_len = CHARACTERISTIC_TEMP_MAX_LEN; */

								uint8_t dataToSend[CHARACTERISTIC_TEMP_MAX_LEN];
								tempen_data.getBytes(dataToSend, sizeof(tempen_data));
								//Serial.println("tempen read:");
								memcpy(buffer, tempen_data, CHARACTERISTIC_TEMP_MAX_LEN);
								characteristic_len = CHARACTERISTIC_TEMP_MAX_LEN;
				}




				//dataRead = true;

				Serial.println("Data read done");
//dataRead = true;

				return characteristic_len;
}

/**
 * @brief Callback for writting event.
 *
 * @param[in]  value_handle
 * @param[in]  *buffer       The buffer pointer of writting data.
 * @param[in]  size          The length of writting data.
 *
 * @retval
 */
int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size) {
				//Serial.print("Write value handler: ");
				//Serial.println(value_handle, HEX);

				if (tempen_handle == value_handle) {
								uint8_t dataToSend[CHARACTERISTIC_TEMP_MAX_LEN];
								tempen_data.getBytes(dataToSend, sizeof(tempen_data));
								//memcpy(tempen_data, buffer, size);
								//Serial.print("tempen write value: ");
								for (uint8_t index = 0; index < CHARACTERISTIC_TEMP_MAX_LEN; index++) {
												//Serial.print(dataToSend[index], HEX);
												//Serial.print(" ");
								}
				}
				return 0;
}

/**
 * @brief Timer task for sending notify of characteristic to client.
 *
 * @param[in]  *ts
 *
 * @retval None

 */
//tempen notify
/*
   static void tempen_notify(btstack_timer_source_t *ts) {

    ATOMIC_BLOCK() {
        Serial.println("Tempen notify:");
        //Serial.println(tempen_data[CHARACTERISTIC_TEMP_MAX_LEN]);
        readData();
        dataToJSON();
        //tempen_data = dataToJSON(temp);
        Serial.println("Hey");
        uint8_t dataToSend[CHARACTERISTIC_TEMP_MAX_LEN];
        Serial.println("Heyo");
        tempen_data.getBytes(dataToSend, sizeof(tempen_data));
        Serial.println("Heyoooo");
        //Serial.println("SIZEOF DATATOSEND:");
        //Serial.println(sizeof(dataToSend));
        //Serial.println("TEMPEN MAX LENGTH:");
        //Serial.println(CHARACTERISTIC_TEMP_MAX_LEN);
        ble.sendNotify(tempen_handle, dataToSend, CHARACTERISTIC_TEMP_MAX_LEN);
        Serial.println("Heyuuuu");

        //Restart timer
        ble.setTimer(ts, 6000);
        ble.addTimer(ts);
    }
   }
 */

void readData() {
				//Serial.println("Helo");
				//temp = dht.getTempCelcius();
				//temp = t->readTempC();
				//tempen_data = "Temp: " + String(temp, 1);
				//Serial.println("temp: " + String(temp, 1));
				//Serial.println(temp, 1);
				//delayMicroseconds(1500);
				//hum = dht.getHumidity();
				//Serial.println("hum: " + String(hum));
}


void dataToJSON() {
				Serial.println("Hello");
				StaticJsonBuffer<200> jsonBuffer;

				JsonObject& root = jsonBuffer.createObject();
				root["temp"] = String(temp, 1).toFloat();
				//root["humid"] = hum;


				//root["motion"] = motion;

				char buffer[200];
				//Serial.println("buffer:");
				root.printTo(buffer, sizeof(buffer));

				String tmp;
				//Serial.println(sizeof(buffer));
				//Serial.println(buffer);
				tmp = String(buffer);
				//Serial.println("tempen data is now:");
				//Serial.println(tmp);
				tempen_data = String(buffer);
}

/**
 * @brief Setup.
 */
void setup() {
				Serial.begin(115200);
				delay(5000);
				Serial.println("BLE peripheral demo.");
				//dht.begin();
				//t = new Thermistor(A0, 10000, 4095, 10000, 25, 3950, 5, 20);
				// Open debugger, must befor init().
				//ble.debugLogger(true);
				//ble.debugError(true);
				//ble.enablePacketLogger();

				// Initialize ble_stack.
				ble.init();

				// Register BLE callback functions.
				ble.onConnectedCallback(deviceConnectedCallback);
				ble.onDisconnectedCallback(deviceDisconnectedCallback);
				ble.onDataReadCallback(gattReadCallback);
				ble.onDataWriteCallback(gattWriteCallback);

				// Add GAP service and characteristics
				ble.addService(BLE_UUID_GAP);
				ble.addCharacteristic(BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME, ATT_PROPERTY_READ|ATT_PROPERTY_WRITE, (uint8_t*)BLE_DEVICE_NAME, sizeof(BLE_DEVICE_NAME));
				ble.addCharacteristic(BLE_UUID_GAP_CHARACTERISTIC_APPEARANCE, ATT_PROPERTY_READ, appearance, sizeof(appearance));
				ble.addCharacteristic(BLE_UUID_GAP_CHARACTERISTIC_PPCP, ATT_PROPERTY_READ, conn_param, sizeof(conn_param));

				// Add GATT service and characteristics
				ble.addService(BLE_UUID_GATT);
				ble.addCharacteristic(BLE_UUID_GATT_CHARACTERISTIC_SERVICE_CHANGED, ATT_PROPERTY_INDICATE, change, sizeof(change));

				// Add primary service1.
				ble.addService(service1_uuid);
				// Add characteristic to service1, return value handle of characteristic.
				//character1_handle = ble.addCharacteristicDynamic(char1_uuid, ATT_PROPERTY_NOTIFY|ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, characteristic1_data, CHARACTERISTIC1_MAX_LEN);
				//character2_handle = ble.addCharacteristicDynamic(char2_uuid, ATT_PROPERTY_READ|ATT_PROPERTY_NOTIFY, characteristic2_data, CHARACTERISTIC2_MAX_LEN);

				//Add tempen to service 1, return value of handle characteristic???
				uint8_t dataToSend[sizeof(tempen_data)];
				tempen_data.getBytes(dataToSend, sizeof(tempen_data));
				tempen_handle = ble.addCharacteristicDynamic(tempen_uuid, ATT_PROPERTY_READ|ATT_PROPERTY_NOTIFY, dataToSend, CHARACTERISTIC_TEMP_MAX_LEN);

				// Add primary sevice2.
				ble.addService(service2_uuid);
				//character3_handle = ble.addCharacteristic(char3_uuid, ATT_PROPERTY_READ, characteristic3_data, CHARACTERISTIC3_MAX_LEN);

				// Set BLE advertising parameters
				ble.setAdvertisementParams(&adv_params);

				// Set BLE advertising and scan respond data
				ble.setAdvertisementData(sizeof(adv_data), adv_data);
				ble.setScanResponseData(sizeof(scan_response), scan_response);

				// Start advertising.
				ble.startAdvertising();
				Serial.println("BLE start advertising.");

				// set one-shot timer
				//characteristic2.process = &characteristic2_notify;
				//tempen_timer.process = &tempen_notify;
				//ble.setTimer(&characteristic2, 10000);
				//ble.addTimer(&characteristic2);
				//ble.setTimer(&tempen_timer, 6000);
				//ble.addTimer(&tempen_timer);


}



/**
 * @brief Loop.
 */
void loop() {

}
