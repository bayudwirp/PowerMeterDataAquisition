//  Technical Test Embedded System Engineer 2023
//  PT. RAVELWARE TECHNOLOGY INDONESIA
//  (c)Bayu Dwi Rizkyadha Putra
//   Ver.0.1 - 20230821

// Keperluan library yang dibutuhkan pada Aplikasi ini.
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <esp32ModbusRTU.h>
#include <algorithm>  // for std::reverse
#include <ArduinoJson.h>

//Penggunaan RTOS
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Definisi untuk penggunaan Task pada RTOS
void TaskSendPeriodicData( void *pvParameters );
void TaskSendModbusData( void *pvParameters );
void TaskReadTemp( void *pvParameters );

const int ledPin = 2;                                                                   //Atur pin LED pada ESP32
const int outputFan = 33;                                                               //Atur pin Output Fan pada ESP32
char out[128];                                                                          //Untuk kebutuhan pengumpulan data JSON

//Nilai Variabel untuk tiap parameter
float value_voltage = 0.0; 
float value_current = 0.0;
float value_power_active = 0.0;
float value_temperature = 0.0;
String status_kipas = "";

//Keperluan WiFi
const char* ssid = "piyu";
const char* password =  "namaste.";

//Keperluan MQTT
const char* mqttServer = "192.168.1.11";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

//Untuk keperluan pengiriman data melalui MQTT ke Server
WiFiClient espClient;
PubSubClient client_mqtt(espClient);

//Untuk pembacaan ModBus
esp32ModbusRTU modbus(&Serial2, 4);                     //Penggunaan Serial Modbus, di Serial2 dengan Pin Direction pada pin 4.

int counter = 0;                                        //Counter yang digunakan untuk menghitung giliran kontroler meminta data ke modbus

void setup() 
{
  Serial.begin(115200);                                 // Inisialisasi serial komputer, menggunakan baudrate 115200
  Serial2.begin(9600, SERIAL_8N1, 16, 17, true);        // Modbus connection. RX = 16, TX = 17 . Baudrate interface diasumsikan sudah disetting 9600 (baik power meter maupun temperature sensor.
  modbus_init();                                        // Memanggil fungsi inisialisasi modbus
  pinMode(ledPin, OUTPUT);                              // Inisialisasi PIN LED
  pinMode(outputFan, OUTPUT);                           // Inisialisasi Pin Fan
  mqtt_init();                                          // Inisialisasi MQTT
  

  Serial.print("IP Address: ");                         // Tampilkan IP Address ESP32
  Serial.println(WiFi.localIP());
  
  xTaskCreatePinnedToCore(
    TaskSendModbusData
    ,  "TaskSendModbusData"   
    ,  1024 
    ,  NULL
    ,  3  
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  
  xTaskCreatePinnedToCore(
    TaskSendPeriodicData
    ,  "TaskSendPeriodicData"  
    ,  2048  
    ,  NULL
    ,  2  
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskReadTemp
    ,  "TaskReadTemp"
    ,  1024  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

}

void loop()
{
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


/*
  TaskSendPeriodicData
  Task yang digunakan untuk mengirimkan data secara periodic ke MQTT.
*/
void TaskSendPeriodicData(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  struct tm timeinfo;
  
  for (;;) // A Task shall never return or exit.
  {
    client_mqtt.loop();

    StaticJsonDocument<200> root_data;
    root_data["status"] = "OK";
    root_data["deviceID"] = "bayu-dwi-rizkyadha-putra";
    
    JsonObject subroot_data = root_data.createNestedObject("data");
    //Untuk penggunaan data dummy
//    subroot_data["v"] = "221";
//    subroot_data["i"] = "2.3";
//    subroot_data["pa"] = "508.3";
//    subroot_data["temp"] = "27.4";
//    subroot_data["fan"] = "OFF";

    //Untuk penggunaan data real
    subroot_data["v"] = value_voltage;
    subroot_data["i"] = value_current;
    subroot_data["pa"] = value_power_active;
    subroot_data["temp"] = value_temperature;
    subroot_data["fan"] = status_kipas;

    serializeJson(root_data, out);
    serializeJsonPretty(root_data, Serial);                     // Cetak data JSON di serial (untuk debug)
    if (client_mqtt.connect("ESP32Client", mqttUser, mqttPassword )) 
    {
       client_mqtt.publish("DATA/ONLINE/SENSOR/PANEL_1", out);
    } 
    delay(990);
    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}


/*
  TaskReadTemp
  Task yang digunakan untuk mengecek suhu dan mentrigger fan.
*/
void TaskReadTemp(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  int counter_dummy = 25;                    //Untuk debugging nilai temperature
  for (;;)
  {
    //if(value_temperature > 29.2)        //Jika ada kenaikan suhu 2% dari suhu normal (27 derajat) = 29.2, maka nyalakan kipas dan beri status kipas ON
    if(counter_dummy > 29.2)              //Untuk percobaan menggunakan data dummyJika ada kenaikan suhu 2% dari suhu normal (27 derajat) = 29.2, maka nyalakan kipas dan beri status kipas ON
    {
      status_kipas = "ON";
      digitalWrite(outputFan, HIGH);    // Fan Menyala
    }
    else
    {
      status_kipas = "OFF";
      digitalWrite(outputFan, LOW);     // Fan Mati
    }
    counter_dummy++;
    if (counter_dummy >35)
    {
      counter_dummy = 25;
    }
    delay(990);
    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}

/*
  TaskSendModbusData
  Task yang digunakan untuk melakukan pengiriman request data modbus dan proses parsing data modbus
*/
void TaskSendModbusData(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
 
  for (;;) 
  {
    get_modbus_data();                  //Panggil fungsi ambil data modbus
    delay(990);
    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}

//Untuk inisialisai masalah koneksi dengan WiFi dan MQTT (Hariff)
void mqtt_init()
{
  WiFi.begin(ssid, password);                                           // Inisialisasi nama SSID dan Password WiFi
 
  while (WiFi.status() != WL_CONNECTED)                                 // Cek WiFi
  {
    delay(500);
    Serial.println("Menghubungkan ke WiFi..");                                 
  }
  Serial.println("Connected to the WiFi network");                      // Berikan status berhasil terkoneksi ke WiFi
  client_mqtt.setServer(mqttServer, mqttPort);
  while (!client_mqtt.connected())                                      // Proses mengkoneksikan ke Broker MQTT berikut dengan MQTT Port
  {
    Serial.println("Connecting to MQTT...");
    if (client_mqtt.connect("bayu-dwi-rizkyadha-putra", mqttUser, mqttPassword )) //Hubungkan dengan broker (lokal)
    {
      Serial.println("connected");
      digitalWrite(ledPin, HIGH);
    } 
    else 
    {
      digitalWrite(ledPin, LOW);
      Serial.print("failed with state ");
      Serial.print(client_mqtt.state());
      delay(2000);
    }
  }
}

//Untuk inisialisasi penggunaan modbus pada ESP32
void modbus_init()
{
    modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
    //Serial.printf("id 0x%02x fc 0x%02x len %u: 0x", serverAddress, fc, length);
    for (size_t i = 0; i < length; ++i) 
    {
      //Serial.printf("%02x", data[i]);   //Untuk menampilkan data mentah dari Modbus, dimana datanya nantinya berbentuk nilai Hex
    }
    if(counter == 1)                                          //Jika counter menunjukan angka 1, maka baca paket data 1
    {
      //Paket Data 1 (Voltage)
      std::reverse(data, data + 2);  
      value_voltage = *reinterpret_cast<float*>(data);       //Ambil hasil pembacaan modbus, simpan ke variabel
    }
    else if(counter == 2)                                     //Jika counter menunjukan angka 1, maka baca paket data 1
    { 
      //Paket Data 2 (Current)
      std::reverse(data, data + 2);  
      value_current = *reinterpret_cast<float*>(data);        //Ambil hasil pembacaan modbus, simpan ke variabel
    }
    else if(counter == 3)                                     //Jika counter menunjukan angka 3, maka baca paket data 3
    { 
      //Paket Data 3 (Power Active)
      std::reverse(data, data + 2);  
      value_power_active = *reinterpret_cast<float*>(data);  //Ambil hasil pembacaan modbus, simpan ke variabel
    }
    else if(counter == 4)                                      //Jika counter menunjukan angka 4, maka baca paket data 4
    {
      //Paket Data 4 (Temperature)
      std::reverse(data, data + 2);  
      value_temperature = *reinterpret_cast<float*>(data);    //Ambil hasil pembacaan modbus, simpan ke variabel
    }
  });
  modbus.onError([](esp32Modbus::Error error) 
  {
    //Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));      // Jika terjadi error (data tidak diterima / tidak bisa terbaca)
  });
  modbus.begin();
}

//Fungsi untuk membaca data modbus
void get_modbus_data()
{
    counter++;                                   // Counternya simpan di awal, karena jika diakhir, datanya bergeser
    if(counter == 1)                             // Pembacaan Voltage dengan Register Address : 0x0123 (Dec = 291)
    {
      modbus.readInputRegisters(0x01, 291, 2);  // Paket Data 1 |  serverId, start address, data length (2 karena ada 1 data, setiap data ada 2 byte)
    }
    else if(counter == 2)                        // Pembacaan Current dengan Register Address : 0x0234 (Dec = 564)
    {
      modbus.readInputRegisters(0x01, 564, 2);  //Paket Data 2 |  serverId, start address, data length (2 karena ada 1 data, setiap data ada 2 byte)
    }
    else if(counter == 3)                        // Pembacaan Power Active dengan Register Address : 0x0345 (Dec = 837)
    {
      modbus.readInputRegisters(0x01, 837, 2);  //Paket Data 3 |  serverId, start address, data length (2 karena ada 1 data, setiap data ada 2 byte)
    }
    else if(counter == 4)                        // Pembacaan Temperature dengan Register Address : 0x03E8 (Dec = 1000)
    {
      modbus.readInputRegisters(0x01, 1000, 2); //Paket Data 4 |  serverId, start address, data length (2 karena ada 1 data, setiap data ada 2 byte)
      counter = 0;
    }
}
