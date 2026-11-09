#include "MicroBit.h"
//###################################################################################
// Write your code for Subtask 1 here (and leave these separators in your code!):

void wait() {
    uint32_t numTicks = 8; // beween 8 and 9 ticks so with all this setup, probably 8 if that

    volatile uint32_t ticks;

    for (uint32_t i = 0; i < numTicks; i++)
        ticks++;
}


void largerWait() {
    uint32_t numTicks = 10000;
    volatile uint32_t ticks;

    for (uint32_t i = 0; i < numTicks; i++)
        ticks++;
}

// the number of ticks needed between each char
// 1/115200 * 16 * 10^6
#define TICKS_PER_CHAR (128) 
// the number of ticks needed between each transmission
// 0.240 * 16*10^6
#define TICKS_BETWEEN_TRANSMISSION (3840000) 
uint8_t pinSetup = 0;

void setLines(uint8_t data) {
    // pins should be P0.06 for interface chip
    // and edge connector p2 - p0.04

    if (!pinSetup) {
        NRF_P0->DIR = (1 << MICROBIT_PIN_P2 |
                       1 << MICROBIT_PIN_UART_TX);
        
        // set both to be high to start with
        NRF_P0->OUTSET = (1 << MICROBIT_PIN_P2 | 
                          1 << MICROBIT_PIN_UART_TX);
        
        pinSetup = 1;
        
        for (int i = 0; i < 100; i++)
            wait();
    }
    // if high
    if ((data & (1 << 0))) {
        // set the pin to high
        NRF_P0->OUTSET = (1 << MICROBIT_PIN_P2 |
                          1 << MICROBIT_PIN_UART_TX);
    
    } else {
        // set the pin to low
        NRF_P0->OUTCLR = (1 << MICROBIT_PIN_P2 |
                          1 << MICROBIT_PIN_UART_TX);
    }
}

void outByte(uint8_t c) {
    // go through every bit in that char to determine if its a 1 or 0

    // combine the byte entered with a start and stop bit
    uint16_t byteToSend = (c << 1) | (1 << 9) | (0 << 0);
    NRF_TIMER0->TASKS_CAPTURE[1]=1;

    NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
    NRF_TIMER0->CC[1] = 0;
    for (uint8_t bitPos = 0; bitPos < 10; bitPos++) {
        NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
        NRF_TIMER0->CC[1] = 0;
        if (byteToSend & (1 << bitPos)) {
            // that pos is a 1
            setLines(1);
        } else {
            // that pos is a 0
            setLines(0);
        }
        // wait until the timer happens
        while (NRF_TIMER0->CC[1] < TICKS_PER_CHAR) {
            NRF_TIMER0->TASKS_CAPTURE[1] = TIMER_TASKS_COUNT_TASKS_COUNT_Msk;
        }

    }
}

// This is an implementation of the bitbang serial 
// function with a different signature
// to allow me to use %d and %s 
// it works the exact same
void serialDebug(char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    char c;

    int prev = 0;

    while ((c =  *fmt++)) {
        if (c == '%') {
            // have they just typed a %
            if (prev) {
                // output an actual % char
                outByte('%');
                prev = 0;
                continue;
            }
            // otherwise this is a control char
            // so set prev to 1 to indicate that the next char is part of the control char
            prev = 1;
            // and go to the next char
            continue;
        }
        if (prev) {
            switch (c)
            {
            case 's':
                {
                    // set the prev value to 0 as used that %
                    prev = 0;
                    // get the string passed in
                    char *s = va_arg(a, char*);
                    // go through each char in the string and output that char
                    while (*s) {
                        outByte(*s++);
                    }
                }
                // go to the next character
                continue;
            

            case 'd':
                {
                    // make a buffer to write the chars into
                    char b[33];
                    // make a pointer to the start of the buffer to write into
                    char *s = b;
                    // reset the prev val as we have used that %
                    prev = 0;

                    // get the int that was passed in
                    int d = va_arg(a, int);

                    // convert the digit into a string placed into the b buffer
                    itoa(d, b);

                    // and print each character
                    while (*s) {
                        outByte(*s++);
                    }
                }
                // go to the next char
                continue;

            case 'b':
                // would usually use itoa but the microbit version doesnt allow base changing
                {
                    prev = 0;
                    int binary = va_arg(a, int);
                    for (int i = 0; i < 32; i++)
                        serialDebug("%d", (((1 << (31-i)) & binary) != 0));
                }
                continue;

            default:
                break;
            }
        }
        // otherwise if its just a normal char
        // output it
        outByte(c);
    }

    va_end(a);

} 

static uint8_t isClockSetup = 0;
void setupClock() {
    if (!isClockSetup) {

        // set scaler
        // 16MHz / 2 ^ pres
        // prescaler of 0 sets freq to 16 MHZ - 138.8 ticks between bit 
        NRF_TIMER0->PRESCALER = 0; // prescale of 0 - 2^0 - 16Mhz
        NRF_TIMER0 ->BITMODE = TIMER_BITMODE_BITMODE_32Bit; // 32 bit width
        NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer; // timer mode not counter mode
        NRF_TIMER0->CC[0] = TICKS_BETWEEN_TRANSMISSION; // ticks between transmissions
        NRF_TIMER0->CC[1] = TICKS_PER_CHAR; // this is the number of ticks between chars
        NRF_TIMER0->TASKS_START = 1; // start the timer
        isClockSetup = 1;
    }
}



//
void bitBangSerial(char *string) {
    // setup the clock if its not already
    if (!isClockSetup) {
        setupClock();
    }

    char *s = string;
    while (*s) {
        outByte(*s++);
        for (int i = 0; i < 10; i++) {
            wait();
        }
    }
}

//
void voteForChocolate(void) {
    // serial.printf("Starting\r\n");
    // bitBangSerial(" ... Subtask 1 running ... \n"); 

    while (1) {
        // bitBangSerial("UW");
        bitBangSerial("Freddo\r\n");

        // for (volatile int i = 0; i < 10000000; i++);

        // clear the timer before
        NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
        NRF_TIMER0->CC[0] = 0;
        // wait until the timer happens
        while (NRF_TIMER0->CC[0] < TICKS_BETWEEN_TRANSMISSION) {
            NRF_TIMER0->TASKS_CAPTURE[0] = 1;
        }
        // serialDebug("Waiting: %d\r\n", NRF_TIMER0->CC[0]);
        // clear the timer after
        NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
        NRF_TIMER0->CC[0] = 0;
    }
    while (1);  // NB this function never returns
}


//###################################################################################
// Write your additional code for Subtask 2 here:
#define ACC_ADDR (0x19)

static uint8_t accSetup = 0;


void readECode(uint32_t ecode){
    serialDebug("error recvd %d:\r\n", ecode);
    if (ecode & (1 << 0))
        serialDebug("- Overrun occured\r\n");
    
    if (ecode & (1 << 1))
        serialDebug("- Addr Nack Rcvd\r\n");

    if (ecode & (1 << 2))
        serialDebug("- First Byte Nack Rcvd\r\n");
}


void writeRegister(uint8_t addr, uint8_t regNum, uint8_t value) {
    // to write
    // --------
    // Start
    // addr + write bit
    // SubAddr
    // Data
    
    
    NRF_TWI0->ADDRESS = addr; // this is the addr of the component
    
    NRF_TWI0->TXD = regNum; // this is the sub address
    
    NRF_TWI0->EVENTS_TXDSENT = 0;

    // start the writing task
    // this will set the start bit
    // write the addr + writebit
    // and output the reg num in TXD
    NRF_TWI0->TASKS_STARTTX = 1;

    // wait until that transfer has completed
    while (!NRF_TWI0->EVENTS_TXDSENT) {
        if (NRF_TWI0->EVENTS_ERROR) {
            uint32_t ecode = NRF_TWI0->ERRORSRC;
            serialDebug("aaaaaaaaaaaaaaaaaaaaaA\r\n");
            readECode(ecode);

            NRF_TWI0->EVENTS_ERROR = 0;
            NRF_TWI0->TASKS_STOP = 1;

            largerWait();

            NRF_TWI0->TASKS_STARTTX = 1;
        }
    }
    NRF_TWI0->EVENTS_TXDSENT = 0;


    // now write the value into the register to be sent
    NRF_TWI0->TXD = value;

    // wait until that transfer has completed
    while (!NRF_TWI0->EVENTS_TXDSENT);
    NRF_TWI0->EVENTS_TXDSENT = 0;
    // and then stop the transmission
    NRF_TWI0->TASKS_STOP = 1;
    while (!NRF_TWI0->EVENTS_STOPPED);
    NRF_TWI0->EVENTS_STOPPED = 0;
}

uint8_t readRegister(uint8_t addr, uint8_t regNum) {
    // to read
    // -------
    // Start 
    // addr + write bit
    // write SubAddr
    // Start + read bit
    // Stop
    largerWait();
    uint8_t data = 0;

    // start + write the subaddr
    NRF_TWI0->ADDRESS = addr;
    NRF_TWI0->EVENTS_TXDSENT = 0;
    NRF_TWI0->TXD = regNum;
    NRF_TWI0->TASKS_STARTTX = 1;

    // wait until that has sent
    while (!NRF_TWI0->EVENTS_TXDSENT) {
        if (NRF_TWI0->EVENTS_ERROR) {
            uint32_t ecode =NRF_TWI0->ERRORSRC;
            
            readECode(ecode);

            NRF_TWI0->EVENTS_ERROR = 0;
            NRF_TWI0->TASKS_STOP = 1;

            largerWait();

            NRF_TWI0->TASKS_STARTTX = 1;
        }
    }
    NRF_TWI0->EVENTS_TXDSENT = 0;


    // then start reading data from that location
    NRF_TWI0->TASKS_STARTRX = 1;
    
    // wait until a byte has been recvd
    // for (int i = 0; i < 5; i++) {
    while (!NRF_TWI0->EVENTS_RXDREADY) {
        if (NRF_TWI0->EVENTS_ERROR) {
            uint32_t ecode = NRF_TWI0->ERRORSRC;
            readECode(ecode); 
        
            NRF_TWI0->EVENTS_ERROR = 0;
            NRF_TWI0->TASKS_STOP = 1;

            largerWait();

            NRF_TWI0->TASKS_STARTRX= 1;
        }
    }
    NRF_TWI0->EVENTS_RXDREADY = 0;
    
    NRF_TWI0->TASKS_STOP = 1;
    data =   NRF_TWI0->RXD;
    // }
    // for (int i = 0; i < 5; i++) {
        //     serialDebug("Vals: %d -> %d\r\n",i, vals[i]);
        // }
        
        // data = vals[0];
        // serialDebug("DATA: %d\r\n", data);
        
    while (!NRF_TWI0->EVENTS_STOPPED);
    NRF_TWI0->EVENTS_STOPPED = 0;
    

    largerWait();

    return data;
}

void setupAcc() {
    // if the accelerometer hasnt been setup yet, set it up
    if (!accSetup) {

        // NRF_P0->DIRSET = (1 << I2C_SCL0);
        // NRF_P0->DIRSET = (1 << I2C_SDA0);
        
        NRF_P0->PIN_CNF[MICROBIT_PIN_INT_SCL] = 
                (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos) |
                (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);

        NRF_P0->PIN_CNF[MICROBIT_PIN_INT_SDA] = 
                (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos) |
                (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);

        NRF_TWI0->PSEL.SCL = MICROBIT_PIN_INT_SCL; // I2C_SCL0; // enable the pins
        NRF_TWI0->PSEL.SDA = MICROBIT_PIN_INT_SDA; // I2C_SDA0; // and connect them
        // (0 << 31) sets the 31st bit to 0, connecting the pin

        // the acclerometer and twi interface both support 100 and 400 kbps so picking 100 for ease of timing
        NRF_TWI0->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K100; // 100 kbps
        
        // this should be 5 - see page 670
        NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Enabled;
        
        writeRegister(ACC_ADDR, LSM303_CTRL_REG1_A, 0b01110111); // turn on the accelerometer to 400hz
        accSetup = 1;
    }
    
    // need to turn it on
    // put in high res mode 
    // ctrl_reg_3 - 0
    // ctrl_reg_4 - 1
    
    // writeRegister(ACC_ADDR, LSM303_CTRL_REG3_A, 0);
    // writeRegister(ACC_ADDR, LSM303_CTRL_REG4_A, 1);

}



//
int getAccelerometerSample(char axis) {
    if (!accSetup)
        setupAcc();

    uint8_t lower = 0,
            higher = 0;
    uint16_t value = 0;

    switch (axis) {
        case 'x':
            lower = readRegister(ACC_ADDR, LSM303_OUT_X_L_A);
            higher = readRegister(ACC_ADDR, LSM303_OUT_X_H_A);
            break;
        
        case 'y':
            lower = readRegister(ACC_ADDR, LSM303_OUT_Y_L_A);
            higher = readRegister(ACC_ADDR, LSM303_OUT_Y_H_A);
            break;
        
        case 'z':

            lower = readRegister(ACC_ADDR, LSM303_OUT_Z_L_A);
            higher = readRegister(ACC_ADDR, LSM303_OUT_Z_H_A);
            break;
    }
    // serialDebug("Lower: %b - Higher: %b\r\n", lower, higher);
    // combine the two values into 1 large number
    value = lower | (higher << 8);
    // value >>= 4;
    #define RANGE 1024
    #define MAX_VAL 0xFFFF
    int normalised = ((value * RANGE)/ ((MAX_VAL))) - RANGE/2;
    return normalised;


}


//
void showAccelerometerSamples(void) {
    bitBangSerial(" ... Subtask 2 running ... \n"); 
    // while (1);  // NB this function never returns

    
    setupAcc();
    
    while (1) {
        // serialDebug("Starting readReg\r\n");
        // serialDebug("A %d\r\n", 10);
        // serialDebug("Got val: %d from WHO_AM_I reg\r\n", data);
        while (1) {
            uint8_t data = readRegister(ACC_ADDR, LSM303_WHO_AM_I_A);
            if (data == 51) {
                break;
            }        
        }

        int x, y, z;
        y = getAccelerometerSample('y');
        x = getAccelerometerSample('x');
        z = getAccelerometerSample('z');
        serialDebug("[X:%d][Y:%d][Z:%d]\r\n", x,y,z);
        // for (int i = 0; i < 32; i++)
          // clear the timer before
        NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
        NRF_TIMER0->CC[0] = 0;
        // wait until the timer happens
        while (NRF_TIMER0->CC[0] < TICKS_BETWEEN_TRANSMISSION) {
            NRF_TIMER0->TASKS_CAPTURE[0] = 1;
        }
        // serialDebug("Waiting: %d\r\n", NRF_TIMER0->CC[0]);
        // clear the timer after
        NRF_TIMER0->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk;
        NRF_TIMER0->CC[0] = 0;
    }

}

//###################################################################################
