#include "OLED_1in51/OLED_Driver.h"
#include "OLED_1in51/OLED_Driver.cpp"  //fine
#include "OLED_1in51/GUI_paint.h"
#include "OLED_1in51/DEV_Config.h"
#include "OLED_1in51/Debug.h"
#include "OLED_1in51/ImageData.h"
#include "OLED_1in51/GUI_paint.cpp"
#include "OLED_1in51/DEV_Config.cpp"
#include "OLED_1in51/ImageData.c"
#include "OLED_1in51/font12.cpp"
#include "OLED_1in51/font16.cpp"
#include "OLED_1in51/font8.cpp"

//int counter = 0;
UBYTE *BlackImage;

void oled_setup() {
  System_Init();
  Serial.print(F("OLED_Init()...\r\n"));
  OLED_1IN51_Init();
  Driver_Delay_ms(500);
  OLED_1IN51_Clear();

  //0.Create a new image cache
  //UBYTE *BlackImage;
  UWORD Imagesize = ((OLED_1IN51_WIDTH % 8 == 0) ? (OLED_1IN51_WIDTH / 8) : (OLED_1IN51_WIDTH / 8 + 1)) * OLED_1IN51_HEIGHT;

  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.print("Failed to apply for black memory...\r\n");
    return;
  }

  Serial.print("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, OLED_1IN51_WIDTH, OLED_1IN51_HEIGHT, 270, BLACK);

  //1.Select Image
  //Paint_SelectImage(BlackImage);
  Paint_Clear(BLACK);

  //Serial.println("Drawing:page 3");
  Paint_DrawString_EN(10, 0, "System started ", &Font16, WHITE, WHITE);
  Paint_DrawNum(10, 30, "3.1415", &Font8, 4, WHITE, WHITE);
  OLED_1IN51_Display(BlackImage);
  Paint_Clear(BLACK);
}

void draw_step(const char *step_tb_displayed) {
  Serial.println("Started drawing...");
  //Paint_DrawString_EN(10, 0, "Current Step: ", &Font16, WHITE, WHITE);
  //char charArr[5] ;
  //String nStr = String(counter);
  //nStr.toCharArray(charArr, sizeof(charArr));
  //Paint_DrawNum(10, 30, charArr, &Font16, 0, WHITE, WHITE);
  //Serial.print("Tushar's  counter: ");
  //Serial.println(counter++);
  OLED_1IN51_Display(BlackImage);
  Paint_Clear(BLACK);
  Paint_DrawString_EN(0, 3, step_tb_displayed, &Font12, WHITE, WHITE);
  //delay(750);
  //OLED_1IN51_Display(BlackImage);
  //Paint_Clear(BLACK);
  //delay(750);
}
