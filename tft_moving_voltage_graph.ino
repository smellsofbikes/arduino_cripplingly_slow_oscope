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
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void push(int *ary, int arysize);
unsigned long plotgraph(uint16_t color, int *ary, int setsize, int dataset_pointer, int stepwidth);
void screen_setup(uint16_t backgroundColor, uint16_t penColor, int margin1, int margin2);
void graph_erase_data(uint16_t penColor, int x1, int x2, int y1, int y2);
int fileExists(char buffer[10]);

int debug = 0;

void setup(void) {
  //SREG = SREG | 0b10000000;      //enable global interrupts
  //ADMUX = 0b10100101;            //vref, left-justified, ADC5
  //ADCSRA = 0b11101111;           //enable adc, enable interrupt, prescalar 128
 
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
  int setsize = 56;                   // this determines how many samples are displayed on the screen
  int dataset[setsize];
  int data_index = 0;
  int counter, filename, stepwidth = 0;
  char namebuffer[] = "log00.txt";        // buffer for sdcard name
  for (uint8_t i = 0; i < 100; i++)       // neat bit of code from adafruit to make unique names rather than overwriting
  {
    namebuffer[3] = i/10 + '0';
    namebuffer[4] = i%10 + '0';
    if (! SD.exists(namebuffer)) 
    {
      if(debug) { Serial.print("Datalog name: "); Serial.println(namebuffer); }
      break; 
    }
  }
  
  for (counter = 0; counter < setsize; counter++)   // zero out data array
    dataset[counter] = 0;
  
  while(1)
  {
    int measurement = analogRead(A5);    // this should be set up in hardware as a fast free-running ADC
    if (debug) {  Serial.println(measurement); }
    push(dataset, setsize);             // shift all elements in array over one
    dataset[0] = measurement;           // add new element to array
    File dataFile = SD.open(namebuffer, FILE_WRITE);
    dataFile.println(measurement);
    dataFile.close();
    graph_erase_data(WHITE, 21, 21, tft.width(), tft.height());
    plotgraph(BLACK, dataset, setsize, data_index, stepwidth);  // graph it
  }
}

ISR(ADC_vect) {
//  int measurement = ADCH;
//  if (data_pointer > setsize)
//    data_pointer = 0;
//  dataset[data_pointer] = measurement;
}

void push(int *ary, int setsize)
{
  int counter;
  for (counter = setsize-1; counter > -1; counter--)
    ary[counter+1] = ary[counter];
}

unsigned long plotgraph(uint16_t color, int *ary, int setsize, int dataset_pointer, int stepwidth)
{
  int counter, scratch, plottimer;
  int x1, y1, x2, y2, width = tft.width(), height = tft.height();
  if (debug) {
    Serial.print("width: ");
    Serial.print(width);
    Serial.print(", height: ");
    Serial.println(height);
    scratch = millis();           // time how long plotting takes
    }
  int margin = 20;
  int stepsize = int((height - margin)/setsize);
  if (debug) {
    Serial.print("Stepsize: ");
    Serial.println(stepsize);
  }
  
  int lasty = margin;
  tft.setRotation(2);
  for (counter = 0; counter < setsize-1; counter++)
  {
    y1 = lasty;
    y2 = lasty + stepsize;
    lasty = y2;
    x1 = (ary[counter] / 6)+ margin;
    x2 = (ary[counter+1] / 6)+ margin;
    tft.drawLine(x1, y1, x2, y2, BLUE);
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

//clear screen and draw and label axes
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

int fileExists(char buffer[10])
{
  FILE *fp;
  if((fp = fopen(buffer, "r")) == NULL)
    return(0);
  fclose(fp);
  return(1);
}
