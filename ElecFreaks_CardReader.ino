/*! 
    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card
    
    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card

*/

/*
        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)

// Software SPI connection:
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

void setup() {
  Serial.begin(115200);
  Serial.println(F("ElecFreaks CardReader Utility"));

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print(F("Didn't find PN53x board, quitting"));
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print(F("Chip PN5")); Serial.print((versiondata>>24) & 0xFF, HEX); 
  Serial.print(F(" - Firmware v")); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print(F(".")); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println(F("Waiting for an ISO14443A Card ..."));
}


void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    Serial.print(F("\nISO14443A card: ")); nfc.PrintHex(uid, uidLength);
    
    if (uidLength == 4) {
      // We probably have a Mifare Classic card ... 
      Serial.println(F("Type: Mifare Classic (4 byte UID)"));
      readMifareClassicTag(uid, uidLength);
    }
    
    if (uidLength == 7) {
      Serial.println(F("Type: Mifare Ultralight (7 byte UID)"));
      readMifareUltralightTag();
    }
    
  }
  
  delay(1000);
}

// Read and dump a Mifare Classic card (UID is 4 bytes)
void readMifareClassicTag(uint8_t uid[], uint8_t uidLength) {
  
  uint8_t success;
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  uint8_t keyb[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t data[16];
  char dataline[12];
  
  Serial.println(F("---------------------------------"));
  // Skipping manufacturer block
  for (uint8_t blockn = 1; blockn < 64; blockn++)
  {
    success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockn, 1, keya);
    
    if (success)
    {
      success = nfc.mifareclassic_ReadDataBlock(blockn, data);
    
      if (success)
      {
        // Data seems to have been read ... spit it out
        sprintf(dataline, "Block %02d : ", blockn);
        Serial.print(dataline);

        if (((blockn + 1) % 4) == 0) {
            Serial.print(F("[ Sector ] "));
        } else {
            Serial.print(F("[  Data  ] "));
        }
        
        nfc.PrintHexChar(data, 16);

      } else {
        Serial.println(F("[unable to read .. quitting]"));
        return;
      } 
    } else {
      Serial.println(F("[unable to authenticate .. quitting]"));
      return;
    } 
  }
  Serial.println(F("---------------------------------"));
}

// Read and dump a Mifare Ultralight tag (UID is 7 bytes)
void readMifareUltralightTag() {
  
  uint8_t success;
  uint8_t data[32];
  char dataline[12];

  Serial.println(F("------------------------------------"));
  
  // Iterate through interesting pages (4 to 15)
  // http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf
  for (int currentpage = 4; currentpage < 16; currentpage++) {
    
    // Dump the data into the 'data' array
    success = nfc.mifareultralight_ReadPage(currentpage, data);

    sprintf(dataline, "Page %02d : ", currentpage);
    Serial.print(dataline);
    
    if (success) {
      nfc.PrintHexChar(data, 4);
    } else {
      Serial.println(F("[unable to read .. quitting]"));
      return;
    }
    
  }
  Serial.println(F("------------------------------------"));
}

