/*
 * The aim of this project is to create a digital info display for a Volkswagen T3 Bus.
 * 
 * Main components:
 * - STM32F103C8 (aka Blue Pill) microcontroller
 * - 1.8" 160x128 RGB SPI TFT Display
 * 
 * Wiring:
 * - LED   -> PB1
 * - SCK   -> PA5
 * - SDA   -> PA7
 * - A0    -> PB0 (A0 is also known as Data/Command or DC)
 * - RESET -> PB10
 * - CS    -> PB11 (can also be wired to ground if the display is the only SPI device)
 * - GND   -> G
 * - VCC   -> 3.3
 */

#include <Adafruit_ST7735.h> // Hardware-specific library
#include "t3-pic.h"  // imports "t3_pic" array

#define TFT_BL     PB1 // Led Backlight
#define TFT_DC     PB0 // Data/Command Pin
#define TFT_RST    PB10 // Reset Pin
#define TFT_CS     PB11 // Not needed when using a single SPI device

// width and height of a single char
#define TXT_W 6
#define TXT_H 8

const char* T3_NUM_PLATE = "S NN 315H";

struct Values {
  float oil_temp;
  float oil_press;
  float water_temp;
  float boost;
  float rpm;
  float volt;

  static Values empty() {
    return { 
      .oil_temp = NAN,
      .oil_press = NAN,
      .water_temp = NAN,
      .boost = NAN,
      .rpm = NAN,
      .volt = NAN
    };
  }
};

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
Values data = Values::empty();
uint16_t bg_cache[6] = {
  ST7735_BLACK, 
  ST7735_BLACK, 
  ST7735_BLACK, 
  ST7735_BLACK, 
  ST7735_BLACK, 
  ST7735_BLACK
};


/*
 * SETUP METHOD
 */
void setup() {
  // initialize pins
  pinMode(TFT_BL, OUTPUT);
  
  // initialize display
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);

  // show the splashscreen
  draw_splashscreen();
  
  // fade in
  fade_backlight(true, 2000);

  delay(3000);
  
  // fade out
  fade_backlight(false, 1000);

  // clear screen
  tft.fillScreen(ST7735_BLACK);
}

/*
 * LOOP METHOD
 */
void loop() {
  update_data(data);
  
  draw_cells(data);
  
  // turn on backlight
  analogWrite(TFT_BL, 255);

  delay(50);
}

/*
 * draws the splash screen, consisting of an image and two lines of text above / below it.
 */
void draw_splashscreen() {
  
  // draw t3_image
  draw_image();

  // draw text above and below
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(tft.width()/2-TXT_W*5, 5);
  tft.print("VW T3");
  tft.setCursor((tft.width()-(TXT_W*2*9))/2-2, 140);
  tft.print(T3_NUM_PLATE);
}

/*
 * draws the t3_pic image vertically centered onto the display.
 */
void draw_image() {
  int h = 111;
  int y_offset = (tft.height()-h)/2;
  for (int y=0; y<h; y++) {
    for (int x=0; x<32; x++) {
       uint16_t val = t3_pic[(y*32+x)];
       uint16_t val1 = val & 0xf;
       uint16_t val2 = (val>>4) & 0xf;
       uint16_t val3 = (val>>8) & 0xf;
       uint16_t val4 = (val>>12) & 0xf;
       tft.drawPixel(x*4, y+y_offset, gray_to_color(val4));
       tft.drawPixel(x*4+1, y+y_offset, gray_to_color(val3));
       tft.drawPixel(x*4+2, y+y_offset, gray_to_color(val2));
       tft.drawPixel(x*4+3, y+y_offset, gray_to_color(val1));
    }
  }
}

/*
 * fades the backlight in (`up` == true) or out (`up` == false). 
 * `duration` denotes the duration of the fade (in ms).
 */
void fade_backlight(bool up, float duration) {
  for (int i=0; i<=255; i++) {
    analogWrite(TFT_BL, up ? i : 255-i);
    delay(duration/255);
  }
}

/*
 * converts the lowest 4 bit of v into a 565 RGB color value
 */
uint16_t gray_to_color(uint16_t v) {
  
  v &= 0xf;
  // v = [0, 15];
  uint16_t five_bit = v*31/15; // [0, 31]
  uint16_t six_bit = v*63/15;// [0, 63]
  
  return (five_bit<<11)+(six_bit<<5)+five_bit; //  565 color (RGB)
}

/*
 * draws all cells onto the display
 */
void draw_cells(Values &data) {
  const char* UNIT_DEG_C = "\xF7" "C";
  const char* UNIT_BAR = "Bar";
  char buf[10];

  // oil temp
  {
    int ret = sprintf(buf, "%3.0f", data.oil_temp);
    uint16_t color = ST7735_BLUE;
    if (data.oil_temp >= 50) color = ST7735_BLACK;
    if (data.oil_temp >= 100) color = ST7735_ORANGE;
    if (data.oil_temp >= 115) color = ST7735_RED;
    if (ret != 3 | isnan(data.oil_temp)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell(UNIT_DEG_C, buf, "OIL", color, 0, 0);
  }

  // oil pressure
  {
    int ret = sprintf(buf, "%3.1f", data.oil_press);
    uint16_t color = ST7735_BLACK;
    if (data.oil_press >= 2.5) color = ST7735_RED;
    if (ret != 3 | isnan(data.oil_press)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell(UNIT_BAR, buf, "", color, 1, 0);
  }

  // water temp
  {
    int ret = sprintf(buf, "%3.0f", data.water_temp);
    uint16_t color = ST7735_BLUE;
    if (data.water_temp >= 50) color = ST7735_BLACK;
    if (data.water_temp >= 90) color = ST7735_ORANGE;
    if (data.water_temp >= 100) color = ST7735_RED;
    if (ret != 3 | isnan(data.water_temp)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell(UNIT_DEG_C, buf, "WATER", color, 0, 1);
  }
  
  // boost
  {
    int ret = sprintf(buf, "%3.1f", data.boost);
    uint16_t color = ST7735_BLACK;
    if (ret != 3 | isnan(data.boost)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell(UNIT_BAR, buf, "BOOST", color, 1, 1);
  }

  // rpm
  {
    int ret = sprintf(buf, "%4.0f", data.rpm);
    uint16_t color = ST7735_BLACK;
    if (ret != 4 | isnan(data.rpm)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell("", buf, "RPM", color, 0, 2);
  }
  
  // volt
  {
    int ret = sprintf(buf, "%4.1f", data.volt);
    uint16_t color = ST7735_BLACK;
    if (ret != 4 | isnan(data.volt)) {
      sprintf(buf, "ERR");
      color = ST7735_RED;
    }
    draw_cell("V", buf, "VOLT", color, 1, 2);
  }
}

/*
 * draws a cell
 */
 void draw_cell(const char* unit, const char* val, const char* label, uint16_t bg, int pos_x, int pos_y) {
  if (bg != bg_cache[pos_y*2+pos_x]) {
    draw_bg(bg, pos_x, pos_y);
    bg_cache[pos_y*2+pos_x] = bg;
  }
  
  int unit_len = strlen(unit);
  int val_len = strlen(val);
  
  int label_x = 64 * pos_x + 5;
  int label_y = 50 * pos_y + 10;

  int val_x = label_x;
  int val_y = label_y + 14;

  int unit_x = val_x + val_len * TXT_W * 2;
  int unit_y = val_y + TXT_H - 1;

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE, bg);
  
  tft.setCursor(label_x, label_y);
  tft.print(label);

  tft.setCursor(unit_x, unit_y);
  tft.print(unit);

  tft.setTextSize(2);

  tft.setCursor(val_x, val_y);
  tft.print(val);
}


/*
 * draws the background of a cell
 */
void draw_bg(uint16_t bg, int pos_x, int pos_y) {
  int w = 64;
  int x = 64 * pos_x;
  int y, h;
  if (pos_y == 0) {
    y = 0;
    h = 5 + 50;
  } else if (pos_y == 1) {
    y = 55;
    h = 50;
  } else {
    y = 105;
    h = 5 + 50;
  }
  tft.fillRect(x, y, w, h, bg);
}

/*
 * updates data values using dummy values
 */
void update_data(Values &data) {
  static uint32_t counter = 0;

  if (counter < 5) {
    counter++;
    return;
  }

  // oil temp
  {
    float range_min = -20;
    float range_max = 130;
    float range_diff = range_max - range_min;
    uint32_t tick = counter % (int) (range_diff*2);
    if (tick < range_diff) {
      data.oil_temp = range_min + tick;
    } else {
      data.oil_temp = range_max - (tick - range_diff);
    }
  }
  
  // oil pressure
  {
    float range_min = 0;
    float range_max = 5.0;
    float range_diff = (range_max - range_min)*10;
    uint32_t tick = (counter/2) % (int) (range_diff*2);
    if (tick < range_diff) {
      data.oil_press = range_min + tick / 10.0;
    } else {
      data.oil_press = range_max - (tick - range_diff) / 10.0;
    }
  }

  // water temp
  {
    float range_min = 40;
    float range_max = 60;
    float range_diff = (range_max - range_min);
    uint32_t tick = (counter/7) % (int) (range_diff*2);
    if (tick < range_diff) {
      data.water_temp = range_min + tick;
    } else {
      data.water_temp = range_max - (tick - range_diff);
    }
  }

  // boost
  {
    uint32_t tick = (counter/20) % 2;
    if (tick == 1) {
      data.boost = NAN;
    } else {
      data.boost = 0.9;
    }
  }


  // rpm
  {
    float range_min = 950;
    float range_max = 2750;
    float range_diff = (range_max - range_min) / 14.0;
    uint32_t tick = (counter) % (int) (range_diff*2);
    if (tick < range_diff) {
      data.rpm = range_min + tick*14;
    } else {
      data.rpm = range_max - (tick - range_diff) * 14;
    }
    data.rpm = ((int)data.rpm)/25*25;
  }

  // volt
  {
    uint32_t tick = (counter) % 2;
    data.volt = tick? 12.0: 12.1;
  }
  
  counter++;
}
