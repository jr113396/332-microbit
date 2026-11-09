#include "MicroBit.h"

//
// references to external objects
//
extern NRF52Serial serial;  // serial object for displaying type of sensor and debugging 
extern NRF52I2C    i2cInt;  // internal I2C object for talking to accelerometer
extern NRF52I2C    i2cExt;  // external I2C object for talking to OLED display


//###################################################################################
// Write your additional code for Subtask 1 below here (and leave these separators in your code!)
// put functions, global variables, consts, #DEFINEs etc. that you need for Subtask 1 here
//



// TODO - instead of scrolling the memory, change where you read/write from + pointer to current location - just replace the next memory

void clearOledDisplay();
void updateDisplay();

#define SCREEN_ADDR (0x3C << 1)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_PAGE_SIZE 8
#define DEBUGPRINTS

// the bitmap in memory for the OLED pixels
// you must specify the type and the size, but keep the name the same
uint8_t oledDisplayFrameBuffer[SCREEN_HEIGHT / SCREEN_PAGE_SIZE][SCREEN_WIDTH] = {0};


int16_t scrollOffset = 0;
uint8_t OLEDinit = 0;
uint8_t changeToPage = 0b00000000; // each bit indicates if that page has been changed since the last screen update


// use this to access the buffer rather than accessing it directly as it will provide different results and this is correct
// value should either be 1 or 0
void setValueInBuffer(uint8_t x, uint8_t y, uint8_t value) {
    // write a value to the buffer at an x, y coord
    // value uses the least significant bit as the value written to that position
    
    // correct to the correct scrolled location
    int16_t correctedX = (x + scrollOffset);
    
    uint8_t correctedY = y / SCREEN_PAGE_SIZE;    
    uint8_t bitYValue = y % SCREEN_PAGE_SIZE;

    // if gone off the left side of the screen
    if (correctedX < 0) {
        // wrap to the right side
        correctedX += SCREEN_WIDTH;
    }

    // otherwise if we are off the left side
    if (correctedX > SCREEN_WIDTH) {
        // wrap round to the left side
        correctedX -= SCREEN_WIDTH;
    }
    // serial.printf("Setting %d, %d to %d\r\n", correctedX, correctedY, (value & 1) << bitYValue);
    oledDisplayFrameBuffer[correctedY][correctedX] &= ~(1 << bitYValue); // clear the value
    oledDisplayFrameBuffer[correctedY][correctedX] |= ((value & 1) << bitYValue); // and set the new value 
}

// use this to access the buffer rather than accessing it directly as it will provide different results and this is correct
uint8_t readPageFromBuffer(uint8_t x, uint8_t y) {
    // correct to the correct scrolled location
    // serial.printf("Rading with offset: %d\r\n", scrollOffset);
    int16_t correctedX = (x + scrollOffset);
    
    uint16_t correctedY = y / SCREEN_PAGE_SIZE;    
    
    // if gone off the left side of the screen
    if (correctedX < 0) {
        // wrap to the right side
        correctedX += SCREEN_WIDTH;
    }
    
    // otherwise if we are off the left side
    if (correctedX > SCREEN_WIDTH) {
        // wrap round to the left side
        correctedX -= SCREEN_WIDTH;
    }
    // serial.printf("Read from %d, %d\r\n", correctedX, correctedY);
    return oledDisplayFrameBuffer[correctedY][correctedX];
}

// use this to access the buffer rather than accessing it directly as it will provide different results and this is correct
// value will either be 0 or 1
uint8_t readValueFromBuffer(uint8_t x, uint8_t y) {
    uint8_t bitYValue = y % SCREEN_PAGE_SIZE;
    
    // return the value from the buffer
    return readPageFromBuffer(x, y) & (1 << bitYValue);
}

// got to specified x on display
void setColumnAddr(uint8_t startColumn, uint8_t endColumn) {
    
    // serial.printf("Set Column to %d - %d\r\n", startColumn, endColumn);
    // columnAddr
    uint8_t columnAddrData[6] = {0x80, 0x21, 0x80, startColumn, 0x80, endColumn};
    i2cExt.write(SCREEN_ADDR, columnAddrData, 6);
}



// go to specified y on display
// startpage / endpage take ranges of 0-7
void setPageAddr(uint8_t startPage, uint8_t endPage) {
    // serial.printf("Page vals before: %d - %d\r\n", startPage, endPage);
    startPage &= (0b111); // remove the top 5 bits as not needed
    endPage &= (0b111);
    // serial.printf("Set Page to %d - %d\r\n", startPage, endPage);

    // pageAddr
    uint8_t pageAddrData[6] = {0x80, 0x22, 0x80, startPage, 0x80, endPage};
    i2cExt.write(SCREEN_ADDR, pageAddrData, 6);
}


// turn on the display - only works first time so can be called multiple times with no change
void initOledDisplay() {
    if (OLEDinit)
        return;
    serial.printf("Init Display\r\n");
    // set mux ratio

    uint8_t muxData[4] = {0x80, 0xA8, 0x80, 0x3F};
    i2cExt.write(SCREEN_ADDR, muxData, 4);

    // set display offset
    uint8_t displayOfsetData[4] = {0x80, 0xD3, 0x80, 0x00};
    i2cExt.write(SCREEN_ADDR, displayOfsetData, 4);

    // set display start line
    uint8_t displayStartLineData[2] = {0x80, 0x40};
    i2cExt.write(SCREEN_ADDR, displayStartLineData, 2);

    // segment remap
    uint8_t segmentRemapData[2] = {0x80, 0xA0}; // second value can be 0xA0 or 0xA1
    i2cExt.write(SCREEN_ADDR, segmentRemapData, 2);

    // COM output scan direction
    uint8_t COMoutputScanDirData[2] = {0x80, 0xC0}; // second value can be 0xC0 or 0xC8
    i2cExt.write(SCREEN_ADDR, COMoutputScanDirData, 2);

    // COM pins hardware config
    uint8_t COMpinHardwareConfigData[4] = {0x80, 0xDA, 0x80, 0x12};
    i2cExt.write(SCREEN_ADDR, COMpinHardwareConfigData, 4);

    // Contrast Control
    uint8_t constrastControlData[4] = {0x80, 0x81, 0x80, 0x7F};
    i2cExt.write(SCREEN_ADDR, constrastControlData, 4);

    // disable entire display on
    uint8_t disableDispOnData[2] = {0x80, 0xA4};
    i2cExt.write(SCREEN_ADDR, disableDispOnData, 2);

    // set osc freq
    uint8_t oscFreqData[4] = {0x80, 0xD5, 0x80, 0x80};
    i2cExt.write(SCREEN_ADDR, oscFreqData, 4);

    uint8_t chargePumpRegData[4] = {0x80, 0x8D, 0x80, 0x14};
    i2cExt.write(SCREEN_ADDR, chargePumpRegData, 4);

    // display on
    uint8_t displayOnData[2] = {0x80, 0xAF};
    i2cExt.write(SCREEN_ADDR, displayOnData, 2);
    
    
    // addressing mode
    uint8_t addrModeData[4] = {0x80, 0x20, 0x80, 0x00}; // horizonal addr mode - 0x01 for vertical addressing mode
    i2cExt.write(SCREEN_ADDR, addrModeData, 4);

    OLEDinit = 1;
    serial.printf("Setup Display\r\n");
    clearOledDisplay(); // then clear the display once set up
}

// prints to serial the value of the buffer
void outputBuffer() {
    serial.printf("Buffer\r\n");
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
            serial.printf("%d", readValueFromBuffer(x, y));
        }
        serial.printf("\r\n");
    }
}

// set a pixel in the buffer but dont update the screen - use this most of the time
void bufferSetPixel(uint8_t x, uint8_t y) {
    // update the buffer but dont refresh the display

    changeToPage |= (1 << y/SCREEN_PAGE_SIZE);
    // updateBuffer
    // oledDisplayFrameBuffer[y/SCREEN_PAGE_SIZE][x] |= (1 << (y % SCREEN_PAGE_SIZE)); // set the relevent pixel in the buffer

    
    setValueInBuffer(x, y, 1); // set the relevent pixel in the buffer
    
}

// clear a pixel in the buffer but dont update the screen - use most of the time
void bufferClearPixel(uint8_t x, uint8_t y) {
    // update the buffer but dont refresh the display

    // update buffer
    changeToPage |= (1 << y/SCREEN_PAGE_SIZE);
    setValueInBuffer(x, y, 0); // clear the pixel in the buffer
    // oledDisplayFrameBuffer[y/SCREEN_PAGE_SIZE][x] &= ~ (1 << (y % SCREEN_PAGE_SIZE)); // clear the 

}

// update the buffer and display -- only use for single updates as its slower
void setOledPixel(uint8_t x, uint8_t y) {
    // set the pixel
    initOledDisplay();
    bufferSetPixel(x, y);
    // and refresh the display
    updateDisplay();
}


// update the buffer and display -- only use for single updates as its slower
void clearOledPixel(uint8_t x, uint8_t y) {
    initOledDisplay();
    // clear the pixel
    bufferClearPixel(x, y);
    // refresh the display
    updateDisplay();
}

// update the display with all the new changes since the last update
void updateDisplay() {
    // serial.printf("Change to pages: %d\r\n", changeToPage);
    initOledDisplay();
    uint8_t line[1 + SCREEN_WIDTH];

    
    for (int y = 0; y < (SCREEN_HEIGHT/SCREEN_PAGE_SIZE); y++) {
        if ((changeToPage & (1 << y)) == 0)
            continue; // is this page hasnt changed since last time skip it

        setColumnAddr(0, SCREEN_WIDTH-1);
        setPageAddr(y, y);

        // memcpy(line+1, oledDisplayFrameBuffer[y], SCREEN_WIDTH); // Set as a page row to udate all at once
        
        for (int x = 0; x < SCREEN_WIDTH; x++)
            line[1 + x] = readPageFromBuffer(x,y * SCREEN_PAGE_SIZE); // oledDisplayFrameBuffer[y][x];
        

        line[0] = 0x40; // set the first value to be a command byte 

        i2cExt.write(SCREEN_ADDR, line, 1+SCREEN_WIDTH);
    }

    changeToPage = 0b00000000; // reset as all changes have been written

}


// draw an oled line using the passed in function to actually set the pixel
void funcOledLine(void setPixel(uint8_t, uint8_t),uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, bool updateScreen) {
    initOledDisplay();

    if (x_start > x_end) {
        uint8_t temp = x_end;
        // swap the start and end
        x_end = x_start;
        x_start = temp;

        temp = y_end;
        y_end = y_start;
        y_start = temp;
    }

    int8_t xLen = x_end - x_start;
    int8_t yLen = y_end - y_start;
    // if its a vertical line do something different 
    if (xLen == 0) {
        for (int y = 0; y < yLen; y++) {
            uint8_t xVal = x_start;
            uint8_t yVal = y + y_start;
            if (y > SCREEN_WIDTH || xVal < 0 || yVal > SCREEN_HEIGHT || yVal < 0) {
                // serial.printf("Skipping (%d, %d)\r\n", xVal, yVal);
                continue;
            }
            // serial.printf("Drawing at (%d, %d)\r\n", xVal, yVal);
            setPixel(xVal, yVal);
        } 
    }
    // otherwise its fine
    else {
        // this is so that you always draw in a positive direction

        int m = ((yLen * 100) / xLen);
        uint8_t c = y_start;

        // y = mx + c
        // y/mx = c
        
        serial.printf("m: %d - c: %d\r\n", m, c);

        for (int x = 0; x < xLen; x++) {
            uint8_t xVal = x + x_start;
            uint8_t yVal = ((m * (int)(xVal)) / 100) + c;
            if (xVal > SCREEN_WIDTH || xVal < 0 || yVal > SCREEN_HEIGHT || yVal < 0) {
                // serial.printf("Skipping (%d, %d)\r\n", xVal, yVal);
                continue;
            }
            // serial.printf("Drawing at (%d, %d)\r\n", xVal, yVal);
            setPixel(xVal, yVal);
        } 
    }
    if (updateScreen)
        updateDisplay();
}

// set the line in the buffer and optionally update the screen after drawing
void setOledLine(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, bool updateScreen) {
    // serial.printf("Drawing Line\r\n");
    // serial.printf("Drawing (%d, %d) to (%d, %d)\r\n", x_start, y_start, x_end, y_end);
    funcOledLine(bufferSetPixel, x_start, y_start, x_end, y_end, updateScreen);
}

// clear the line in the buffer and optionalyl clear the screen after
void clearOledLine(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end, bool updateScreen) {
    // serial.printf("Clearing Line\r\n");
    // serial.printf("Clearing (%d, %d) to (%d, %d)\r\n", x_start, y_start, x_end, y_end);
    funcOledLine(bufferClearPixel, x_start, y_start, x_end, y_end, updateScreen);
    
}

// wrapped for setOledLine to use the correct args
void drawOledLine(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end) {
    setOledLine(x_start, y_start, x_end, y_end, true);
}

// sets the entire display to one value
void setOledDisplay(uint8_t val) {
    initOledDisplay();

    memset(oledDisplayFrameBuffer, val, SCREEN_HEIGHT * SCREEN_WIDTH / SCREEN_PAGE_SIZE);
    uint8_t lineClearData[1 + SCREEN_WIDTH];
    memset(lineClearData, 0xFF, (1 + SCREEN_WIDTH)); // set the values to all 0 to clear the page row
    lineClearData[0] = 0x40; // set the first value to be a command byte 
    
    for (int y = 0; y < (SCREEN_HEIGHT/SCREEN_PAGE_SIZE); y++) {
        setColumnAddr(0, SCREEN_WIDTH-1);
        setPageAddr(y, y);

        i2cExt.write(SCREEN_ADDR, lineClearData, 1+SCREEN_WIDTH);
    }
}

// set the entire display to 1 / on
void fillOledDisplay() {
    initOledDisplay();
    serial.printf("Filling Display\r\n");

    setOledDisplay(0xFF);

}


// set the entire display to 0 / off
void clearOledDisplay() {
    initOledDisplay();
    serial.printf("Clearing Display\r\n");

    setOledDisplay(0x00);
}

//###################################################################################
// Write your additional code for Subtask 2 below here (and leave these separators in your code!)
// put functions, global variables, consts, #DEFINEs etc. that you need for Subtask 2 here
//

#define ACC_ADDR 51
uint8_t accSetup = 0;
uint8_t lineNum = SCREEN_WIDTH;
uint8_t scrolling = 0;
uint8_t prevVal = 0;
#define MAX_VAL (511)
#define MIN_VAL (-512)
// turn on the accelerometer
void setupAcc() {
    if (accSetup)
        return;
    
    //          Turn on acc at 400 Hz  Turn on x, y, z axis
    //                             ||| ||| 
    //                             VVV VVV    
    static uint8_t ACC_ON_VAL = 0b01110111;
    uint8_t data[2] = {LSM303_CTRL_REG1_A, ACC_ON_VAL};
    i2cInt.write(ACC_ADDR, data, 2); // turn on the accelerometer to 400hz
    accSetup = 1;
}

void outputBinary(uint8_t binary) {
    for (int i = 0; i < 8; i++)
    serial.printf("%d", (((1 << (7-i)) & binary) != 0));
}


int16_t getData(char axis) {
    setupAcc();
    uint8_t lowerAddr;;
    uint8_t upperAddr;;

    // set the correct registers to read from for each axis
    switch (axis) {
        case 'x':
            lowerAddr = LSM303_OUT_X_L_A;
            upperAddr = LSM303_OUT_X_H_A;
            break;
        case 'y':
            lowerAddr = LSM303_OUT_Y_L_A;
            upperAddr = LSM303_OUT_Y_H_A;
            break;

        case 'z':
            lowerAddr = LSM303_OUT_Z_L_A;
            upperAddr = LSM303_OUT_Z_H_A;
            break;

        default:
            return INT16_MAX;
    }

    
    uint8_t lowerVal = 0;
    uint8_t upperVal = 0;

    while (1) {
        uint8_t data;
        // check the whoami register to make sure its working
        i2cInt.readRegister(ACC_ADDR, LSM303_WHO_AM_I_A, &data,1);
        // serial.printf("Who Am I: %d\r\n", data);
        if (data == 51) {
            break;
        }        
    }
    // serial.printf("Out of WHOAMI\r\n");

    // read the upper and lower vals of the accelerometer
    i2cInt.readRegister(ACC_ADDR, lowerAddr, &lowerVal, 1);
    i2cInt.readRegister(ACC_ADDR, upperAddr, &upperVal, 1);

    // serial.printf("Lower: "); outputBinary(lowerVal);
    // serial.printf(" Upper: "); outputBinary(upperVal);
    // serial.printf("\r\n");
    // combine the two vals into one 16 bit number
    int16_t combinedVal = lowerVal | (upperVal << 8);
    combinedVal >>= 6; // as in 10 bit resolution mode, fixes to correct value
    // serial.printf("Val: ");
    // outputBinary(combinedVal);
    // serial.printf("\r\n");
    // thanks to https://stackoverflow.com/questions/49467573/convert-twos-complement-left-justified-integer-into-regular-binary
    return combinedVal;
}


void scrollDisplay(bool update) {
    scrollOffset--;
    if (scrollOffset > SCREEN_WIDTH) {
        scrollOffset -= SCREEN_WIDTH;
    }
    if (scrollOffset < 0) {
        scrollOffset += SCREEN_WIDTH;
    }
    changeToPage = 0xFF;

    // serial.printf("ScrollOffset: %d\r\n", scrollOffset);
    if (update)
        updateDisplay();
}

void plotLine(int16_t value) {
    if (scrolling) {
        scrollDisplay(false);
        // do screen scrolling
        // activateScroll();
        // wait
        // deativeateScroll();
        scrolling = 0;
    }
    // get percent of way through for y coord
    

    
    if (!scrolling) {
        int val = value - MIN_VAL; // get it pos -- 0 to 1024
        val *= SCREEN_HEIGHT;
        val /= (MAX_VAL - MIN_VAL);

        // serial.printf("Drawing line num %d to %d\r\n", lineNum, val);
        clearOledLine(lineNum, 0, lineNum, SCREEN_HEIGHT, false);
        setOledLine(lineNum, 0, lineNum, val, true);
    }
    if (lineNum == 0) {
        scrolling = 1;
    }
    else 
        lineNum--;
    // if (lineNum > SCREEN_WIDTH)
    //     lineNum = 0;
}

enum mode {
    acceleration,
    jerk
};
static uint8_t setupButtons = 0;
static uint8_t lastPressed = 0; 
//
void graphData(uint8_t refreshRate){
    // setup scrolling for task 2
    // uint8_t scrollSetup[14] = {
    //     0x80, 0x27, // right horizoneal scroll
    //     0x80, 0x00, // dummy byte
    //     0x80, 0x00, // page 0 as start page
    //     0x80, 0b000, // int = 5 frames
    //     0x80, 0x07, // page 7 as end page
    //     0x80, 0x00, // dummy byte
    //     0x80, 0xFF // dummy byte -- page 28
    // };
    // i2cExt.write(SCREEN_ADDR, scrollSetup, 14);

    if (!setupButtons) {
        // setup button a as an input
        NRF_P0->PIN_CNF[MICROBIT_PIN_BUTTON_A] = 
                    (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                    (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                    (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
                    (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
                    (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    
        // setup button b as an input
        NRF_P0->PIN_CNF[MICROBIT_PIN_BUTTON_B] = 
                    (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                    (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                    (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
                    (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
                    (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
        setupButtons = 1;
    }

    enum mode mode = acceleration; // 1 for accleration mode, 0 for jerk mode
    uint16_t prevVal = 0;
    while (1)
    {
        // BUTTON A THINGS
        if (NRF_P0->IN & (1 << MICROBIT_PIN_BUTTON_A)) {
           // button A is not pressed
           lastPressed &= ~(1 << 0); // clear that bit
        }
        else {
            // button A is pressed
            // was button A pressed lasttime?
            if (lastPressed & (1 << 0)) {
                // yes - dont do anything
            }
            else {
                // no - new press
                mode = acceleration;
                clearOledDisplay();
                lastPressed |= (1 << 0); // set that bit
                lineNum = SCREEN_WIDTH;
            }
        }

        // BUTTON B THINGS
        if (NRF_P0->IN & (1 << MICROBIT_PIN_BUTTON_B)) {
           // button A is not pressed
           lastPressed &= ~(1 << 1); // clear that bit
        }
        else {
            // button B is pressed
            // was button B pressed lasttime?
            if (lastPressed & (1 << 1)) {
                // yes - dont do anything
            }
            else {
                mode = jerk;
                prevVal = 0;
                clearOledDisplay();
                lastPressed |= (1 << 1); // set that bit
                lineNum = SCREEN_WIDTH;
            }
        }
      
        // serial.printf("[X:%d][Y:%d][Z:%d]\r\n",getData('x'),getData('y'), getData('z'));
        if (mode == acceleration) {
            // acceleration mode
            plotLine(getData('x'));
        } else if (mode == jerk) {
            uint16_t thisVal = getData('x');
            int16_t valDif = thisVal - prevVal; // gives values from 1023 to -1024 (12 bits) need to be 511 - -512
            int16_t normalised = valDif / 2 ;
            serial.printf("ValDif: %d -- %d\r\n", valDif, normalised);
            plotLine(normalised);
            prevVal = thisVal;
        }

    }
}
