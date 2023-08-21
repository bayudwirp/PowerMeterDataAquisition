# PowerMeterDataAquisition
Source code ini, merupakan program untuk melakukan akuisisi data power meter Selec EM4M dan termperature sensor Autonics TX4S series. Menggunakan mikrokontroler ESP32 dengan menggunakan protokol modbus untuk melakukan pembacaan sensor. Data sensor akan diambil oleh broker lokal (gateway) untuk disimpan di server lokal dan dikirimkan ke server

Ada dua file yang dishare pada project ini:

File .ino
Merupakan file source code yang akan dijalankan pada mikrokontroler ESP32. Pada program ini, akan melakukan pembacaan perdetik pada modul power meter dan temperature sensor. Hasil pembacaan tersebut, akan dikirimkan ke Broker MQTT.
Jika alat tidak terpasang, dapat menggunakan mode dummy pada program ini, sehingga memudahkan pengembang untuk melakukan tracing permasalahan proses pengiriman data MQTT

File .py
Merupakan file source code yang akan dijalankan pada broker lokal (pada percobaan ini, pengembang menggunakan laptop windows untuk melakukan percobaan), dimana nantinya data dari ESP32, akan diproses dan ditambahkan data timestamp sebelum dikirimkan ulang ke gateway
