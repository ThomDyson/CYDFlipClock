#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <roboto_64.h>

// *****************
// create secrets.h  in the include folder and put your wifi data there
//       OR
// comment out this include and uncomment the lines for ssid and WiFipassword
// *****************

#include <secrets.h>
// const char *ssid         = "My Home Wifi Name";               // Change this to your WiFi SSID
// const char *WiFipassword = "MySuperSecretPassword"; // Change this to your WiFi password

const bool MilTime = false; // 24 hour clock, AKA military time
// Touchscreen pins
#define XPT2046_IRQ 36  // T_IRQ
#define XPT2046_MOSI 32 // T_DIN
#define XPT2046_MISO 39 // T_OUT
#define XPT2046_CLK 25  // T_CLK
#define XPT2046_CS 33   // T_CS

#define CLOCK_BACKGROUND_COLOR 0x75BE
#define BORDER_HIGHLIGHT_COLOR TFT_WHITE
// 0x9CF3

#define LARGE_FONT roboto_regular64pt7b
#define MEDIUM_FONT FreeSans24pt7b
#define SMALL_FONT FreeSans18pt7b

const int16_t lightSensorPin = 34; // Pin connected to the light sensor
const int16_t backlightPin   = 21; // Pin connected to the display backlight (PWM capable)

SPIClass touchscreenSPI = SPIClass( VSPI );
XPT2046_Touchscreen touchscreen( XPT2046_CS, XPT2046_IRQ );

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_X_PADDING 2
#define TOP_BORDER_WIDTH 3
#define SIDE_BORDER_WIDTH 3
#define LARGE_DIGIT_INTER_SPACE 2
#define SMALL_DIGIT_INTER_SPACE 1
#define LARGE_DIGIT_COUNT 4
#define SMALL_DIGIT_COUNT 8
// Touchscreen coordinates: (x, y) and pressure (z)
int16_t touchX, touchY, touchZ;
int16_t text_height = 0;

// Set X and Y coordinates for center of display
const int16_t centerX = SCREEN_WIDTH / 2;
const int16_t centerY = SCREEN_HEIGHT / 2;

TFT_eSPI tft = TFT_eSPI();

TFT_eSprite sprite_LargeTopBorder    = TFT_eSprite( &tft ); // top border for large sprites
TFT_eSprite sprite_LargeBottomBorder = TFT_eSprite( &tft ); // bottom
TFT_eSprite sprite_LargeSideBorder   = TFT_eSprite( &tft ); // side
TFT_eSprite sprite_LargeCenterLine   = TFT_eSprite( &tft ); // center line

TFT_eSprite sprite_SmallTopBorder    = TFT_eSprite( &tft ); // top border for small sprites
TFT_eSprite sprite_SmallBottomBorder = TFT_eSprite( &tft ); // bottom
TFT_eSprite sprite_SmallSideBorder   = TFT_eSprite( &tft ); // side
TFT_eSprite sprite_SmallCenterLine   = TFT_eSprite( &tft ); // center line

TFT_eSprite sprite_blipBackground = TFT_eSprite( &tft ); // centered of screen flasher
TFT_eSprite sprite_blipCenter     = TFT_eSprite( &tft ); // inset for blip

TFT_eSprite sprite_newValue     = TFT_eSprite( &tft ); // New sprite for modified image
TFT_eSprite sprite_currentValue = TFT_eSprite( &tft ); // New sprite for modified image
TFT_eSprite sprite_Display      = TFT_eSprite( &tft ); // New sprite for modified image

int16_t h1X, h2X, m1X, m2X, DOW1X, DOW2X, DOW3X, Date1X, Date2X, Month1X, Month2X, Month3X; // x position for all the sprites

// Define dimensions for each sprite (adjust size as needed)
int16_t largeSpriteWidth        = ( ( SCREEN_WIDTH - ( SCREEN_X_PADDING * 2 ) - ( SIDE_BORDER_WIDTH * LARGE_DIGIT_COUNT * 2 ) + ( ( LARGE_DIGIT_COUNT - 1 ) * LARGE_DIGIT_INTER_SPACE ) ) ) / LARGE_DIGIT_COUNT; // Width for each digit sprite
int16_t largeSpriteSpace        = largeSpriteWidth + SIDE_BORDER_WIDTH * 2;                                                                                                                                      // the full width of a sprite and borders
const int16_t largeSpriteHeight = ( 0.66 * SCREEN_HEIGHT ) - ( TOP_BORDER_WIDTH * 2 ) - 50;                                                                                                                      // Height for each digit sprite
const int16_t largeSpriteTop    = ( SCREEN_HEIGHT - largeSpriteHeight - ( TOP_BORDER_WIDTH * 3 ) );

int16_t smallSpriteWidth        = ( SCREEN_WIDTH - ( SCREEN_X_PADDING * 2 ) - ( SIDE_BORDER_WIDTH * SMALL_DIGIT_COUNT * 2 ) + +( ( SMALL_DIGIT_COUNT - 1 ) * SMALL_DIGIT_INTER_SPACE ) ) / ( SMALL_DIGIT_COUNT + 1 ); // Width for each  sprite, based on number of sprites plus padding
int16_t smallSpriteSpace        = smallSpriteWidth + TOP_BORDER_WIDTH * 2;
const int16_t smallSpriteHeight = ( 0.30 * SCREEN_HEIGHT ) - ( TOP_BORDER_WIDTH * 2 ) - 20; // Height for each digit sprite
const int16_t smallSpriteTop    = ( TOP_BORDER_WIDTH * 3 );

const char *monthsOfYear[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

const char *daysOfWeek[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

int8_t currentMinute1 = 9;
int8_t currentMinute2 = 9;
int8_t currentHour1   = 9;
int8_t currentHour2   = 9;
int8_t currentDate1   = 9;
int8_t currentDate2   = 9;
char currentDOW[4]    = "XXX";
char currentMonth[4]  = "XXX";
bool firstpass;

static unsigned long lastMillis = 0;
int blipCycle                   = 0;

void setup_character_sprite( TFT_eSprite &thisSprite, int16_t width, int16_t height, uint8_t colorDepth, uint16_t textColor, uint16_t bgColor, const GFXfont *font ) {
  thisSprite.setColorDepth( colorDepth );
  thisSprite.createSprite( width, height );
  thisSprite.setTextDatum( MC_DATUM );
  thisSprite.setTextColor( textColor, bgColor );
  thisSprite.setFreeFont( font );
}

void please_wait() {

  setup_character_sprite( sprite_Display, largeSpriteWidth, largeSpriteHeight, 16, TFT_WHITE, CLOCK_BACKGROUND_COLOR, &MEDIUM_FONT );
  int16_t midWidth  = ( largeSpriteWidth / 2 );
  int16_t midHeight = ( largeSpriteHeight / 2 );
  sprite_Display.fillRect( 0, 0, largeSpriteWidth, largeSpriteHeight, CLOCK_BACKGROUND_COLOR );
  sprite_Display.drawString( "W", midWidth, midHeight );                   // Draw the text in the sprite
  sprite_Display.fillRect( 0, midHeight, largeSpriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( h1X, largeSpriteTop );

  sprite_Display.fillRect( 0, 0, largeSpriteWidth, largeSpriteHeight, CLOCK_BACKGROUND_COLOR );
  sprite_Display.drawString( "A", midWidth, midHeight );                   // Draw the text in the sprite
  sprite_Display.fillRect( 0, midHeight, largeSpriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( h2X, largeSpriteTop );

  sprite_Display.fillRect( 0, 0, largeSpriteWidth, largeSpriteHeight, CLOCK_BACKGROUND_COLOR );
  sprite_Display.drawString( "I", midWidth, midHeight );                   // Draw the text in the sprite
  sprite_Display.fillRect( 0, midHeight, largeSpriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( m1X, largeSpriteTop );

  sprite_Display.fillRect( 0, 0, largeSpriteWidth, largeSpriteHeight, CLOCK_BACKGROUND_COLOR );
  sprite_Display.drawString( "T", midWidth, midHeight );                   // Draw the text in the sprite
  sprite_Display.fillRect( 0, midHeight, largeSpriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( m2X, largeSpriteTop );
  sprite_Display.deleteSprite();
}

// function to darken a color to look like it is in shadow
uint16_t shadow_color( uint16_t color, float shadowFactor ) {
  // Extract RGB components from the 16-bit RGB565 color
  uint8_t r = ( color >> 11 ) & 0x1F; // 5 bits for red
  uint8_t g = ( color >> 5 ) & 0x3F;  // 6 bits for green
  uint8_t b = color & 0x1F;           // 5 bits for blue

  // Apply shadow effect by reducing the brightness of each color channel
  r = ( uint8_t ) ( r * shadowFactor );
  g = ( uint8_t ) ( g * shadowFactor );
  b = ( uint8_t ) ( b * shadowFactor );

  // Clamp values to the maximum allowed for RGB565 (5 bits for red and blue, 6 bits for green)
  r = r > 31 ? 31 : r;
  g = g > 63 ? 63 : g;
  b = b > 31 ? 31 : b;

  // Rebuild the RGB565 color from the adjusted channels
  uint16_t shadow_color = ( r << 11 ) | ( g << 5 ) | b;

  return shadow_color;
}

// function to animate the transition between characters
// animation happens in 4 steps:
// 1. draw the orig value,
// 2. draw the top half of the orig value in a constrained space so it looks like foreshortening. reveal the top of the new value behind that.
// 3. draw the top half of the new value and the bottom  half of the old value.
// 4. draw the old value, then draw the top of the new over that and the bottom half of the new in a constrained space over the bottom
// 5. draw the new value.
void change_sprite_display( const char *currentValue, const char *newValue, int16_t posX, int16_t posY, bool largeSprite ) {
  int16_t spriteHeight;
  int16_t spriteWidth;
  int16_t animationDelay = 100;
  int16_t newY;
  int16_t lastYrow  = 0;
  float scalefactor = ( 2.0 / 3.0 ); // this sets the space for the partial drawing segments. gotta force floating  point math

  int16_t newRangeSize;
  int16_t newMaxY;
  int16_t newMinY;
  GFXfont thisFont;

  if ( largeSprite ) {
    spriteHeight = largeSpriteHeight;
    spriteWidth  = largeSpriteWidth;
    thisFont     = LARGE_FONT;
  } else {
    spriteHeight = smallSpriteHeight;
    spriteWidth  = smallSpriteWidth;
    thisFont     = SMALL_FONT;
  }
  // 3 sprites - current value, new value and the display image we push to the screen
  // at each step, the display is some combination of current and new
  setup_character_sprite( sprite_Display, spriteWidth, spriteHeight, 16, TFT_WHITE, CLOCK_BACKGROUND_COLOR, &thisFont );
  setup_character_sprite( sprite_newValue, spriteWidth, spriteHeight, 16, TFT_WHITE, CLOCK_BACKGROUND_COLOR, &thisFont );
  setup_character_sprite( sprite_currentValue, spriteWidth, spriteHeight, 16, TFT_WHITE, CLOCK_BACKGROUND_COLOR, &thisFont );
  sprite_Display.fillSprite( CLOCK_BACKGROUND_COLOR );
  sprite_newValue.fillSprite( CLOCK_BACKGROUND_COLOR );
  sprite_currentValue.fillSprite( CLOCK_BACKGROUND_COLOR );

  int16_t midWidth  = ( spriteWidth / 2 );
  int16_t midHeight = ( spriteHeight / 2 );
  Serial.printf( "Animating from %s to %s\n", currentValue, newValue );
  sprite_currentValue.drawString( currentValue, midWidth, midHeight );     // Draw the text in the sprite
  sprite_currentValue.fillRect( 0, midHeight, spriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_newValue.drawString( newValue, midWidth, midHeight );             // Draw the text in the sprite
  sprite_newValue.fillRect( 0, midHeight, spriteWidth, 1, TFT_BLACK );     // add in split line

  // display current value
  sprite_currentValue.pushToSprite( &sprite_Display, 0, 0 );
  sprite_Display.pushSprite( posX, posY );

  // animation step one. fold the top half a little
  newRangeSize = midHeight * scalefactor;
  newMaxY      = midHeight + 1;
  newMinY      = newMaxY - newRangeSize;
  sprite_newValue.pushToSprite( &sprite_Display, 0, 0 ); // set the new display and then over write parts

  for ( int y = 0; y < spriteHeight; y++ ) {
    if ( y < newMaxY ) {
      newY = map( y, 0, midHeight, newMinY, newMaxY ); // re draw the  top half in less space, "revealing" the new value a little
    } else {
      newY = y; // redraw the bottom half as is
    }
    for ( int x = 0; x < spriteWidth; x++ ) {
      uint16_t color = sprite_currentValue.readPixel( x, y ); // Read the pixel from the original sprite
      if ( ( ( lastYrow + 1 ) < newY ) && ( lastYrow != 0 ) ) {
        sprite_Display.drawPixel( x, newY, color );         // Draw it on the new sprite
        sprite_Display.drawPixel( x, lastYrow + 1, color ); // sometimes the map function leaves a gap from rounding, so ensure there are no gaps
      } else {
        sprite_Display.drawPixel( x, newY, color ); // Draw it on the new sprite
      }
    }
    lastYrow = newY;
  }
  sprite_Display.fillRect( 0, newMinY, spriteWidth, 1, TFT_BLACK );   // simulate top of flip card
  sprite_Display.fillRect( 0, midHeight, spriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( posX, posY );                            // Display  sprite
  delay( animationDelay * .9 );

  // animation step two, just show the bottom, with a shadow look
  sprite_Display.fillSprite( CLOCK_BACKGROUND_COLOR );   // reset the display sprite to blank
  sprite_newValue.pushToSprite( &sprite_Display, 0, 0 ); // set the display to the new value and then over write parts
  lastYrow = 0;
  for ( int y = midHeight; y < spriteHeight; y++ ) { // leave the top alone and only draw the bottom
    for ( int x = 0; x < spriteWidth; x++ ) {
      uint16_t color = shadow_color( sprite_currentValue.readPixel( x, y ), 0.8 ); // Read the pixel from the original sprite
      sprite_Display.drawPixel( x, y, color );                                     // Draw it on the new sprite
    }
  }
  sprite_Display.pushSprite( posX, posY ); // Display  sprite
  delay( animationDelay * .8 );

  // animation step 3, new value top, inverted lower section
  sprite_Display.fillSprite( CLOCK_BACKGROUND_COLOR );       // reset the new sprite to blank
  sprite_currentValue.pushToSprite( &sprite_Display, 0, 0 ); // set the new display to the CURRENT value and then over write new parts
  lastYrow = 0;
  newMaxY  = midHeight + newRangeSize; // scale factor down from the mid point
  newMinY  = midHeight - 1;            // mid point
  for ( int y = 0; y < spriteHeight; y++ ) {
    if ( y < midHeight ) { // top half so we just write the new values
      for ( int x = 0; x < spriteWidth; x++ ) {
        uint16_t color = sprite_newValue.readPixel( x, y ); // Read the pixel from the new  value
        sprite_Display.drawPixel( x, y, color );            // Draw it on the new sprite
      }
    } else {
      newY = map( y, midHeight, spriteHeight, newMinY, newMaxY ); // map the bottom half of the new image to a constrained space
      for ( int x = 0; x < spriteWidth; x++ ) {
        uint16_t color = sprite_newValue.readPixel( x, y );       // Read the pixel from the new  value
        if ( ( ( lastYrow + 1 ) < newY ) && ( lastYrow != 0 ) ) { // mapping sometimes leaves gaps, so fill them
          sprite_Display.drawPixel( x, newY, color );             // Draw it on the new sprite
          sprite_Display.drawPixel( x, lastYrow + 1, color );     // Draw it on the new sprite
        } else {
          sprite_Display.drawPixel( x, newY, color ); // Draw it on the new sprite
        }
      }
      lastYrow = newY;
      if ( y > newMaxY ) { // redraw the last portion is in shadow
        for ( int x = 0; x < spriteWidth; x++ ) {
          uint16_t color = sprite_Display.readPixel( x, y );           // Read the pixel from the new  value
          sprite_Display.drawPixel( x, y, shadow_color( color, .8 ) ); // Draw it on the new sprite
        }
      }
    }
  }
  sprite_Display.fillRect( 0, newMaxY, spriteWidth, 1, TFT_BLACK );   // simulate bottom of flip
  sprite_Display.fillRect( 0, midHeight, spriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( posX, posY );                            // Display  sprite
  delay( animationDelay * .7 );                                       // the animation gets a little faster as the card flips down.

  // last step draw full new value
  sprite_Display.fillSprite( CLOCK_BACKGROUND_COLOR ); // reset the new sprite to blank
  sprite_newValue.pushToSprite( &sprite_Display, 0, 0 );
  sprite_Display.fillRect( 0, midHeight, spriteWidth, 1, TFT_BLACK ); // add in split line
  sprite_Display.pushSprite( posX, posY );                            // Display  sprite
  sprite_Display.deleteSprite();
  sprite_currentValue.deleteSprite();
  sprite_newValue.deleteSprite();
}

void setup_blip_sprites() {
  sprite_blipBackground.createSprite( 14, 14 );
  sprite_blipBackground.fillSprite( TFT_BLACK ); // You can change the color or use an image
  sprite_blipBackground.fillRect( 2, 2, sprite_blipBackground.width() - 4, sprite_blipBackground.height() - 4, CLOCK_BACKGROUND_COLOR );

  sprite_blipCenter.createSprite( 6, 6 );
  sprite_blipCenter.fillSprite( TFT_WHITE );
}

void setup_large_sprite_top_border() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_LargeTopBorder.setColorDepth( 8 );
  sprite_LargeTopBorder.createSprite( largeSpriteWidth, TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_LargeTopBorder.fillSprite( TFT_BLACK );                            // Fill sprite with black background
  // Draw a 2-pixel white border along the long side (BORDER_LENGTH)
  sprite_LargeTopBorder.fillRect( 0, TOP_BORDER_WIDTH - 1, largeSpriteWidth, 1, BORDER_HIGHLIGHT_COLOR ); // highlight
}

void setup_large_sprite_bottom_border() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_LargeBottomBorder.setColorDepth( 8 );
  sprite_LargeBottomBorder.createSprite( largeSpriteWidth + ( 2 * SIDE_BORDER_WIDTH ), TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_LargeBottomBorder.fillSprite( TFT_BLACK );                                                        // Fill sprite with black background
  // Draw a 2-pixel white border along the long side (BORDER_LENGTH)
  sprite_LargeBottomBorder.fillRect( 0, TOP_BORDER_WIDTH - 1, largeSpriteWidth + ( 2 * SIDE_BORDER_WIDTH ), 1, BORDER_HIGHLIGHT_COLOR ); // highlight
}

void setup_large_sprite_center_line() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_LargeCenterLine.setColorDepth( 8 );
  sprite_LargeCenterLine.createSprite( largeSpriteWidth, 1 ); // Create a sprite with specified width and height
  sprite_LargeCenterLine.fillSprite( TFT_BLACK );             // Fill sprite with black background
}

void add_hinge( TFT_eSprite &thisSprite, int16_t steps ) {
  uint16_t colorStartTop    = TFT_WHITE; //// was 0x75BE
  uint16_t colorEndTop      = 0x75BE;    /// 0x4B05;
  uint16_t colorStartBottom = 0x75BE;    //// was 0x75BE
  uint16_t colorEndBottom   = TFT_BLACK; /// 0x4B05;

  // bottom hinge
  thisSprite.fillRectVGradient( 0, ( thisSprite.height() + TOP_BORDER_WIDTH ) / 2, SIDE_BORDER_WIDTH, steps, colorStartBottom, colorEndBottom );
  // 0x4B05 is a darker version of the background
  // top hinge
  thisSprite.fillRectVGradient( 0, ( thisSprite.height() + TOP_BORDER_WIDTH ) / 2 - steps + 1, SIDE_BORDER_WIDTH, steps, colorStartTop, colorEndTop );
}

void setup_large_sprite_side_border() {
  // Define sprite dimensions based onTOP_BORDER_WIDTH and BORDER_LENGTH
  sprite_LargeSideBorder.setColorDepth( 8 );
  sprite_LargeSideBorder.createSprite( SIDE_BORDER_WIDTH, largeSpriteHeight + TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_LargeSideBorder.fillSprite( TFT_BLACK );                                                 // Fill sprite with black background

  add_hinge( sprite_LargeSideBorder, 10 );
}

void setup_small_sprite_top_border() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_SmallTopBorder.setColorDepth( 8 );
  sprite_SmallTopBorder.createSprite( smallSpriteWidth, TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_SmallTopBorder.fillSprite( TFT_BLACK );                            // Fill sprite with black background
  // Draw a 2-pixel white border along the long side (BORDER_LENGTH)
  sprite_SmallTopBorder.fillRect( 0, TOP_BORDER_WIDTH - 1, smallSpriteWidth, 1, BORDER_HIGHLIGHT_COLOR ); // highlight
}

void setup_small_sprite_bottom_border() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_SmallBottomBorder.setColorDepth( 8 );
  sprite_SmallBottomBorder.createSprite( smallSpriteWidth + ( 2 * SIDE_BORDER_WIDTH ), TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_SmallBottomBorder.fillSprite( TFT_BLACK );                                                        // Fill sprite with black background
  // Draw a 2-pixel white border along the long side (BORDER_LENGTH)
  sprite_SmallBottomBorder.fillRect( 0, TOP_BORDER_WIDTH - 1, smallSpriteWidth + ( 2 * SIDE_BORDER_WIDTH ), 1, BORDER_HIGHLIGHT_COLOR ); // highlight
}

void setup_small_sprite_side_border() {
  // Define sprite dimensions based onTOP_BORDER_WIDTH and BORDER_LENGTH
  sprite_SmallSideBorder.setColorDepth( 8 );
  sprite_SmallSideBorder.createSprite( SIDE_BORDER_WIDTH, smallSpriteHeight + TOP_BORDER_WIDTH ); // Create a sprite with specified width and height
  sprite_SmallSideBorder.fillSprite( TFT_BLACK );                                                 // Fill sprite with black background
  add_hinge( sprite_SmallSideBorder, 5 );
}

void setup_small_sprite_center_line() {
  // for horizontal border,TOP_BORDER_WIDTH is the Y dimension
  sprite_SmallCenterLine.setColorDepth( 8 );
  sprite_SmallCenterLine.createSprite( smallSpriteWidth, 1 ); // Create a sprite with specified width and height
  sprite_SmallCenterLine.fillSprite( TFT_BLACK );             // Fill sprite with black background
}

// this function drawers the  borders around a large sprite
void draw_large_borders( int16_t thisTargetXPos ) {
  // top borders are the same width as the main sprite
  sprite_LargeTopBorder.pushSprite( thisTargetXPos, largeSpriteTop - TOP_BORDER_WIDTH );                         // top, top borders are as wide as the main sprite
  sprite_LargeBottomBorder.pushSprite( thisTargetXPos - SIDE_BORDER_WIDTH, largeSpriteTop + largeSpriteHeight ); // bottom, bottom borders are as wide as the main sprite and the two side borders
  sprite_LargeSideBorder.pushSprite( thisTargetXPos - SIDE_BORDER_WIDTH, largeSpriteTop - TOP_BORDER_WIDTH );    // left side, borders sit under the top border
  sprite_LargeSideBorder.pushSprite( thisTargetXPos + largeSpriteWidth, largeSpriteTop - TOP_BORDER_WIDTH );     // right
                                                                                                                 //  sprite_LargeCenterLine.pushSprite( thisTargetXPos, largeSpriteTop + ( largeSpriteHeight / 2 ) - 1 ); // not drawing this here while using full hight letters because they overlap
}

// this function drawers the  borders around a small sprite
void draw_small_borders( int16_t thisTargetXPos ) {
  // top borders are the same width as the main sprite
  sprite_SmallTopBorder.pushSprite( thisTargetXPos, smallSpriteTop - TOP_BORDER_WIDTH );                             // top border
  sprite_SmallBottomBorder.pushSprite( ( thisTargetXPos - SIDE_BORDER_WIDTH ), smallSpriteTop + smallSpriteHeight ); // bottom border
  sprite_SmallSideBorder.pushSprite( ( thisTargetXPos - SIDE_BORDER_WIDTH ), smallSpriteTop - TOP_BORDER_WIDTH );    // left border
  sprite_SmallSideBorder.pushSprite( ( thisTargetXPos + smallSpriteWidth ), smallSpriteTop - TOP_BORDER_WIDTH );     // right border
}

// calculate the positions of all the characters, init them and draw them on screen
void setup_sprites() {
  // check to ensure that rounding errors don't lead to things not fitting on the screen.
  while ( ( SCREEN_X_PADDING * 2 ) + ( ( SIDE_BORDER_WIDTH * 2 + largeSpriteWidth ) * LARGE_DIGIT_COUNT ) + ( ( LARGE_DIGIT_COUNT - 1 ) * LARGE_DIGIT_INTER_SPACE ) > SCREEN_WIDTH ) {
    largeSpriteWidth--;
  }
  largeSpriteSpace = largeSpriteWidth + SIDE_BORDER_WIDTH * 2;

  // small digit have some space place holder allocation
  while ( ( SCREEN_X_PADDING * 2 ) +
              ( ( ( SIDE_BORDER_WIDTH * 2 ) + smallSpriteWidth ) * ( SMALL_DIGIT_COUNT + 1 ) ) + ( SMALL_DIGIT_COUNT * SMALL_DIGIT_INTER_SPACE ) >
          SCREEN_WIDTH ) {
    smallSpriteWidth--;
  }
  smallSpriteSpace = smallSpriteWidth + TOP_BORDER_WIDTH * 2;

  h1X = SCREEN_X_PADDING + SIDE_BORDER_WIDTH;
  h2X = SCREEN_X_PADDING + ( largeSpriteSpace + LARGE_DIGIT_INTER_SPACE ) + SIDE_BORDER_WIDTH;
  m1X = SCREEN_X_PADDING + ( largeSpriteSpace + LARGE_DIGIT_INTER_SPACE ) * 2 + SIDE_BORDER_WIDTH;
  m2X = SCREEN_X_PADDING + ( largeSpriteSpace + LARGE_DIGIT_INTER_SPACE ) * 3 + SIDE_BORDER_WIDTH;

  DOW1X   = ( SCREEN_X_PADDING + smallSpriteSpace * 0 + SIDE_BORDER_WIDTH );
  DOW2X   = ( SCREEN_X_PADDING + ( smallSpriteSpace + SMALL_DIGIT_INTER_SPACE ) * 1 + SIDE_BORDER_WIDTH );
  DOW3X   = ( SCREEN_X_PADDING + ( smallSpriteSpace + SMALL_DIGIT_INTER_SPACE ) * 2 + SIDE_BORDER_WIDTH );
  Date1X  = ( centerX - ( smallSpriteSpace - SIDE_BORDER_WIDTH ) - SMALL_DIGIT_INTER_SPACE );
  Date2X  = ( centerX + SIDE_BORDER_WIDTH + SMALL_DIGIT_INTER_SPACE );
  Month1X = ( SCREEN_WIDTH - ( SCREEN_X_PADDING + ( smallSpriteSpace + SMALL_DIGIT_INTER_SPACE ) * 3 + SIDE_BORDER_WIDTH ) );
  Month2X = ( SCREEN_WIDTH - ( SCREEN_X_PADDING + ( smallSpriteSpace + SMALL_DIGIT_INTER_SPACE ) * 2 + SIDE_BORDER_WIDTH ) );
  Month3X = ( SCREEN_WIDTH - ( SCREEN_X_PADDING + ( smallSpriteSpace + SMALL_DIGIT_INTER_SPACE ) + SIDE_BORDER_WIDTH ) );

  setup_large_sprite_top_border();
  setup_large_sprite_bottom_border();
  setup_large_sprite_side_border();
  setup_large_sprite_center_line();

  setup_small_sprite_top_border();
  setup_small_sprite_bottom_border();
  setup_small_sprite_side_border();
  setup_small_sprite_center_line();

  draw_large_borders( h1X );
  draw_large_borders( h2X );
  draw_large_borders( m1X );
  draw_large_borders( m2X );

  draw_small_borders( DOW1X );
  draw_small_borders( DOW2X );
  draw_small_borders( DOW3X );
  draw_small_borders( Month1X );
  draw_small_borders( Month2X );
  draw_small_borders( Month3X );
  draw_small_borders( Date1X );
  draw_small_borders( Date2X );

  setup_blip_sprites();
  sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 ); // 75 really should be a #define
  sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 + sprite_blipBackground.height() + SMALL_DIGIT_INTER_SPACE );
}

void init_TFT_screen() {
  touchscreenSPI.begin( XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS ); // touch functions
  touchscreen.begin( touchscreenSPI );                                         // display functions
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  touchscreen.setRotation( 1 );

  // Start the tft display
  tft.init();
  // Set the TFT display rotation in landscape mode
  tft.setRotation( 1 );
  // Clear the screen before writing to it
  tft.fillScreen( CLOCK_BACKGROUND_COLOR );
}

void update_clock( bool force_update ) {
  time_t now = time( NULL ); // Get current time as time_t
  struct tm timeInfo;
  localtime_r( &now, &timeInfo ); // Convert time_t to struct tm

  int nowDOWNumber = timeInfo.tm_wday; // Get day of week (0 = Sunday, 6 = Saturday)
  int hours        = timeInfo.tm_hour;
  int nowH1;
  int nowH2;
  if ( !MilTime && ( hours > 12 ) ) {
    hours = hours - 12;
    nowH1 = hours / 10; // First hour digit
    nowH2 = hours % 10; // Second hour digit
  } else {
    nowH1 = hours / 10; // First hour digit
    nowH2 = hours % 10; // Second hour digit
  }

  int nowM1          = timeInfo.tm_min / 10; // First minute digit
  int nowM2          = timeInfo.tm_min % 10; // Second minute digit
  int nowMonthNumber = timeInfo.tm_mon;
  int nowDate1       = timeInfo.tm_mday / 10;
  int nowDate2       = timeInfo.tm_mday % 10;

  char nowMonthCharacter[2];
  nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][0];
  nowMonthCharacter[1] = '\0';

  char placeholderNow[2];
  char placeholderCurrent[2];

  char DOWnowCharacter[2];
  DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][0];
  DOWnowCharacter[1] = '\0';

  if ( ( currentMinute2 != nowM2 ) || force_update ) {
    snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowM2 + '0' );
    snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentMinute2 + '0' );
    change_sprite_display( placeholderCurrent, placeholderNow, m2X, largeSpriteTop, true );
    currentMinute2 = nowM2;
  }

  if ( ( currentMinute1 != nowM1 ) || force_update ) {
    snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowM1 + '0' );
    snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentMinute1 + '0' );
    change_sprite_display( placeholderCurrent, placeholderNow, m1X, largeSpriteTop, true );
    currentMinute1 = nowM1;
  }
  if ( ( currentHour2 != nowH2 ) || force_update ) {
    snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowH2 + '0' );
    snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentHour2 + '0' );
    change_sprite_display( placeholderCurrent, placeholderNow, h2X, largeSpriteTop, true );
    currentHour2 = nowH2;
  }

  if ( ( currentHour1 != nowH1 ) || force_update ) {
    snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowH1 + '0' );
    snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentHour1 + '0' );
    change_sprite_display( placeholderCurrent, placeholderNow, h1X, largeSpriteTop, true );
    currentHour1 = nowH1;
    if ( ( nowH2 == 0 ) || force_update ) { // it is a new day
      Serial.printf( "Current day of week is %s to start\n", currentDOW );
      Serial.printf( "Current Month  is %s to start\n", currentMonth );
      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentDOW[0] );
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][0];
      change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW1X, smallSpriteTop, false );

      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentDOW[1] );
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][1];
      change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW2X, smallSpriteTop, false );

      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentDOW[2] );
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][2];
      change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW3X, smallSpriteTop, false );

      strcpy( currentDOW, &daysOfWeek[nowDOWNumber][0] ); // we get everything until the first null character

      snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowDate1 + '0' );
      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentDate1 + '0' );
      change_sprite_display( placeholderCurrent, placeholderNow, Date1X, smallSpriteTop, false );

      snprintf( placeholderNow, sizeof( placeholderNow ), "%c", nowDate2 + '0' );             // conv int to character
      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentDate2 + '0' ); // conv int to character
      change_sprite_display( placeholderCurrent, placeholderNow, Date2X, smallSpriteTop, false );

      currentDate1 = nowDate1;
      currentDate2 = nowDate2;

      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentMonth[0] );
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][0];
      change_sprite_display( placeholderCurrent, nowMonthCharacter, Month1X, smallSpriteTop, false );

      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentMonth[1] );
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][1];
      change_sprite_display( placeholderCurrent, nowMonthCharacter, Month2X, smallSpriteTop, false );

      snprintf( placeholderCurrent, sizeof( placeholderCurrent ), "%c", currentMonth[2] );
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][2];
      change_sprite_display( placeholderCurrent, nowMonthCharacter, Month3X, smallSpriteTop, false );

      strcpy( currentMonth, &monthsOfYear[nowMonthNumber][0] );
      Serial.printf( "Current DOW is %s to end\n", currentDOW );
      Serial.printf( "Current Month  is %s to end\n", currentMonth );
    }
  }
}

// This function reads the onboard light sensor and adjusts the led backlight
void adjust_backlight() {
  int16_t lightLevel = analogRead( lightSensorPin );
  int16_t brightness = map( lightLevel, 0, 1500, 255, 10 ); // Adjust to reverse brightness
  brightness         = constrain( brightness, 10, 255 );    // Ensure it's within PWM bounds

  // Set the backlight brightness
  analogWrite( backlightPin, brightness );
}

void setup() {
  pinMode( lightSensorPin, INPUT );
  pinMode( backlightPin, OUTPUT );
  Serial.begin( 115200 );
  init_TFT_screen();

  tft.fillScreen( CLOCK_BACKGROUND_COLOR );
  tft.drawCentreString( "Connecting to wifi.", centerX, 75, 4 );
  // Timeout variables
  unsigned long wifiStartTime     = millis();
  const unsigned long wifiTimeout = 20000; // 20 seconds
  // Wait for connection
  Serial.println( ssid );
  Serial.println( WiFipassword );
  WiFi.begin( ssid, WiFipassword );

  while ( WiFi.status() != WL_CONNECTED && ( millis() - wifiStartTime ) < wifiTimeout ) {
    delay( 500 );
    Serial.print( "." );
  }

  // Check if connected
  if ( WiFi.status() == WL_CONNECTED ) {
    Serial.println( "\nWi-Fi Connected!" );
    Serial.println( "IP Address: " );
    Serial.println( WiFi.localIP() );
    tft.fillScreen( CLOCK_BACKGROUND_COLOR );
    setup_sprites();
    please_wait();
    Serial.println( "" );
    // Set up time via NTP
    configTime( -5 * 3600, 3600, "pool.ntp.org" );
    firstpass = true;
  } else {
    Serial.println( "\nWi-Fi Connection Failed." );
    tft.fillScreen( CLOCK_BACKGROUND_COLOR );
    tft.drawCentreString( "WiFi connection failed.", centerX, 115, 4 );
  }

  //  tft.drawCentreString( "WiFi connected.", centerX, 75, 4 );
}

// Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
void printTouchToSerial( int touchX, int touchY, int touchZ ) {
  Serial.print( "X = " );
  Serial.print( touchX );
  Serial.print( " | Y = " );
  Serial.print( touchY );
  Serial.print( " | Pressure = " );
  Serial.print( touchZ );
  Serial.println();
}

void loop() {
  unsigned long currentMillis = millis();
  if ( WiFi.status() == WL_CONNECTED ) { // with no wifi connection, do nothing
    time_t now = time( nullptr );
    if ( now > 1700000000 ) { // do not display the clock until we get ntp time
      if ( firstpass ) {
        update_clock( true );
        firstpass = false;
      } else {
        update_clock( false );
      }

      // If one second has passed
      if ( currentMillis - lastMillis >= 1000 ) {
        lastMillis = currentMillis;
        if ( blipCycle == 0 ) {
          //  sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 );
          sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 + sprite_blipBackground.height() + SMALL_DIGIT_INTER_SPACE );
          sprite_blipCenter.pushSprite( ( SCREEN_WIDTH - sprite_blipCenter.width() ) / 2, 75 + ( sprite_blipBackground.height() / 2 - sprite_blipCenter.height() / 2 ) );
          blipCycle = 1;
        } else {
          sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 );
          // sprite_blipBackground.pushSprite( ( SCREEN_WIDTH - sprite_blipBackground.width() ) / 2, 75 + sprite_blipBackground.height() + SMALL_DIGIT_INTER_SPACE );
          sprite_blipCenter.pushSprite( ( SCREEN_WIDTH - sprite_blipCenter.width() ) / 2, 75 + sprite_blipBackground.height() + SMALL_DIGIT_INTER_SPACE + ( sprite_blipBackground.height() / 2 - sprite_blipCenter.height() / 2 ) );
          blipCycle = 0;
        }
      }
    }
    delay( 200 );
    adjust_backlight();
    if ( touchscreen.tirqTouched() && touchscreen.touched() ) {
      // Get Touchscreen points
      TS_Point p = touchscreen.getPoint();
      // Calibrate Touchscreen points with map function to the correct width and height
      touchX = map( p.x, 200, 3700, 1, SCREEN_WIDTH );
      touchY = map( p.y, 350, 3800, 1, SCREEN_HEIGHT );
      touchZ = p.z;
      Serial.print( " raw X = " );
      Serial.print( p.x );
      Serial.print( " | raw Y = " );
      Serial.println( p.y );
      printTouchToSerial( touchX, touchY, touchZ );
      delay( 100 );
      currentMinute1 = 9;
      currentMinute2 = 9;
      currentHour1   = 9;
      currentHour2   = 9;
      currentDate1   = 9;
      currentDate2   = 9;
      strcpy( currentDOW, "XXX" );
      strcpy( currentMonth, "XXX" );
      update_clock( true );
    }
  }
}
