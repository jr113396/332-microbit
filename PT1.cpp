#include "MicroBit.h"
extern NRF52Serial serial;   // serial object may be useful for debugging 

//###################################################################################
// Write your code for Subtask 1 here (and leave these separators in your code!):

//

uint8_t setupDispForBin = 0; // has the display been setup for binary counting?

void displayBinary(uint8_t value) {
    serial.printf("Displaying value: %d\r\n", value);
    uint32_t pins[] = { 
        MICROBIT_PIN_COL5, 
        MICROBIT_PIN_COL4, 
        MICROBIT_PIN_COL3, 
        MICROBIT_PIN_COL2, 
        MICROBIT_PIN_COL1
    }; // a mapping of significant bit to led, if left shifted 0 then index pos 0 for light
    
    NRF_GPIO_Type *groups[] = { 
        NRF_P0,
        NRF_P1                
    };
  
    if (!setupDispForBin) {
        setupDispForBin = 1;
    
        NRF_P0->DIR = (1 << MICROBIT_PIN_ROW1) | // make ROW1 and all COL Outputs
                      (1 << MICROBIT_PIN_COL1) |
                      (1 << MICROBIT_PIN_COL2) |
                      (1 << MICROBIT_PIN_COL3) | 
                      (1 << MICROBIT_PIN_COL5);
        
        NRF_P1->DIR = (1 << MICROBIT_PIN_COL4 % 32);

        // make sure that the row is on 
        NRF_P0->OUTSET = (1 << MICROBIT_PIN_ROW1);
    }


    for (int bitPos = 0; bitPos < 5; bitPos++) { 
        NRF_GPIO_Type *group;  // get the right group -- for column 4
        if (pins[bitPos] > 31)
            group = groups[1]; // the pin number will be > 32 if on the other group so move to that one
        else
            group = groups[0]; // otherwise its on the first group an its fine

        if ((value & (1 << bitPos)) ) { // if the bit being looked at is a 1
            // turn the corresponding light on
            group->OUTCLR = (1 << (pins[bitPos] % 32));
        } else {
            // otherwise turn it off
            group->OUTSET = (1 << (pins[bitPos] % 32));
        }
    }

}


void wait() {
    // this is a 200ms sleep
    uint32_t numTicks = 1820442;//0x000FFFFF;
    volatile uint32_t ticks;
    for (uint32_t i = 0; i < numTicks; i++)
        ticks++;
}


void waitMS(uint32_t delayMS) {
    uint32_t numTicksPer100ms = 910221; // ticks per 100ms
    
    uint32_t numTicks = numTicksPer100ms * delayMS / 100;

    volatile uint32_t ticks;

    for (uint32_t i = 0; i < numTicks; i++)
        ticks++;
}

void smallWait() {
    uint32_t numTicks = 0x0000FFFF * 2;
    volatile uint32_t ticks;
    for (uint32_t i = 0; i < numTicks; i++)
        ticks++;

}
//
void countUpBinary(uint8_t initialValue) {
    serial.printf(" ... Subtask 1 running ... \n"); 
    //while (1);  // NB this function never returns
    uint8_t val = initialValue;
    uint8_t maxVal = 31;
    uint8_t minVal = 0;
   

    while (1) {
        val++;
        // wrap around values if they get too high/low
        if (val > maxVal)
            val = minVal;
        if (val < minVal)
            val = maxVal;
        
        displayBinary(val);
        waitMS(200);
    }
}

//###################################################################################
// Write your additional code for Subtask 2 here:

//
void countWithButtonsBinary(uint8_t initialValue) {
    serial.printf(" ... Subtask 2 running ... \n"); 

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


    uint8_t lastPressed = 0; // XXXX XXBA
                             // 1 if button A/B was pressed last time
                             // used to remove repeated pressed when held
                             //
    uint8_t val = 0,
            maxVal = 31,
            minVal = 0;

    while (1) {

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
                lastPressed |= (1 << 0); // set the bit so you know it was pressed lastime
                // are you already at the min value
                if (val <= minVal) {
                    // yes
                    val = maxVal; // wrap back around
                } else {
                    // no
                    // go lower
                    val--;
                }
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
                // no - new press
                lastPressed |= (1 << 1); // set the bit so you know it was pressed lastime

                val++;
                if (val > maxVal) {
                    val = minVal; // wrap back around
                }
            }
        }

        // update the display
        displayBinary(val);

        smallWait();
   }

}

//###################################################################################
// Write your additional code for Subtask 3 here:

//
uint8_t sampleVoltage() {
}

//
void displayVoltageBinary() {
    serial.printf(" ... Subtask 3 running ... \n"); 

    // **********ADC setup************
    // need to setup samplerate, resolution, result maxcnt
    // cnfig, pselp, seln for channel
    NRF_SAADC->CH[0].CONFIG = ( 0b00 << 0) | // pos channel bypass resistor
                              ( 0b01 << 4) | // neg channel pull down resistor
                              (0b010 << 8) | // 1/4 gain
                              (  0b1 <<12) | // vdd/4 as reference
                              (0b101 <<16) | // 40 us time
                              (  0b0 <<24);  // no burst mode
    NRF_SAADC->CH[0].PSELP = 1; // set to analogue input 0
    NRF_SAADC->CH[0].PSELN = 0; // set neg channel to not connected
    NRF_SAADC->RESOLUTION = 0; // 8 bit resolution
    NRF_SAADC->SAMPLERATE = 0b11111111111; // 2047
    NRF_SAADC->RESULT.MAXCNT = 1;
    // Channel 0 - p0.02
    // Pos channel
    // Pull down on negative
    // 1/4 gain 
    // sample rate of 2047
    // 8 bit resolution
    // max num convertsion 1

    // set up the memory location to store the result into
    static volatile uint32_t AdcResult;
    NRF_SAADC->RESULT.PTR = (uint32_t) &AdcResult;

    // and enable the ADC

    // NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->ENABLE = 1;
    // get voltage
    // work out where normalised value is 0-31
    // display corresponding value
    // wait
    // repeat
    uint32_t value;
    while (1) {
        // get a sample
        NRF_SAADC->TASKS_START = 1;
        NRF_SAADC->TASKS_SAMPLE = 1;
        value = AdcResult;
        
        // if the value is greater than 256 then it is shorting with ground eg 0
        if (value > 256) {
            // so make it 0 
            value = 0;
        }
        uint32_t usedVal = value/8; // to make between 0 - 31 to map to the LEDs

        // display that value
        displayBinary(usedVal); 
        
    
        serial.printf("Got value: %d\r\n", (uint32_t)value);
        serial.printf("Using value: %d\r\n", (uint32_t)usedVal);
        // wait 50ms then loop
        waitMS(50);
    }// NB this function never returns
}

//###################################################################################
