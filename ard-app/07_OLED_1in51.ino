#include "OLED_1in51/OLED_Driver.h"
#include "OLED_1in51/OLED_Driver.cpp"  //fine
#include "OLED_1in51/GUI_paint.h"
#include "OLED_1in51/DEV_Config.h"
#include "OLED_1in51/Debug.h"
#include "OLED_1in51/ImageData.h"
#include "OLED_1in51/GUI_paint.cpp"
#include "OLED_1in51/DEV_Config.cpp"
#include "OLED_1in51/ImageData.c"
#include "OLED_1in51/font24.cpp"
#include "OLED_1in51/font16.cpp"
#include "OLED_1in51/font12.cpp"
#include "OLED_1in51/font8.cpp"


//int counter = 0;
UBYTE *BlackImage;

// isLandscape: true = 128x64 (Rotate=270), false = 64x128 (Rotate=0)
void oled_setup(bool isLandscape) {
  System_Init();
  Serial.print(F("OLED_Init()...\r\n"));
  OLED_1IN51_Init();
  Serial.print(F("Done with OLED_Init()...\r\n"));

  Driver_Delay_ms(500); 
  OLED_1IN51_Clear(); 
  Serial.print(F("OLED clear\r\n"));

  //0.Create a new image cache (uses global BlackImage)
  UWORD Imagesize = ((OLED_1IN51_WIDTH%8==0)? (OLED_1IN51_WIDTH/8): (OLED_1IN51_WIDTH/8+1)) * OLED_1IN51_HEIGHT;
  if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) { 
      Serial.print("Failed to apply for black memory...\r\n");
      return;
  }
  
  // Set rotation based on orientation: 270 for landscape (128x64), 0 for portrait (64x128)
  UWORD rotation = isLandscape ? 270 : 0;
  Serial.print("Paint_NewImage with rotation=");
  Serial.println(rotation);
  Paint_NewImage(BlackImage, OLED_1IN51_WIDTH, OLED_1IN51_HEIGHT, rotation, BLACK);
  
/*
  //1.Select Image
  Paint_SelectImage(BlackImage);
  Paint_Clear(BLACK);

  // 2.Drawing on the image   
  Serial.print("Drawing:page 1\r\n");
  Paint_DrawPoint(20, 10, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  Paint_DrawPoint(30, 10, WHITE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  Paint_DrawPoint(40, 10, WHITE, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  Paint_DrawLine(10, 10, 10, 20, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(20, 20, 20, 30, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(30, 30, 30, 40, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawLine(40, 40, 40, 50, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawCircle(60, 30, 15, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawCircle(100, 40, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);      
  Paint_DrawRectangle(50, 30, 60, 40, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(90, 30, 110, 50, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);   
  // 3.Show image on page1
  OLED_1IN51_Display(BlackImage);
  Driver_Delay_ms(2000);      
  Paint_Clear(BLACK);
  
  // Drawing on the image
  Serial.print("Drawing:page 2\r\n");     
  Paint_DrawString_EN(10, 0, "waveshare", &Font16, WHITE, WHITE);
  Paint_DrawString_EN(10, 17, "hello world", &Font8, WHITE, WHITE);
  Paint_DrawNum(10, 30, "123.456789", &Font8, 4, WHITE, WHITE);
  Paint_DrawNum(10, 43, "987654", &Font12, 5, WHITE, WHITE);
  // Show image on page2
  OLED_1IN51_Display(BlackImage);
  Driver_Delay_ms(2000);  
  Paint_Clear(BLACK);   
  
  // Drawing on the image
  Serial.print("Drawing:page 3\r\n");
  Paint_DrawString_EN(10, 0,"Abc", &Font12, WHITE, WHITE);
  Paint_DrawString_EN(0, 20, "PQR", &Font24, WHITE, WHITE);
  // Show image on page3
  OLED_1IN51_Display(BlackImage);
  Driver_Delay_ms(2000);    
  Paint_Clear(BLACK); 

  // Drawing on the image
  Serial.print("Drawing:page 4\r\n");
  OLED_1IN51_Display_Array(gImage_1in3);
  Driver_Delay_ms(2000);
  Paint_Clear(BLACK); 
*/

  OLED_1IN51_Clear();  
}

