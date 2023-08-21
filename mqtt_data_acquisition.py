#   Technical Test Embedded System Engineer 2023
#   PT. RAVELWARE TECHNOLOGY INDONESIA
#   (c)Bayu Dwi Rizkyadha Putra
#   Ver.0.1 - 20230821

# Kebutuhan Penggunaan Library
import json
from datetime import datetime

import mysql.connector
import paho.mqtt.client as mqtt

# Deklarasi variabel global
data_status = ""
data_deviceID = ""
data_main =""
data_voltage =""
data_current =""
data_power=""
data_temp = ""
data_fan = ""

# Fungsi Callback untuk koneksi MQTT 2
def on_connect2(client2, userdata, flags, rc):
    print("Connected with result code {0}".format(str(rc)))             # Tampilkan Hasil Proses Koneksi

# Fungsi Callback untuk koneksi MQTT 1
def on_connect(client, userdata, flags, rc):                            # Tampilkan Hasil Proses Koneksi
    print("Connected with result code {0}".format(str(rc)))  

    client.subscribe("DATA/ONLINE/SENSOR/PANEL_1")                      # Subscribe data dari esp32

# Fungsi Callback saat broker menerima data dari esp32
def on_message(client, userdata, msg):  
    global data_status  
    global data_deviceID  
    global data_main 
    global data_voltage 
    global data_current 
    global data_power
    global data_temp  
    global data_fan  

    # Ambil data saat ini
    now =  datetime.now()  
    waktu = now.strftime("%Y-%m-%d %H:%M:%S")               # Menentukan format data dan waktu

    # Proses decode JSON Message dari ESP32
    decoded_message=str(msg.payload.decode("utf-8"))
    msg1=json.loads(decoded_message)

    # Hasil parsing data JSON, disimpan ke Variabel
    data_status = msg1["status"]
    data_deviceID = msg1["deviceID"]
    data_voltage = msg1["data"]["v"]
    data_current = msg1["data"]["i"]
    data_power = msg1["data"]["pa"]
    data_temp = msg1["data"]["temp"]
    data_fan = msg1["data"]["fan"]

    # Proses Simpan data ke MySQL
    cursor = mydb.cursor()
    sql = "INSERT INTO logging_data (status, deviceID,voltage, current, power, temperature,fan,time) VALUES (%s, %s,%s, %s,%s, %s,%s, %s)"
    val = (data_status, data_deviceID,data_voltage,data_current,data_power,data_temp,data_fan,waktu)
    cursor.execute(sql, val)
    mydb.commit()

    # Recreate data JSON, hasil parsing dari ESP32 dan menambahkan variabel waktu untuk dikirim ke gateway
    data_json_kirim = {
        "status": data_status,
        "deviceID": data_deviceID,
        "data":{
            "v":str(data_voltage),
            "i":str(data_current),
            "pa":str(data_power),
            "temp":str(data_temp),
            "fan":data_fan,
            "time":waktu
        }
    }
    json_object = json.dumps(data_json_kirim, indent=4)

    # Kebutuhan untuk debugging, ditampilkan di layar terminal
    print("data : "+data_status+","+data_deviceID+","+str(data_voltage)+","+str(data_current)+","+str(data_power)+","+str(data_temp)+","+data_fan+","+waktu)
    
    # Publish data ke gateway
    client2.publish("DATA/ONLINE/SENSOR/PANEL_1",str(json_object))

# Inisialisasi MySQL
mydb = mysql.connector.connect(
  host="localhost",
  user="root",
  password="",
  database="power_meter_db"
)

# Cek konektivitas database
if mydb.is_connected():
    print("Berhasil terhubung ke database")

# Inisialisasi data ke broker MQTT
broker_address= "tailor.cloudmqtt.com"
port = 16144
user = "uinfmnbq"
password = "b2vg8UYSyRii"
client2 = mqtt.Client("bayu-dwi-rizkyadha-putra")            
client2.username_pw_set(user, password=password)    
client2.on_connect= on_connect2                      

# Inisialisasi ke broker lokal
client = mqtt.Client("bayu-dwi-rizkyadha-putra")  
client.on_connect = on_connect 
client.on_message = on_message  

client.connect('127.0.0.1', 1883)
client2.connect(broker_address, port=port)         

# Start service MQTT, baik lokal maupun ke gateway
client.loop_forever()        
client2.loop_forever()  

