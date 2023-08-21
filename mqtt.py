import json
from datetime import datetime

import mysql.connector
import paho.mqtt.client as mqtt

data_status = ""
data_deviceID = ""
data_main =""
data_voltage =""
data_current =""
data_power=""
data_temp = ""
data_fan = ""

def on_connect2(client2, userdata, flags, rc):
    print("Connected with result code {0}".format(str(rc)))  
    # Print result of connection attempt 

def on_connect(client, userdata, flags, rc):  # The callback for when  the client connects to the broker
    print("Connected with result code {0}".format(str(rc)))  
    # Print result of connection attempt 

    client.subscribe("DATA/ONLINE/SENSOR/PANEL_1")  
    # Subscribe to the topic “digitest/test1”, receive any messages    published on it


def on_message(client, userdata, msg):  # The callback for when a PUBLISH  message is received from the server. 
    global data_status  
    global data_deviceID  
    global data_main 
    global data_voltage 
    global data_current 
    global data_power
    global data_temp  
    global data_fan  

    decoded_message=str(msg.payload.decode("utf-8"))
    msg1=json.loads(decoded_message)
    now =  datetime.now()  
    waktu = now.strftime("%Y-%m-%d %H:%M:%S")
    data_status = msg1["status"]
    data_deviceID = msg1["deviceID"]
    data_voltage = msg1["data"]["v"]
    data_current = msg1["data"]["i"]
    data_power = msg1["data"]["pa"]
    data_temp = msg1["data"]["temp"]
    data_fan = msg1["data"]["fan"]

    cursor = mydb.cursor()
    sql = "INSERT INTO logging_data (status, deviceID,voltage, current, power, temperature,fan,time) VALUES (%s, %s,%s, %s,%s, %s,%s, %s)"
    val = (data_status, data_deviceID,data_voltage,data_current,data_power,data_temp,data_fan,waktu)
    cursor.execute(sql, val)
    mydb.commit()

    data_json_kirim = {
        "status": data_status,
        "deviceID": data_deviceID,
        "data":{
            "v":data_voltage,
            "i":data_current,
            "pa":data_power,
            "temp":data_temp,
            "fan":data_fan,
            "time":waktu
        }
    }

    json_object = json.dumps(data_json_kirim, indent=4)


    print("data : "+data_status+","+data_deviceID+","+str(data_voltage)+","+str(data_current)+","+str(data_power)+","+str(data_temp)+","+data_fan+","+waktu)
    client2.publish("DATA/ONLINE/SENSOR/PANEL_1",str(json_object))

mydb = mysql.connector.connect(
  host="localhost",
  user="root",
  password="",
  database="power_meter_db"
)

if mydb.is_connected():
    print("Berhasil terhubung ke database")

broker_address= "tailor.cloudmqtt.com"
port = 16144
user = "uinfmnbq"
password = "b2vg8UYSyRii"
client2 = mqtt.Client("bayu-dwi-rizkyadha-putra")            #create new instance
client2.username_pw_set(user, password=password)    #set username and password
client2.on_connect= on_connect2                      #attach function to callback

client = mqtt.Client("bayu-dwi-rizkyadha-putra")  # Create instance of client with client ID “digi_mqtt_test”
client.on_connect = on_connect  # Define callback function for successful connection
client.on_message = on_message  # Define callback function for receipt of a message

client.connect('127.0.0.1', 1883)
client2.connect(broker_address, port=port)          #connect to broker

client.loop_forever()        #start the loop
client2.loop_forever()  # Start networking daemon

