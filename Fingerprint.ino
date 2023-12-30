#include <ESP8266WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(14,12);
#else
#define mySerial Serial1
#endif
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);
char ssid[] = "Guide";
char pass[] = "0922654189";
String GAS_ID = "AKfycbxBFJhpqofBC3mi5QggBiOYdRwyznk9RVIIrHUJMAPtT4aXPy9O8uqCmTjIm8nMmeO1Dg"; //--> spreadsheet script ID
const char* host = "script.google.com";

#define MODE_NORMAL       0
#define MODE_ENROLL       1
#define MODE_DELETE       2
#define MODE_EMPTY        3

uint8_t id;

unsigned char working_mode;

unsigned char detected_id;
unsigned char sheet_id_update;

bool flag_enrolling;
bool flag_deleting;
bool flag_update_sheet;
bool flag_update_sheet_not_matched;
bool isDataSent = false;

void update_google_sheet()
{
  if ( flag_update_sheet || flag_update_sheet_not_matched )
  {
    flag_update_sheet = 0;

    // Blink 3 time
    digitalWrite(LED_BUILTIN, LOW); // LED ON
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF
    delay(100);
    digitalWrite(LED_BUILTIN, LOW); // LED ON
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF
    delay(100);
    digitalWrite(LED_BUILTIN, LOW); // LED ON
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF
    
    Serial.print("connecting to ");
    Serial.println(host);
  
    // Use WiFiClient class to create TCP connections
    WiFiClientSecure client;
    const int httpPort = 443; // 80 is for HTTP / 443 is for HTTPS!
    
    client.setInsecure(); // this is the magical line that makes everything work
    
    if (!client.connect(host, httpPort)) { //works!
      Serial.println("connection failed");
      return;
    }
       
    //----------------------------------------Processing data and sending data
    String url = "/macros/s/" + GAS_ID + "/exec?id=" ;

    if ( flag_update_sheet_not_matched )
      url += "255";
    else
      url += String(sheet_id_update);

    flag_update_sheet_not_matched = 0;
     
    Serial.print("Requesting URL: ");
    Serial.println(url);
  
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
  
    Serial.println();
    Serial.println("closing connection");      

    isDataSent = true;
  }
}

void serial_detection()
{  
  if ( Serial.available() > 0 ) 
  {
    String Str_Rx = Serial.readString();
    uint8_t num = Str_Rx.toInt(); 
    if ( working_mode == MODE_NORMAL )
    {
      if ( Str_Rx == "enroll" )
      {
        working_mode = MODE_ENROLL;
        Serial.println();
        Serial.println("Enroll Mode");
        Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
        Serial.println("Or type \"exit\" to exit to normal operation");
        Serial.println();
      }
      else if ( Str_Rx == "delete" )
      {
        working_mode = MODE_DELETE;
        Serial.println();
        Serial.println("Delete Mode");
        Serial.println("Please type in the ID # (from 1 to 127) you want to delete...");
        Serial.println("Or type \"exit\" to exit to normal operation");
        Serial.println();        
      }
      else if ( Str_Rx == "exit" )
      {
        working_mode = MODE_NORMAL;
        Serial.println();
        Serial.println("Normal mode");
        Serial.println();
      }
      else
      {
        Serial.println();
        Serial.println("Command not matched");
        Serial.println();
      }
    }
    else
    {
      if ( Str_Rx == "exit" )
      {
        working_mode = MODE_NORMAL;
        flag_enrolling = 0;
        Serial.println();
        Serial.println("Normal mode");
        Serial.println();
      }
      else
      {
        switch ( working_mode )
        {
          case MODE_ENROLL :
                              if ( num == 0 )
                              {
                                Serial.println("Value allowed");
                                Serial.println("Try again!!!");
                              }
                              else 
                              {
                                id = num;
                                flag_enrolling = 1;
                                Serial.println();
                                Serial.println("Enrolling ID #" + String(id));
                                Serial.println();
                              }
                              break;

          case MODE_DELETE :
                              if ( num == 0 )
                              {
                                Serial.println("Value allowed");
                                Serial.println("Try again!!!");
                              }
                              else 
                              {
                                id = num;
                                flag_deleting = 1;
                                Serial.println();
                                Serial.println("Deleting ID #" + String(id));
                                Serial.println();
                              }
                              break;
        }
              
      }      
    }
  }
}

/* ------------------------------------------------------------------------*/
void fn_enrolling()
{
  if ( flag_enrolling )
  {
    while (!  getFingerprintEnroll() );
    Serial.println();
    Serial.println("Waiting for a next finger....");
    Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
    Serial.println("Or type \"exit\" to exit to normal operation");
    Serial.println();
    flag_enrolling = 0;
  }
}

/* ------------------------------------------------------------------------*/
void fn_detection()
{
  if ( working_mode == MODE_NORMAL )
  {
    getFingerprintID();
      
    if ( detected_id )
    {
      Serial.println();
      Serial.println("ID: " + String(detected_id) + " is detected");
      Serial.println();
      
      sheet_id_update = detected_id;
      flag_update_sheet = 1;      
      detected_id = 0;
    }
  }
}

/* ---------------------------------------------------------------------- */
void fn_deleting()
{
  if ( flag_deleting )
  {
    deleteFingerprint(id);
    Serial.println();
    Serial.println("Waiting for a next ID....");
    Serial.println("Please type in the ID # (from 1 to 127) you want to delete...");
    Serial.println("Or type \"exit\" to exit to normal operation");
    Serial.println();    
    flag_deleting = 0;
  }
}

void setup()
{
  Serial.begin(9600);
  lcd.begin(); 
  lcd.backlight();  // เปิดไฟพร้อมจอ LCD
  lcd.setCursor(1, 0); // ตั้งตำแหน่งในจอ LCD
  lcd.print("Status : Ready");
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  // Digital output pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED OFF

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) 
  {
    Serial.println("Found fingerprint sensor!");
  } 
  else 
  {
    while ( !finger.verifyPassword() )
    {
      Serial.println("Did not find fingerprint sensor :(");
      delay(1000);
      finger.begin(57600);
      delay(1000);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }

  // WiFi connection
  Serial.println();
  Serial.print("WiFi Connecting");
  WiFi.begin(ssid, pass); //--> Connect to your WiFi router
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }  

  digitalWrite(LED_BUILTIN, HIGH); // LED OFF
  
  
  working_mode = MODE_NORMAL;
}

void loop()
{
  serial_detection();
  fn_enrolling();
  fn_detection();
  fn_deleting();
  update_google_sheet();
  delay(50);
  if (isDataSent) {
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("succeed");
        delay(2000); // แสดงข้อความเป็นเวลาสั้นๆ
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Status : Ready"); // คืนค่าสถานะเริ่มต้น
        isDataSent = false; // รีเซ็ตสถานะ
      }
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match!!!!!!!!!!!!!!!!!!");
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("Error");
      lcd.setCursor(3, 1);
      lcd.print("Scan again");
      delay(2000);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Status : Ready"); // คืนค่าสถานะเริ่มต้น
      isDataSent = false;

    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

unsigned long get_finger_ms;
unsigned long get_finger_time_buf;
unsigned long get_finger_time_dif;

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      get_finger_time_dif = millis() - get_finger_time_buf;
      if ( get_finger_time_dif >= 3000 )  // print messages ever 3 seconds
      {
        get_finger_time_buf = millis();
        Serial.println("No finger detected");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match!!!!!!!!!!!!");
    //flag_update_sheet_not_matched = 1;
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("Error");
        lcd.setCursor(3, 1);
        lcd.print("Scan again");
        delay(2000);
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Status : Ready"); // คืนค่าสถานะเริ่มต้น
        isDataSent = false;
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  detected_id = finger.fingerID;
  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) 
{
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  return p;

  
}
