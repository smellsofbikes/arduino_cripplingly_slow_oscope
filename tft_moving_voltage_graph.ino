// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void push(int *ary, int arysize);
unsigned long makegraph(uint16_t color, int *ary, int setsize);

int debug = 0;

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));
  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());
 
  tft.reset();

  uint16_t identifier = tft.readID();

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }
  tft.begin(identifier);
}

void loop(void) {
  int setsize = 56;              // this determines how many points get plotted on the screen at once.
  int dataset[setsize];
  int data_index = 0;
  int counter;
  for (counter = 0; counter < setsize; counter++)
    dataset[counter] = 0;
  int sample_speed = 1000;       // this determines the sampling speed in milliseconds.  Probably limited to 300mS because of plot speed
  while(1)
  {
    // get ADC measurement
    int measurement = analogRead(A5);  // this should be put in hardware as a free-running fast ADC
    if(debug) { Serial.println(measurement); }
    // enter into array
    push(dataset, setsize);            // shift datapoints in array over
    dataset[0] = measurement;          // add newest data point to front of array
    
    if(debug) {
      Serial.print("data point: ");
      Serial.println(data_index);  }
    // plot
    makegraph(BLACK, dataset, setsize); // plot it
    delay(sample_speed);                // wait until next sample.
  }
 }

void push(int *ary, int setsize)  // shift all elements one place in the array
{
  int counter;
  for (counter = setsize-1; counter > -1; counter--)
    ary[counter+1] = ary[counter];
}
unsigned long makegraph(uint16_t color, int *ary, int setsize)
{
  int counter;
  int x1, y1, x2, y2, width = tft.width(), height = tft.height();
  if(debug) {Serial.print("width: "); {
  Serial.print(width);
  Serial.print(", height: ");
  Serial.println(height);  }
  int margin = 20;
 
  tft.fillScreen(WHITE);
  tft.setRotation(2);
  tft.drawLine(margin, height - margin, margin, margin, color);
  tft.drawLine(margin,margin,  width - margin, margin, color);
  int stepsize = int((height - margin)/setsize);
  if(debug) { 
    Serial.print("Stepsize: ");
    Serial.println(stepsize);  }
  int lasty = margin;
  for (counter = 0; counter < setsize-1; counter++)
  {
    y1 = lasty;
    y2 = lasty + stepsize;
    lasty = y2;
    x1 = (ary[counter] / 6)+ margin;
    x2 = (ary[counter+1] / 6)+ margin;
    tft.drawLine(x1, y1, x2, y2, BLUE);
    if(debug) {
      Serial.print(counter);
      Serial.print(" ");
      Serial.print(x1);
      Serial.print(" ");
      Serial.print(x2);
      Serial.print(" ");
      Serial.print(y1);
      Serial.print(" ");
      Serial.print(y2);
      Serial.println(" ");  }
  }
  /* 0,0 is the bottom left corner */
  tft.setCursor( 60,5);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.println("voltage");
  
  tft.setRotation(3);
  tft.setCursor(60, 220);
  tft.println("time");
  tft.setRotation(2);
}
