#include "DueCANLayer.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include "Adafruit_RA8875.h"

#include <stdint.h>
#include <SPI.h>
#include <Wire.h>

// CAN Layer functions
extern byte canInit(byte cPort, long lBaudRate);
extern byte canRx(byte cPort, long* lMsgID, bool* bExtendedFormat, byte* cData, byte* cDataLen);

// definitions for display
uint8_t addr  = 0x38;
#define RA8875_INT 4
#define RA8875_CS 10 
#define RA8875_RESET 9
#define FT5206_WAKE 6
#define FT5206_INT   7   
Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;

enum {
  eNORMAL = 0,
  eTEST   = 0x04,
  eSYSTEM = 0x01
};

# define ADJUST_X -50
# define ADJUST_Y -100

# define GEAR_X 100 + ADJUST_X
# define GEAR_Y 100 + ADJUST_Y 

# define RPM_X 250 + ADJUST_X
# define RPM_Y 100 + ADJUST_Y

# define OIL_X 450 + ADJUST_X
# define OIL_Y 100 + ADJUST_Y

# define WATER_X 650 + ADJUST_X
# define WATER_Y 100 + ADJUST_Y


# define SPEED_X 370 + ADJUST_X
# define SPEED_Y 270 + ADJUST_Y

# define LAM1_X 100 + ADJUST_X
# define LAM1_Y 400 + ADJUST_Y

# define LAM2_X 300 + ADJUST_X
# define LAM2_Y 400 + ADJUST_Y

# define MAP_X 510 + ADJUST_X
# define MAP_Y 400 + ADJUST_Y

# define TPS_X 670 + ADJUST_X
# define TPS_Y 400 + ADJUST_Y


void setup()
{
  // Set the serial interface baud rate
  Serial.begin(115200);
  

  //setup for display
  Wire.begin();        // join i2c bus (address optional for master)

  pinMode     (FT5206_WAKE, INPUT);
  digitalWrite(FT5206_WAKE, HIGH );
  
  while (!tft.begin(RA8875_800x480)) 
  {
    Serial.println("RA8875 Not Found!");
    delay(100);
  }
  
  Serial.println("Found RA8875");

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);    //255: maximum brightness, 0: minimum brightness
  tft.fillScreen(RA8875_BLACK);

  tft.textMode();
  tft.textColor(RA8875_BLUE, RA8875_BLACK);
  tft.textEnlarge(2);

  tft.textSetCursor(SPEED_X,SPEED_Y);
  tft.textWrite("SPEED");

  tft.textColor(RA8875_YELLOW, RA8875_BLACK);
  tft.textSetCursor(GEAR_X,GEAR_Y);
  tft.textWrite("GEAR");
  tft.textSetCursor(RPM_X,RPM_Y);
  tft.textWrite("RPM")   ;
  tft.textSetCursor(OIL_X,OIL_Y);
  tft.textWrite("OIL");
  tft.textSetCursor(WATER_X, WATER_Y);
  tft.textWrite("WATER"); 


  tft.textSetCursor(LAM1_X,LAM1_Y);
  tft.textWrite("LAM1");
  tft.textSetCursor(LAM2_X,LAM2_Y);
  tft.textWrite("LAM2");
  tft.textSetCursor(MAP_X, MAP_Y);
  tft.textWrite("MAP");
  tft.textSetCursor(TPS_X,TPS_Y);
  tft.textWrite("TPS");

  // setup for CAN controllers
  if(canInit(0, CAN_BPS_1000K) != CAN_ERROR)
    Serial.print("CAN0: Initialized Successfully.\n\r");
  else
    Serial.print("CAN0: Initialization Failed.\n\r");
  
}// end setup


int big_to_little(byte h, byte l) {
  int out = h *16 *16;
  return out + l;
}

void loop()
{

  tft.textEnlarge(3);
     
  while(1)  // Endless loop
  {
    
    delay(1);
    // Check for received message
    long lMsgID;
    bool bExtendedFormat;
    byte cRxData[8];
    byte cDataLen;

    char str[20];
    
    if(canRx(0, &lMsgID, &bExtendedFormat, &cRxData[0], &cDataLen) != CAN_ERROR)
    {
      /* DEBUG
      for(byte cIndex = 0; cIndex < cDataLen; cIndex++)
      {
        Serial.print(cRxData[cIndex], HEX);
        Serial.print(" ");
      }// end for

      Serial.print("\n\r");
      */

      //print of display
      switch (lMsgID) {   //hoping the data is big endian

        case 0x604:    // gear, byte 7-8
        {
          memset(str, 0, 20);
          tft.textColor(RA8875_BLUE, RA8875_BLACK);
          int weel_speed = big_to_little(cRxData[2], cRxData[3]);

          sprintf(str, "%d", weel_speed);
          tft.textSetCursor(SPEED_X,SPEED_Y + 50);
          tft.textWrite(str);
          memset(str, 0, 20);

          tft.textColor(RA8875_YELLOW, RA8875_BLACK);

          int gear = big_to_little(cRxData[6], cRxData[7]);
          if (gear >= 2 and gear <= 8) {
            sprintf(str, "%d", gear-2);
          } else {
            sprintf(str, "N");
          }
          tft.textSetCursor(GEAR_X,GEAR_Y + 50);
          tft.textWrite(str);
          break;
        }
        case 0x60A:    // water temperature ect1, byte 5-6
        {
          memset(str, 0, 20);
          float ect1 = big_to_little(cRxData[4], cRxData[5])/10.0;
          sprintf(str, "%.2f", ect1);
          tft.textSetCursor(WATER_X,WATER_Y + 50);
          tft.textWrite(str);
          break;
        }

        case 0x60C:    // oil temperature, eot1, byte 1-2
        {
          memset(str, 0, 20);
          float eot1 =  big_to_little(cRxData[0], cRxData[1])/10.0;
          sprintf(str, "%.2f", eot1);
          tft.textSetCursor(OIL_X,OIL_Y + 50);
          tft.textWrite(str);
          break;
        }
        // LAM1 610, 1-2
        // LAM2 610, 3-4
        case 0x610:
        {
          memset(str, 0, 20);
          float lam = big_to_little(cRxData[0], cRxData[1])/1000.0;

          sprintf(str, "%.3f", lam);
          tft.textSetCursor(LAM1_X,LAM1_Y + 50);
          tft.textWrite(str);

          memset(str, 0, 20);
          lam =  big_to_little(cRxData[2],cRxData[3])/1000.0;
          sprintf(str, "%.3f", lam);
          tft.textSetCursor(LAM2_X,LAM2_Y + 50);
          tft.textWrite(str);
          
          memset(str, 0, 20);
          int rpm = big_to_little(cRxData[4],cRxData[5]);
          sprintf(str, "%d ", rpm);
          tft.textSetCursor(RPM_X,RPM_Y + 50);
          tft.textWrite(str);
          break;

          break;
        }
        // MAP 600, 7-8
        
        case 0x623:
        {
          memset(str, 0, 20);
          int MAP = big_to_little(cRxData[0],cRxData[1]);
         
          sprintf(str, "%d", MAP); 
          tft.textSetCursor(MAP_X,MAP_Y + 50);
          tft.textWrite(str);

          break;
        }
        // TPS 608, 1-2
        case 0x608:
        {
          memset(str, 0, 20);
          float tps = big_to_little(cRxData[0],cRxData[1])/81.92;

          tps = (tps> 100)? 100.0: tps;
          sprintf(str, "%.1f  ", tps );
          tft.textSetCursor(TPS_X,TPS_Y + 50);
          tft.textWrite(str);

          break;
        }
        
      }//end switch

      
      
    }// end if

  }// end while

}// end loop
