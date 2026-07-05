#ifndef CONFIG_H
#define CONFIG_H

// Display Configuration
const int DISPLAY_WIDTH = 400;
const int DISPLAY_HEIGHT = 300;
const int DISPLAY_MARGIN = 10;

// E-Paper Display Settings
const int EPD_INIT_BAUD_RATE = 115200;
const bool EPD_INIT_RESET = true;
const int EPD_INIT_DELAY = 50;
const bool EPD_INIT_PARTIAL = false;

// Display Settings
const int MAX_TEXT_LENGTH = 30;
const int LOOP_DELAY = 10; // milliseconds

// Font Size Constants
const int FONT_SIZE_SMALL = 12;
const int FONT_SIZE_MEDIUM = 18;
const int FONT_SIZE_LARGE = 24;

// Row Heights (in pixels)
const int ROW_HEIGHT_SMALL = 32;
const int ROW_HEIGHT_MEDIUM = 40;
const int ROW_HEIGHT_LARGE = 48;

#endif // CONFIG_H
