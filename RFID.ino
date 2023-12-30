#include <SPI.h>  // เรียกใช้งานไลบรารี่ SPI
#include <MFRC522.h>  // เรียกใช้งานไลบรารี่ MFRC522
#include <WiFiClientSecure.h>  // เรียกใช้งานไลบรารี่ WiFiClientSecure

#define SSID        "Guide"  // กำหนดชื่อ SSID ของ Wi-Fi
#define PASSWORD    "0922654189"  // กำหนดรหัสผ่านของ Wi-Fi
String GAS_ID = "AKfycbxBFJhpqofBC3mi5QggBiOYdRwyznk9RVIIrHUJMAPtT4aXPy9O8uqCmTjIm8nMmeO1Dg";  // กำหนด GAS_ID
const char* host = "script.google.com";  // กำหนดโฮสต์

#define SS_PIN  5  // กำหนดขา SS_PIN
#define RST_PIN 27  // กำหนดขา RST_PIN
#define Tag_PIN D1  // กำหนดขา Tag_PIN
MFRC522 rfid(SS_PIN, RST_PIN);  // กำหนดการเชื่อมต่อกับ MFRC522
int LED = 21;  // กำหนดขา LED

// ฟังก์ชันสำหรับส่งข้อมูล UID ไปยัง Google Sheet
void update_google_sheet(String uidString) {
    // เชื่อมต่อไปยังโฮสต์
    Serial.print("Connecting to ");
    Serial.println(host);
    WiFiClientSecure client;
    const int httpPort = 443;
    client.setInsecure();

    if (!client.connect(host, httpPort)) {
      Serial.println("Connection to host failed");
      return;
    }

    Serial.println("Connected to host");

    // สร้าง URL สำหรับส่งข้อมูลไปยัง Google Sheet
    String url = "/macros/s/" + GAS_ID + "/exec?id=" + uidString;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // กระพริบ LED 3 ครั้งเพื่อแสดงการส่งข้อมูล
    for(int i=0; i<3; i++) {
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(500);
    }

    // ส่งคำขอ HTTP ไปยัง Google Sheet
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println();
    Serial.println("Request sent");

    // รอรับข้อมูลจากเซิร์ฟเวอร์และแสดงผลใน Serial Monitor
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }
    }

    // หยุดการเชื่อมต่อ
    client.stop();
    Serial.println("\nConnection closed");
}

void setup() {
  Serial.begin(115200);  // เริ่มต้นการสื่อสารทาง Serial
  pinMode(LED, OUTPUT);  // กำหนดขา LED เป็นขาส่งสัญญาณออก
  SPI.begin();  // เริ่มต้น SPI bus
  rfid.PCD_Init();  // เริ่มต้น MFRC522
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  WiFi.begin(SSID, PASSWORD);  // เริ่มต้นการเชื่อมต่อ Wi-Fi
  Serial.printf("WiFi connecting to %s\n",  SSID);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (rfid.PICC_IsNewCardPresent()) {  // ตรวจสอบว่ามีการนำบัตร RFID/NFC ใกล้ๆ
    if (rfid.PICC_ReadCardSerial()) {  // อ่านข้อมูลบัตร RFID/NFC
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);  // ระบุประเภทของ RFID/NFC

      Serial.print("ประเภทของ RFID/NFC: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));  // แสดงประเภทของบัตร RFID/NFC

      // แสดงข้อมูล UID ในรูปแบบฐาน 16 ใน Serial Monitor
      Serial.print("UID:");
      String uidString = "";  // สร้างสตริงว่างเพื่อเก็บ UID
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");  // ใส่ 0 ข้างหน้าหากค่าน้อยกว่า 0x10
        Serial.print(rfid.uid.uidByte[i], HEX);  // แสดงค่า UID ในรูปแบบฐาน 16
        uidString += String(rfid.uid.uidByte[i], HEX);  // แปลงเป็นสตริงและเก็บใน uidString
      }
      Serial.println();

      rfid.PICC_HaltA();  // หยุดการสื่อสารกับบัตร RFID/NFC
      rfid.PCD_StopCrypto1();  // หยุดการเข้ารหัสบน PCD
      update_google_sheet(uidString);  // ส่งสตริง UID ไปยัง Google Sheet
    }
  }
}
