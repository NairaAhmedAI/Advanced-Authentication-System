
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// REMOVED EEPROM.h to save memory - users will need manual ID assignment

SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define LCD_ADDRESS 0x27
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

#define BUZZER_PIN 8
#define GREEN_LED 11
#define RED_LED 12

// Use PROGMEM for admin UID (stored in flash, not RAM)
const char adminUID[] PROGMEM = "3A9CC801";

// Variables (reduced memory usage)
bool adminMode = false;
char currentUID[9] = "";  // Reduced size: 8 chars + null terminator
uint8_t currentFingerID = 0;
unsigned long adminTimeout = 0;
const uint16_t ADMIN_TIMEOUT = 30000;

enum SystemMode {
  MODE_SCAN_RFID,
  MODE_MAIN,
  MODE_ENROLL,
  MODE_VERIFY,
  MODE_DELETE
};
SystemMode currentMode = MODE_SCAN_RFID;

void setup() {
  Serial.begin(9600);
  finger.begin(57600);
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  showMessage(F("Fingerprint Sys"), F("Initializing..."));
  
  if (finger.verifyPassword()) {
    showMessage(F("System Ready"), F("Scan RFID Card"));
    beepSuccess();
    delay(1000);
    currentMode = MODE_SCAN_RFID;
  } else {
    showMessage(F("Sensor Error!"), F("Check Connection"));
    beepError();
    while(1);
  }
}

void loop() {
  if (adminMode && (millis() - adminTimeout > ADMIN_TIMEOUT)) {
    adminMode = false;
    showMessage(F("Admin Timeout"), F("Scan RFID Again"));
    beepError();
    delay(2000);
    currentMode = MODE_SCAN_RFID;
  }
  
  switch(currentMode) {
    case MODE_SCAN_RFID:
      handleRFIDScan();
      break;
    case MODE_MAIN:
      handleMainMenu();
      break;
    case MODE_ENROLL:
      handleEnroll();
      break;
    case MODE_VERIFY:
      handleVerify();
      break;
    case MODE_DELETE:
      handleDelete();
      break;
  }
}

void handleRFIDScan() {
  static bool messageShown = false;
  
  if (!messageShown) {
    lcd.clear();
    lcd.print(F("Access Control"));
    lcd.setCursor(0, 1);
    lcd.print(F("Scan RFID Card"));
    messageShown = true;
  }
  
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Read UID efficiently
    currentUID[0] = '\0';
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      char hex[3];
      sprintf(hex, "%02X", mfrc522.uid.uidByte[i]);
      strncat(currentUID, hex, sizeof(currentUID)-strlen(currentUID)-1);
    }
    
    showMessage(F("Card Detected"), F("Processing..."));
    
    // Check if admin
    char adminUIDRam[9];
    strcpy_P(adminUIDRam, adminUID);
    
    if (strcmp(currentUID, adminUIDRam) == 0) {
      adminMode = true;
      adminTimeout = millis();
      showMessage(F("Admin Access"), F("Granted!"));
      Serial.println(F("Admin: Use Serial for ID input"));
      beepSuccess();
      delay(1000);
      showMainMenu();
      currentMode = MODE_MAIN;
    } else {
      // Non-admin users: verify only
      showMessage(F("User Mode"), F("Place finger..."));
      currentMode = MODE_VERIFY;
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    messageShown = false;
  }
}

void showMainMenu() {
  lcd.clear();
  lcd.print(F("[ADMIN] Menu:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1:Enr 2:Ver 3:Del"));
  Serial.println(F("\n=== ADMIN MENU ==="));
  Serial.println(F("1: Enroll fingerprint (Enter ID 1-127)"));
  Serial.println(F("2: Verify fingerprint"));
  Serial.println(F("3: Delete fingerprint (Enter ID 1-127)"));
  Serial.println(F("0: Exit admin"));
}

void handleMainMenu() {
  if (Serial.available()) {
    char key = Serial.read();
    
    switch(key) {
      case '1':
        Serial.println(F("Enter fingerprint ID (1-127):"));
        currentMode = MODE_ENROLL;
        break;
      case '2':
        showMessage(F("Verify Mode"), F("Place finger..."));
        currentMode = MODE_VERIFY;
        break;
      case '3':
        Serial.println(F("Enter ID to delete (1-127):"));
        currentMode = MODE_DELETE;
        break;
      case '0':
        adminMode = false;
        showMessage(F("Admin Mode"), F("Exited"));
        beepPrompt();
        delay(1000);
        currentMode = MODE_SCAN_RFID;
        break;
    }
  }
  
  // For enroll and delete modes, get ID from Serial
  if (currentMode == MODE_ENROLL || currentMode == MODE_DELETE) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      uint8_t id = input.toInt();
      
      if (id > 0 && id <= 127) {
        if (currentMode == MODE_ENROLL) {
          currentFingerID = id;
          showMessage(F("Enroll ID:"), String(id));
          lcd.setCursor(0, 1);
          lcd.print(F("Place finger..."));
          Serial.print(F("Enrolling ID: "));
          Serial.println(id);
        } else { // DELETE mode
          if (finger.deleteModel(id) == FINGERPRINT_OK) {
            showMessage(F("Deleted!"), String(id));
            Serial.print(F("Deleted ID: "));
            Serial.println(id);
            beepSuccess();
          } else {
            showMessage(F("Delete Failed"), F("ID not found"));
            Serial.println(F("Delete failed"));
            beepError();
          }
          delay(2000);
          showMainMenu();
          currentMode = MODE_MAIN;
        }
      } else {
        showMessage(F("Invalid ID!"), F("1-127 only"));
        Serial.println(F("Invalid ID"));
        beepError();
        delay(2000);
        showMainMenu();
        currentMode = MODE_MAIN;
      }
    }
  }
}

void handleEnroll() {
  static uint8_t enrollStep = 0;
  
  switch(enrollStep) {
    case 0:
      lcd.clear();
      lcd.print(F("Step 1/2"));
      lcd.setCursor(0, 1);
      lcd.print(F("Place finger"));
      
      if (getFingerprintImage()) {
        lcd.clear();
        lcd.print(F("Step 1 OK"));
        lcd.setCursor(0, 1);
        lcd.print(F("Remove finger"));
        beepPrompt();
        delay(1000);
        enrollStep = 1;
      }
      break;
      
    case 1:
      lcd.clear();
      lcd.print(F("Step 2/2"));
      lcd.setCursor(0, 1);
      lcd.print(F("Place finger"));
      
      if (getFingerprintImage()) {
        if (createFingerprintModel(currentFingerID)) {
          showMessage(F("Enroll Success!"), String(currentFingerID));
          Serial.print(F("Success! ID: "));
          Serial.println(currentFingerID);
          beepSuccess();
        } else {
          showMessage(F("Enroll Failed"), F("Try Again"));
          Serial.println(F("Enrollment failed"));
          beepError();
        }
        
        delay(2000);
        enrollStep = 0;
        showMainMenu();
        currentMode = MODE_MAIN;
      }
      break;
  }
}

void handleVerify() {
  lcd.clear();
  lcd.print(F("Verifying..."));
  
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) return;
    showMessage(F("Scan Error"), F(""));
    beepError();
    delay(1000);
    currentMode = adminMode ? MODE_MAIN : MODE_SCAN_RFID;
    if (adminMode) showMainMenu();
    return;
  }
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    showMessage(F("Convert Error"), F(""));
    beepError();
    delay(1000);
    currentMode = adminMode ? MODE_MAIN : MODE_SCAN_RFID;
    if (adminMode) showMainMenu();
    return;
  }
  
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    showMessage(F("Verified!"), String(finger.fingerID));
    Serial.print(F("Verified ID: "));
    Serial.println(finger.fingerID);
    beepSuccess();
  } else if (p == FINGERPRINT_NOTFOUND) {
    showMessage(F("Not Found"), F("Not Registered"));
    Serial.println(F("Fingerprint not found"));
    beepError();
  } else {
    showMessage(F("Search Error"), F(""));
    Serial.println(F("Search error"));
    beepError();
  }
  
  delay(2000);
  currentMode = adminMode ? MODE_MAIN : MODE_SCAN_RFID;
  if (adminMode) showMainMenu();
}

void handleDelete() {
  // Handled in handleMainMenu()
}

bool getFingerprintImage() {
  int p = finger.getImage();
  if (p == FINGERPRINT_OK) return true;
  if (p == FINGERPRINT_NOFINGER) return false;
  showMessage(F("Scan Error"), F(""));
  beepError();
  delay(1000);
  return false;
}

bool createFingerprintModel(uint8_t id) {
  int p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return false;
  
  delay(1000);
  showMessage(F("Remove finger"), F("Place again"));
  
  while (finger.getImage() != FINGERPRINT_NOFINGER) delay(100);
  
  while (finger.getImage() != FINGERPRINT_OK) delay(100);
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return false;
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) return false;
  
  p = finger.storeModel(id);
  return (p == FINGERPRINT_OK);
}

// Helper functions
void showMessage(const __FlashStringHelper* line1, const __FlashStringHelper* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void showMessage(const __FlashStringHelper* line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void beepSuccess() {
  digitalWrite(GREEN_LED, HIGH);
  tone(BUZZER_PIN, 1500, 300);
  delay(300);
  digitalWrite(GREEN_LED, LOW);
  noTone(BUZZER_PIN);
}

void beepError() {
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER_PIN, 500, 300);
  delay(300);
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER_PIN);
}

void beepPrompt() {
  tone(BUZZER_PIN, 1000, 100);
  delay(100);
  noTone(BUZZER_PIN);
}
