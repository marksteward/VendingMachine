/*
 * All Jasper's notes at http://pointless.net/hg/lhs-vending-machine/file/66115b60197e/notes
 */

#define MOTOR_TX (1)
#define MOTOR_CLK (2)
#define MOTOR_OE (3)
#define MOTOR_9 (4)
#define MOTOR_10 (5)
#define MOTOR_SENS (6)

void disable_motors() {
    digitalWrite(MOTOR_OE, HIGH);
}

void enable_motors() {
    digitalWrite(MOTOR_OE, LOW);
}

void set_motor_bits(uint16_t bits) {

    for (int i = 0; i < 16; i++) {
        digitalWrite(MOTOR_TX, bits & 1);

        digitalWrite(MOTOR_CLK, 0);
        delay(1);
        digitalWrite(MOTOR_CLK, 1);
        delay(1);

        bits >>= 1;
    }

}

void set_motor_9(int state) {
    digitalWrite(MOTOR_9, state);
}

void set_motor_10(int state) {
    digitalWrite(MOTOR_10, state);
}

int get_motor_sens() {
    return digitalRead(MOTOR_SENS);
}

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
void set_motor(int motor)
{
    set_motor_9(0);
    set_motor_10(0);
    // wait?

    int source = 0;
    int sink = 0;

    if (motor >= 1 && motor <= 8) {
        source = 0x20;
        sink = 1 << (8 - motor);

    } else if (motor == 9 || motor == 10) {
        source = 0x20;
        sink = 0x0; // use darlingtons

    } else if (motor >= 11 && motor <= 16) {
        source = 0x40;
        sink = 1 << (18 - motor);

    } else {
        source = 0;
        sink = 0;
    }

    set_motor_bits(source << 8 | sink);

    if (motor == 9)  set_motor_9(1);
    if (motor == 10) set_motor_10(1);
}


void setup() {

    pinMode(MOTOR_TX, OUTPUT);
    pinMode(MOTOR_CLK, OUTPUT);
    pinMode(MOTOR_OE, OUTPUT);
    pinMode(MOTOR_9, OUTPUT);
    pinMode(MOTOR_10, OUTPUT);
    pinMode(MOTOR_SENS, INPUT);

    disable_motors();
    set_motor(0);

}

int senshist[0x100];
int histindex = 0;

void loop() {

    while(Serial.available() > 0) {
        int motor = 0;
        int r = Serial.read();
        if (r >= '0' && r <= '9') {
            motor = r - '0' + 1;
        } else if (r <= 'a' && r <= 'f') {
            motor = r - 'a' + 11;
        }
        Serial.print("Running motor ");
        Serial.print(motor, DEC);
        Serial.println("");
        set_motor(motor);
    }

    int sample = get_motor_sens();
    senshist[histindex] = sample;
    histindex++;

    if (histindex == 0x100) {
        for(int i = 0; i < 0x100; i += 0x10) {
            for(int j = 0; j < 0x10; j++) {
                if(senshist[i + j])
                    Serial.write('1');
                else
                    Serial.write('0');
            }
            Serial.println("");
        }
        histindex = 0;
    }

    delay(100);
}
