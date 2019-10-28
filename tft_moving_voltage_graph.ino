// IMPORTANT: Adafruit_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h FOR SETUP.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <SD.h>

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

const int chipSelect = 10;  // sdcard chip select

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

uint8_t plotgraph(uint16_t color, uint8_t *ary, uint8_t setsize, uint8_t data_index, uint8_t stepwidth);
void screen_setup(uint16_t backgroundColor, uint16_t penColor, int margin1, int margin2);
void graph_erase_data(uint16_t penColor, int x1, int x2, int y1, int y2);

uint8_t debug = 0;                    // three levels: 0 is no serial, 1 is serial prints of registers, 2 is print of dataset
uint8_t Gsetsize = 56;               //sizeof dataset
uint8_t Gdataset[56];                //dataset that is accessed by ADC_ISR
uint8_t Gdataset_pointer = 0;        //pointer to oldest array element in dataset

void setup(void) {
  SREG = SREG | 0b10000000;      //enable global interrupts
  ADMUX = 0b10100101;            //vref, left-justified, ADC5
  ADCSRA = 0b11101111;           //enable adc, enable interrupt, prescalar 128
 
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));

#ifdef USE_ADAFRUIT_SHIELD_PINOUT
  Serial.println(F("Using Adafruit 2.8\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Adafruit 2.8\" TFT Breakout Board Pinout"));
#endif

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

  if (!SD.begin(chipSelect)) {  Serial.println("Card failure!"); }
  screen_setup(WHITE, BLACK, 20, 20);  // clear screen, plot white, plot black axes, label
}

void loop(void) {
  uint8_t setsize = Gsetsize;             // this determines how many samples are displayed on the screen
  uint8_t dataset[setsize];               // duplicate of global set, created to hand off to plot function
  uint8_t data_index;                     // pointer to dataset array location
  uint8_t counter, stepwidth = 0;
  char namebuffer[] = "log00.txt";
  for (uint8_t i = 0; i < 100; i++)       // neat bit of code from adafruit to make unique names rather than overwriting
  {
    namebuffer[4] = i/10 + '0';
    namebuffer[5] = i%10 + '0';
    if (! SD.exists(namebuffer)) 
    {
      break; 
    }
  }
  
  while(1)          // this is the main loop: save most recent piece of data to SDcard, copy dataset, plot it.
  {
    File dataFile = SD.open(namebuffer, FILE_WRITE);  // It may be slow to do this every time, but it guarantees closure
    dataFile.println(Gdataset[Gdataset_pointer]);
    dataFile.close();
    graph_erase_data(WHITE, 21, 21, tft.width(), tft.height());   // write a blank over just the graphed area to clear it
    cli();                                               // turn off interrupts to clone dataset
    data_index = Gdataset_pointer;
    for (counter = 0; counter < Gsetsize; counter++)
    {
      if (data_index > Gsetsize) data_index = 0;
      dataset[counter] = Gdataset[data_index];           // make new datset starting at 0
    }
    sei();
    data_index = 0;
    plotgraph(BLACK, dataset, Gsetsize, data_index, stepwidth);  // graph it
  }
}

ISR(ADC_vect) {
  uint8_t measurement = ADCH;             // only need left 8 bits
  if (Gdataset_pointer > Gsetsize-1)  // check for pointer out of array bound
    Gdataset_pointer = 0;
  Gdataset[Gdataset_pointer] = measurement;
}

void push(uint8_t *ary, uint8_t setsize)      // should not need this anymore
{
  uint8_t counter;
  for (counter = setsize-1; counter > -1; counter--)
    ary[counter+1] = ary[counter];
}
uint8_t plotgraph(uint16_t penColor, uint8_t *ary, uint8_t setsize, uint8_t data_index, uint8_t stepwidth)
{
  int counter, scratch, plottimer, index;
  int x1, y1, x2, y2, width = tft.width(), height = tft.height();
  if (debug) 
  {
    Serial.print("width: ");
    Serial.print(width);
    Serial.print(", height: ");
    Serial.println(height);
    scratch = millis();           // time how long plotting takes
  }
  uint8_t margin = 20;
  uint8_t stepsize = uint8_t((height - margin)/setsize);   // I'm still calculating this locally.  It should be computed once and passed in
  if (debug) 
  {
    Serial.print("Stepsize: ");
    Serial.println(stepsize);
  }
  
  uint8_t lasty = margin;
  tft.setRotation(2);

  for (counter = 0; counter < setsize; counter++)
  {
    y1 = lasty;
    y2 = lasty + stepsize;
    lasty = y2;
    x1 = (ary[counter] / 6)+ margin;    // the magic '6' here is a rough raw adc to tft.height() ratio.
    x2 = (ary[counter+1] / 6)+ margin;
    tft.drawLine(x1, y1, x2, y2, penColor);
    if (debug == 2) {
      Serial.print(counter);
      Serial.print(" ");
      Serial.print(x1);
      Serial.print(" ");
      Serial.print(x2);
      Serial.print(" ");
      Serial.print(y1);
      Serial.print(" ");
      Serial.print(y2);
      Serial.println(" ");
    }
  }
  if (debug)
  {
    plottimer = millis() - scratch;
    Serial.print("plot time: ");
    Serial.println(plottimer);
  }
}

//clear screen and draw and label axes, run once from setup()
void screen_setup(uint16_t backgroundColor, uint16_t penColor, int margin1, int margin2)
{
  int scratch = millis();
  int width = tft.width();
  int height = tft.height();
  tft.fillScreen(backgroundColor);
  tft.setRotation(2);
  tft.drawLine(margin1, height - margin2, margin1, margin2, penColor);
  tft.drawLine(margin1, margin2,  width - margin1, margin2, penColor);
    
  tft.setCursor( 60,5);
  tft.setTextColor(penColor);
  tft.setTextSize(2);
  tft.println("voltage");
  
  tft.setRotation(3);
  tft.setCursor(60, 220);
  tft.println("time");
  tft.setRotation(2);
  int setup_time = millis() - scratch;
  if (debug)
  {
    Serial.print("setup plot time: ");
    Serial.println(setup_time);
  }
}

void graph_erase_data(uint16_t penColor, int x1, int y1, int x2, int y2)
{
  int scratch, erase_time;
  if (debug) { scratch = millis(); }
  tft.fillRect(x1, y1, x2, y2, penColor);
  if (debug)
  {
    erase_time = millis()-scratch;
    Serial.print("erase plot time: ");
    Serial.println(erase_time);
  }
}
