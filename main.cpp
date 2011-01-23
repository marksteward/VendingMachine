#include "mbed.h"

DigitalOut g(p22);
DigitalOut r(p21);

void red(void) {
    r = 1; g = 0;
}

void green(void) {
    r = 0; g = 1;
}

void orange(void) {
    r = 1; g = 1;
}

void off(void) {
    r = 0; g = 0;
}

Serial pc(USBTX, USBRX);

DigitalOut ser(p25);
DigitalOut mclock(p24);
DigitalOut oe(p26);
DigitalOut m10(p28);
DigitalOut m9(p27);

InterruptIn vint(p30);

int intrcount = 0;
bool vendfound = 0;

/*
motor connectors pinouts:

        a       b
1       a1      b1
2       a2      b2
3       a3      b3
4       a4      b4
5       a5      b5
6       a6      b6
7       a7      n/a
8       a8      n/a
9       a9      n/a
10      a0      n/a
*/

/* bits are:
 *
 * serial data mcu -> 5890 -> 5842A
 *
 * first bits out are for the first 8 motor sinks
 * next 8 are the sources, we only need 2
 *
 * low order bits are the sinks
 *
 */ 
uint16_t motor_bits(int motor)
{
        uint16_t out = 0; // 16 bits!

        switch (motor) {
        case 1:
                out = 0x2080; // 0010000010000000
                break;
        case 2:
                out = 0x2040; // 0010000001000000
                break;
        case 3:
                out = 0x2020; // 0010000000100000
                break;
        case 4:
                out = 0x2010; // 0010000000010000
                break;
        case 5:
                out = 0x2008; // 0010000000001000
                break;
        case 6:
                out = 0x2004; // 0010000000000100
                break;
        case 7:
                out = 0x2002; // 0010000000000010
                break;
        case 8:
                out = 0x2001; // 0010000000000001
                break;
        case 9:
                out = 0x2000; // 0010000000000000 uses the darlingtons
                break;
        case 10:
                out = 0x2000; // 0010000000000000 uses the darlingtons
                break;
        case 11:
                out = 0x4080; // 0100000010000000
                break;
        case 12:
                out = 0x4040; // 0100000001000000
                break;
        case 13:
                out = 0x4020; // 0100000000100000
                break;
        case 14:
                out = 0x4010; // 0100000000010000
                break;
        case 15:
                out = 0x4008; // 0100000000001000
                break;
        case 16:
                out = 0x4004; // 0100000000000100
                break;
        }
        return out;
}

/*
 * motor is 1 to 16
 */
int motors(int motor) {
    int i, bit;
    uint16_t bits;

    bits = 0;

    if (motor < 1)
        return -1;
        
    if (motor > 16)
        return -1;

    bits = motor_bits(motor);

    for (i = 0 ; i < 16 ; i++)
    {
        bit = (bits & (1 << i)) >> i;
        ser = bit;
        mclock = 0;
        wait_ms(1);
        mclock = 1;
        wait_ms(1);
    }

    // need to use the darlington pins instead
    if (motor == 9)
        m9 = 1;

    if (motor == 10)
        m10 = 1;

    return 0;
}

void clear(void) {
    int i;
    
    ser = 0;
    m9 = 0;
    m10 = 0;
    oe = 1; // oe is active low
    
    for (i = 0; i < 16 ; i++)
    {
        mclock = 0;
        wait_ms(1);
        mclock = 1;
    }
    wait_ms(1);
    m9 = 0;
    m10 = 0;
    oe = 0; // should switch everything off.
    intrcount = 0;
}

void vendintr() {
    intrcount++;
}

int main() {
    int i;
    char got;
    char state1, gcount;
    int ga, gb;
    
    state1 = 'n';
    gcount = 0;
    ga = gb = 0;

    oe = 1; // active low
    ser = 0;
    mclock = 0;

    printf("\n\rGo!\n\r");
    green();

    vint.mode(PullUp);
    vint.fall(&vendintr);

    for (i = 0; 1 ; i++) {
        if (i % 2) {
            orange();
        } else {
            green();
        }

        wait_ms(100);
        if (pc.readable())
        {
            got = pc.getc();
            pc.putc(got); // remote echo            
            if (state1 == 'n') // no command in progress
            {
                if (got == 'c') // no args
                {
                    clear();
                    printf("cleared\n\r");
                }
                if (got == 'r') { // no args, report intr count
                    printf("vendintrs: %d\n\r", intrcount);
                }
                if (got == 'm') // look for numbers
                    state1 = 'm';
            } else if (state1 == 'm') {
                if ((got - '0') > -1 && (got - '0') < 10 )
                {
                    if (gcount == 0)
                        ga = got - '0';
                    if (gcount == 1)
                        gb = got - '0';
                
                    gcount += 1;
                } else if (got == '\n' || got == '\r') {
                    // newline, so end command
                    int runmotor = 0;
                    if (gcount < 3 && gcount != 0) {
                        if (gcount == 1)
                            runmotor = ga;
                        if (gcount == 2)
                            runmotor = (ga * 10) + gb;
                        if (runmotor > 16) {
                            printf("motor too big: %d\n\r", runmotor);
                        } else {
                            printf("running motor %d\n\r", runmotor);
                            oe = 1;
                            motors(runmotor);
                            oe = 0; // active low
                        }
                    } else {
                        printf("too many digits\n\r");
                    }
                    state1 = 'n';
                    gcount = ga = gb = 0;
                } else {
                    printf("m fail\n\r");
                    state1 = 'n';
                    gcount = ga = gb = 0;
                }
            }            
        }
   }
   clear();
        
   printf("Done!\r\n");
}
