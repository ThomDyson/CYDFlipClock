# Flip Clock on a Cheap Yellow Display (CYD)
This program uses a Cheap Yellow Display to show a classic flip clock.  The design is based on Jeremy Cook's [Simulated Flip Clock](https://github.com/funnypolynomial/FlipClock), but I've updated the code to use modern libraries and a CYD. **Note:** Touch the screen to force a flip.

![Animation](./assets/CYDFlipClock1.gif)

## Requirements

CYD (part ESP32-2432S028R) is an ESP32 with a a built-in 2.8" LCD display (240x320) touchscreen. More information is available [here](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) and from [Random Nerds](https://randomnerdtutorials.com/projects-esp32/). 

## Dependencies

This project requires the following libraries.

**[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)**
Arduino and PlatformIO IDE compatible TFT library optimized for the Raspberry Pi Pico (RP2040), STM32, ESP8266 and ESP32 that supports different driver chips.

**[XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)**
Touchscreen Arduino Library for XPT2046 Touch Controller Chip. Even though TFT_eSPI provides some support for touchscreens, it doesn't work on the CYD, so you need this for touchscreen functions. **Note:** If you are using PlatformIO, you must configure the library in the platformio.ini by pointing to the official [repo](https://github.com/PaulStoffregen/XPT2046_Touchscreen.git) or manually downloading the lib. The library included by PlatformIO is out of date.

**roboto_64.h** - This file holds the fonts for the large digits. Small digits use a font from the TFT library.

## Instructions
Download main.cpp and robot_64.h. robot_64.h is an include, so it needs to be where your compiler keeps included files. You will need to configure two variables, "ssid" and "WiFipassword" for your setup. To use the code as written, create a "secrets.h" text file in your "include" folder.  It should hold something like

```cpp
const char *ssid         = "My Home Wifi";               // Change this to your WiFi SSID
const char *WiFipassword = "MySuperSecretPassword";      // Change this to your WiFi password
```

to hold your WiFi credentials. The other option is to add these to the top of the main.cpp file and comment out the "#include <secrets.h>".

To use a 24 hour display, set this line `const bool MilTime = true;` to true. To use a 12 hour display, set MilTime to false, `const bool MilTime = false;`

Not all CYDs are the same.  For my CYD (a variant of the original), I had to add  `-DTFT_INVERSION_ON=1` to platformio.ini (or the equivalent  ` #define TFT_INVERSION_ON` to the User_Setup.h). You may need to remove that to get your colors correct.

In the platformio.ini, `upload_speed = 961200` is a bit aggressive. You may need to reduce this if your CYD/cable/PC are in a mood.


## Visual Studio Code/Platformio vs Arduino IDE
The source is configured to use platformio.ini to configure settings for TFT_eSPI.  To configure these settings under Arduino IDE, copy the [User_Setup.h](https://raw.githubusercontent.com/RuiSantosdotme/ESP32-TFT-Touchscreen/main/configs/User_Setup.h) from Random Nerds to your TFT_eSPI folder under your Arduino\libraries. Details are at [Random Nerds CYD Tutorial](https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/). 
