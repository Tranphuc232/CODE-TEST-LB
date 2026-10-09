#define  T0STEPPER_TCCR0B  ( 0x03 )           //  WGM2 = 0, CS0 = 011 (clk / 64)

#define  ISCALE  8

#define  MotorA_DirPin    7
#define  MotorA_stepsPin  6

#define  MotorB_stepsPin  5
#define  MotorB_DirPin    4

void MotorA_begin() {
  TCCR0A &= ~( _BV(WGM01) | _BV(WGM00) );           // WGM0 = b000 : Normal, 0xFF mode for Timer0
  //TCCR0B &= ~  _BV(WGM02);
  TCCR0B  = T0STEPPER_TCCR0B;
  
  pinMode(MotorA_stepsPin, OUTPUT);
  pinMode(MotorA_DirPin,  OUTPUT);
  MotorA_setVelocity(0);
}

void MotorB_begin() {
  TCCR0A &= ~( _BV(WGM01) | _BV(WGM00) );           // WGM0 = b000 : Normal, 0xFF mode for Timer0
  //TCCR0B &= ~  _BV(WGM02);
  TCCR0B  = T0STEPPER_TCCR0B;
  
  pinMode(MotorB_stepsPin, OUTPUT);
  pinMode(MotorB_DirPin,  OUTPUT);
  MotorB_setVelocity(0);
}

#define  COM0A_MASK    ( 0b11 << COM0A0 )
#define  COM0A_TOGGLE  ( 0b01 << COM0A0 )           //  COM0A = 01 : Toggle OC0A on compare match
#define  COM0A_CLEAR   ( 0b10 << COM0A0 )           //  COM0A = 10 : Clear  OC0A on compare match

#define  COM0B_MASK    ( 0b11 << COM0B0 )
#define  COM0B_TOGGLE  ( 0b01 << COM0B0 )           //  COM0B = 01 : Toggle OC0B on compare match
#define  COM0B_CLEAR   ( 0b10 << COM0B0 )           //  COM0B = 10 : Clear  OC0B on compare match

volatile unsigned int MotorA_steps = 0;
volatile unsigned int MotorA_period = ISCALE * (256*2 + 1);

volatile unsigned int MotorB_steps = 0;
volatile unsigned int MotorB_period = ISCALE * (256*2 + 1);

int  MotorA_velocity = 0;
long MotorA_position = 0;

int  MotorB_velocity = 0;
long MotorB_position = 0;

void MotorA_setVelocity(int velocity) {
  if (velocity == 0) {
    TIMSK0 &= ~_BV(OCIE0A);                         // Disable Timer0 Output Compare Match A Interrupt
    TCCR0A  = (TCCR0A & ~COM0A_MASK) | COM0A_CLEAR;
    TCCR0B  = _BV(FOC0A) | T0STEPPER_TCCR0B;        // Force a compare match strobe
    MotorA_velocity = 0;
    digitalWrite(MotorA_stepsPin, HIGH);            // nENABLE = HIGH : disable A4988
    
  } else {
    int speed;
    
    if (velocity > 0) {
      speed = +velocity;
      if (MotorA_velocity <= 0) {
        digitalWrite(MotorA_DirPin, HIGH);
      }
    } else {
      speed = -velocity;
      if (MotorA_velocity >= 0) {
        digitalWrite(MotorA_DirPin, LOW);
      }
    }

    unsigned int period = (250000L * ISCALE + speed/2) / speed;
    MotorA_velocity = velocity;
    
    noInterrupts();
    MotorA_period = period;
    interrupts();
    TIMSK0 |=  _BV(OCIE0A);                         // Enable Timer0 Output Compare Match A Interrupt
  }
}


void MotorB_setVelocity(int velocity) {
  if (velocity == 0) {
    TIMSK0 &= ~_BV(OCIE0B);                         // Disable Timer0 Output Compare Match B Interrupt
    TCCR0A  = (TCCR0A & ~COM0B_MASK) | COM0B_CLEAR;
    TCCR0B  = _BV(FOC0B) | T0STEPPER_TCCR0B;        // Force a compare match strobe
    MotorB_velocity = 0;
    digitalWrite(MotorB_stepsPin, HIGH);            // nENABLE = HIGH : disable A4988
    
  } else {
    int speed;
    
    if (velocity > 0) {
      speed = +velocity;
      if (MotorB_velocity <= 0) {
        digitalWrite(MotorB_DirPin, HIGH);
      }
    } else {
      speed = -velocity;
      if (MotorB_velocity >= 0) {
        digitalWrite(MotorB_DirPin, LOW);
      }
    }
      
    unsigned int period = (250000L * ISCALE + speed/2) / speed;
    MotorB_velocity = velocity;
    
    noInterrupts();
    MotorB_period = period;
    interrupts();
    TIMSK0 |=  _BV(OCIE0B);                         // Enable Timer0 Output Compare Match A Interrupt
  }
}


ISR(TIMER0_COMPA_vect) {
  TCCR0B = _BV(FOC0A) | T0STEPPER_TCCR0B;     // Force a compare match strobe
  
  static unsigned int remain = 0;
  if (remain == 0) {                          // new step
    MotorA_steps++;
    
    unsigned int p = MotorA_period;
    remain = p / ISCALE;
    
    static char err = 0;
    if (err > 0) remain++;
    err += p - remain*ISCALE;
  }

  byte delta = remain;
  if (delta == 0) {
    remain -= 0x100;
  } else {
    delta   = max(delta, 20);                  // delta min limit, to avoid interrupt overrun
    remain -= delta;
    OCR0A  += delta;
  }
  
//  TCCR0B = _BV(FOC0A) | T0STEPPER_TCCR0B;     // Force a compare match strobe
  byte com0A = (remain > 0) ? COM0A_CLEAR : COM0A_TOGGLE;
  TCCR0A = (TCCR0A & ~COM0A_MASK) | com0A;
}

ISR(TIMER0_COMPB_vect) {
  TCCR0B = _BV(FOC0B) | T0STEPPER_TCCR0B;     // Force a compare match strobe
  
  static unsigned int remain = 0;
  if (remain == 0) {                          // new step
    MotorB_steps++;
    
    unsigned int p = MotorB_period;
    remain = p / ISCALE;
    
    static char err = 0;
    if (err > 0) remain++;
    err += p - remain*ISCALE;
  }

  byte delta = remain;
  if (delta == 0) {
    remain -= 0x100;
  } else {
    delta   = max(delta, 20);                  // delta min limit, to avoid interrupt overrun
    remain -= delta;
    OCR0B  += delta;
  }
  
//  TCCR0B = _BV(FOC0B) | T0STEPPER_TCCR0B;     // Force a compare match strobe
  byte com0B = (remain > 0) ? COM0B_CLEAR : COM0B_TOGGLE;
  TCCR0A = (TCCR0A & ~COM0B_MASK) | com0B;
}

void testV(int v, int tDelayMs) {
  MotorA_setVelocity(v);
  //delay(1);
  MotorB_setVelocity(v);
  Serial.println(v);
  delay(tDelayMs);
}

void setup() {
  Serial.begin(115200);

  MotorA_begin();
  MotorB_begin();

  testV(0, 500);
}


void loop() {
  const int vMax     = 2047;
  const int vStep    = 50;
  const int tDelayMs = 100;

  for (int v = 0; v >= -vMax; v -= vStep) {
    testV(v, tDelayMs);
  }
  delay(4600);
  
  for (int v = -vMax; v <= 0; v += vStep) {
    testV(v, tDelayMs);
  }
  delay(400);
  
  for (int v = 0; v <= vMax; v += vStep) {
    testV(v, tDelayMs);
  }
  delay(4600);
  
  for (int v = vMax; v >= 0; v -= vStep) {
    testV(v, tDelayMs);
  }
  delay(400);
  
}
