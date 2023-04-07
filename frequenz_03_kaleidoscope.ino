// TODO: Check out the bitDepth of the panel for dimming individual pixels?
// FrekvensPanel(int p_latch, int p_clock, int p_data, int bitDepth, int numPanels);

// TODO: Set brightness on button-press

#include <FrekvensPanel.h> // obviously

#include <FastLED.h> // get FastLED for the noise function

#include <EasyButton.h>


///// FREKVENS PANEL (Arduino Nano inside)
#define p_ena 6
#define p_data 4
#define p_clock 3
#define p_latch 2
#define p_btnYellow 14 // A0
#define p_btnRed 15 // A1
// #define p_microphone  17 // A3 
// #define p_lineIn 18 // A4 
FrekvensPanel panel(p_latch, p_clock, p_data);
short panelBrightness = 250; // 0 == full brightness, 240 == dim, 254 == very dim, 1 == off 

///// NOISE from FEstLED example:
// The 32bit version of our coordinates
static uint16_t xCoord;
static uint16_t yCoord;
static uint16_t zCoord;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
// uint16_t speed = 1; // almost looks like a painting, moves very slowly
// uint16_t speed = 20; // a nice starting speed, mixes well with a scale of 100
// uint16_t speed = 100; // wicked fast!
uint16_t noiseSpeed = 0; // will be ranomized later;

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.

// uint16_t scale = 1; // mostly just solid colors
// uint16_t scale = 4011; // very zoomed out and shimmery
uint16_t noiseScale = 0; // will be randomized later;

// Noise values will be between 0 and 255 (8* 32)
uint16_t inactiveThreshold = 8 *19; // Will turn on a pixel when above this value
uint16_t activeThreshold = 8 *2; // Will turn off a pixel when below this value

unsigned short maxFPS = 60;


///// PIXEL MATRIX
int matrixWidth = panel.width();
int matrixHeight = panel.height();
int matrixPixelCount = matrixHeight*matrixWidth;

int kaleidoSectionWidth = matrixWidth/2;
int kaleidoSectionHeight = matrixHeight/2;
int kaleidoSectionPixelCount = kaleidoSectionHeight*kaleidoSectionWidth;

//uint16_t kaleidoSectionMatrix[kaleidoSectionWidth][kaleidoSectionHeight]; // Throws an error...
uint16_t kaleidoSectionMatrix[8][8]; // FIXME: uses static values.


///// BUTTONS
EasyButton buttonRed(p_btnRed);
EasyButton buttonYellow(p_btnYellow);
unsigned short buttonLongPressDuration = 777;


//    ________________  _____ 
//   / __/ __/_  __/ / / / _ \
//  _\ \/ _/  / / / /_/ / ___/
// /___/___/ /_/  \____/_/    
//                           
void setup() {
  // For debugging
  Serial.begin(9600);
  
  // Initialize our coordinates and noise parameters to some random values
  xCoord = random16();
  yCoord = random16();
  zCoord = random16();
  randomizeNoiseParameters(false);
  
  // Setup panel
  panel.clear();
  panel.scan();
  pinMode(p_ena, OUTPUT); 
  analogWrite(p_ena, panelBrightness);

  //Setup Frekens buttons
  buttonYellow.begin();
  buttonYellow.onPressed(onButtonYellowShortPressed);
  buttonYellow.onPressedFor(buttonLongPressDuration, onButtonYellowLongPressed);
  
  buttonRed.begin();
  buttonRed.onPressed(onButtonRedShortPressed);
  buttonRed.onPressedFor(buttonLongPressDuration, onButtonRedLongPressed);
}

//    __   ____  ____  ___ 
//   / /  / __ \/ __ \/ _ \
//  / /__/ /_/ / /_/ / ___/
// /____/\____/\____/_/    
//
void loop() {
  buttonYellow.read();
  buttonRed.read();
  renderPanel();
}

//    ___  ___   _  ________ 
//   / _ \/ _ | / |/ / __/ / 
//  / ___/ __ |/    / _// /__
// /_/  /_/ |_/_/|_/___/____/
//
void renderPanel(){
  static unsigned long int timeLastPainted = 0;
  static unsigned short frameTime = 1000/maxFPS;

  unsigned long int timeNow = millis();
  unsigned long int timeSinceLastPaint = timeNow - timeLastPainted;
  if (timeSinceLastPaint >= frameTime) {
    timeLastPainted = timeNow;
    
    renderNoiseToSection();
    panel.clear();
    preparePanel();
    panel.scan();
    
    // move forward in space
    zCoord += noiseSpeed;  
  }
}

void preparePanel(){
  for (int x = 0; x < matrixWidth; x++){
    for (int y = 0; y < matrixHeight; y++){
      uint16_t pixelValue;

      // bottom left quarter
      if (x < kaleidoSectionWidth && y < kaleidoSectionHeight){
        pixelValue = kaleidoSectionMatrix[x][y];
      }

      // top left quarter
      if (x >= kaleidoSectionWidth && y < kaleidoSectionHeight){
        int xMirrored = x - ((x - kaleidoSectionWidth)*2)-1; // dont ask me how I came up with this...
        pixelValue = kaleidoSectionMatrix[xMirrored][y];
      }

      // bottom right quarter
      if (x < kaleidoSectionWidth && y >= kaleidoSectionHeight){
        int yMirrored = y - ((y - kaleidoSectionHeight)*2)-1; // again, dont ask me how I came up with this...
        pixelValue = kaleidoSectionMatrix[x][yMirrored];
      }
      
      // top right quarter
      if (x >= kaleidoSectionWidth && y >= kaleidoSectionHeight){
        int xMirrored = x - ((x - kaleidoSectionWidth)*2)-1; // same...
        int yMirrored = y - ((y - kaleidoSectionHeight)*2)-1; // ... again
        pixelValue = kaleidoSectionMatrix[xMirrored][yMirrored];
      }


      boolean oldPixelValue = panel.getPixel(x, y);

      // Only turn on pixels, if value is above inactiveThreshold
      if (oldPixelValue == false && pixelValue > inactiveThreshold){
        panel.drawPixel(x, y, 1);  
      }

      // Only turn off pixels, if value is below activeThreshold
      if (oldPixelValue == true && pixelValue < activeThreshold){
        panel.drawPixel(x, y, 0);  
      }
    }
  }
}

//    _  ______  ____________
//   / |/ / __ \/  _/ __/ __/
//  /    / /_/ // /_\ \/ _/  
// /_/|_/\____/___/___/___/  
//                          
void renderNoiseToSection(){
  for(int x = 0; x < kaleidoSectionWidth; x++) {
    int xOffset = noiseScale * x;
    for(int y = 0; y < kaleidoSectionHeight; y++) {
      int yOffset = noiseScale * y;
      kaleidoSectionMatrix[x][y] = inoise8(xCoord + xOffset, yCoord + yOffset, zCoord);
    }
  }
}

void randomizeNoiseParameters(boolean highRange){
  if (highRange) {
    noiseSpeed = random8(5, 25);
    noiseScale = random8(10, 200);
  } else {
    noiseSpeed = random8(5, 9); // 7 was nice
    noiseScale = random8(35, 49); // 42 was nice    
  }
}


//    ___  __  __________________  _  ______
//   / _ )/ / / /_  __/_  __/ __ \/ |/ / __/
//  / _  / /_/ / / /   / / / /_/ /    /\ \  
// /____/\____/ /_/   /_/  \____/_/|_/___/  
//                                          
void onButtonYellowShortPressed(){
  Serial.println("Yellow button short press.");
  // 0 == full brightness, 240 == dim, 254 == very dim, 1 == off
  panelBrightness -= 25;
  if (panelBrightness <= 0){
    panelBrightness = 255;
  } 
  analogWrite(p_ena, panelBrightness);
}

void onButtonYellowLongPressed(){
  Serial.println("Yellow button long press.");
  if (panelBrightness > 0) {
    panelBrightness = 0;
  } else {
    panelBrightness = 250;
  }
  analogWrite(p_ena, panelBrightness);
}

void onButtonRedShortPressed(){
  Serial.println("Red button short press.");
  randomizeNoiseParameters(true);
}

void onButtonRedLongPressed(){
  Serial.println("Red button long press.");
  randomizeNoiseParameters(false);
}
