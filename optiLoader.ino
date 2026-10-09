// optiLoader.pde
//
// this sketch allows an Arduino to program Optiboot onto any other
// Arduino-like device containing ATmega8, ATmega168, or ATmega328
// microcontroller chips.
//
// Copyright (c) 2011, 2015 by Bill Westfield ("WestfW")

//-------------------------------------------------------------------------------------
// "MIT Open Source Software License":
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in the
// Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------------------------

//
// this sketch allows an Arduino to program Optiboot onto any other
// Arduino-like device containing ATmega8, ATmega168, or ATmega328
// microcontroller chips.
//
// It is based on AVRISP
//
// Designed to connect to a generic programming cable,
// using the following pins:
// 4: slave reset
// 11: MOSI
// 12: MISO
// 13: SCK
//  9: Power to external chip.
//     This is a little questionable, since the power it is legal to draw
//     from a AVR pin is pretty close to the power consumption of an AVR
//     chip being programmed.  But it permits the target to be entirely
//     powered down for safe reconnection of the programmer to additional
//     targets, and it seems to work for most Arduinos.  If the target board
//     contains additional circuitry and is expected to draw more than 40mA,
//     connect the target power to a stronger source of +5V.  Do not use pin
//     9 to power more complex Arduino boards that draw more than 40mA, such
//     as the Arduino Uno Ethernet !
//
// If the aim is to reprogram the bootloader in one Arduino using another
// Arudino as the programmer, you can just use jumpers between the connectors
// on the Arduino board.  In this case, connect:
// Pin 13 to Pin 13
// Pin 12 to Pin 12
// Pin 11 to Pin 11
// Pin 4 (of "programmer") to RESET (of "target" (on the "power" connector))
// +5V to +5V and GND to GND.  Only the "programmer" board should be powered
//     by USB or external power.
//
// ----------------------------------------------------------------------

// The following credits are from AVRISP.  It turns out that there isn't
// a lot of AVRISP left in this sketch, but probably if AVRISP had never
// existed,  this sketch would not have been written.
//
// - Added support for the read signature command
//
// February 2009 by Randall Bohn
// - Added support for writing to EEPROM (what took so long?)
// Windows users should consider WinAVR's avrdude instead of the
// avrdude included with Arduino software.
//
// January 2008 by Randall Bohn
// - Thanks to Amplificar for helping me with the STK500 protocol
// - The AVRISP/STK500 (mk I) protocol is used in the arduino bootloader
// - The SPI functions herein were developed for the AVR910_ARD programmer



#include <avr/pgmspace.h>
#include "optiLoader.h"

char Arduino_preprocessor_hint;

/*
   Pins to target
*/
#define SCK 13
#define MISO 12
#define MOSI 11
#define RESET 2 //#define RESET 4
#define POWER 9

// STK Definitions; we can still use these as return codes
#define STK_OK 0x10
#define STK_FAILED 0x11


// Useful message printing definitions
#define fp(string) flashprint(PSTR(string));
#define debug(string) // flashprint(PSTR(string));
#define error(string) flashprint(PSTR(string));

// Forward references
void pulse(int pin, int times);
void read_image(const image_t *ip);

// Global Variables

/*
   Table of defined images
*/
const image_t * images[] = {
//  &image_328, &image_328p, &image_168, &image_8, 0
    &image_328p, 0
};

/*
   Table of "Aliases."  Chips that are effectively the same as chips
   that we have a bootloader for.  These work by simply overriding the
   signature read with the signature of the chip we "know."
*/
const alias_t aliases[] = {
  { "ATmega328PB", 0x9516, 0x950F },  /* Treat 328PB same as 328P */
};

int pmode = 0;
// address for reading and writing, set by 'U' command
int here;

uint16_t target_type = 0;   /* type of target_cpu */
uint16_t target_startaddr;
uint8_t target_pagesize;       /* Page size for flash programming (bytes) */
uint8_t *buff;

const image_t *target_flashptr;          /* pointer to target info in flash */
uint8_t target_code[512];        /* The whole code */

void setup (void) {
  Serial.begin(115200);       /* Initialize serial for status msgs */
  pinMode(13, OUTPUT);      /* Blink the pin13 LED a few times */
#ifdef PIN_SPI_SS
#if PIN_SPI_SS != RESET
  // on non-AtmegaXX8 boards (MEGA, Leonardo, etc) the SPI SS pin is NOT pin 10,
  // and although SS is driven manually (RESET to the target), SS should not be
  // allowed to default to INPUT and floating, or the SPI hardware may decide that
  // some other SPI master is trying to use the bus.  (See Datasheet SS description.)
  pinMode(PIN_SPI_SS, INPUT_PULLUP);
#endif
#endif
  pulse(13, 20);

  
  /*TONE START
 */
 delay(500);
 BEEP(1600, 500);  // play tone 1600Hz in 500ms
}

/*
 * Buzzer function 
 * Pass in the number of calls and the duration of each call in ms
 * Buzzer sounds with frequency 1600
 */
//void BEEP(int T, int TIME)
//{
//  for(int i = 0; i < T; i++)
//  {
//    tone(11, 1600);
//    delay(TIME);
//    noTone(11);
//    delay(TIME);
//  }
//}

void BEEP(unsigned int freq, int tOn)
{
  tone(11, freq);
  delay(tOn);
  noTone(11);
}

void SuccessStatus(void){
  digitalWrite(13, HIGH); // on led
  BEEP(100, 100);         // play tone 100Hz in 100ms

  // no tone and off led for 900ms
  digitalWrite(13, LOW);
  delay(900);
}

void FailStatus(void){
  digitalWrite(13, HIGH); // on led
  BEEP(400, 100);         // play tone 400Hz in 100ms

  // no tone and off led for 400ms
  digitalWrite(13, LOW);
  delay(400);
}

void loop(void) {
  fp("\nOptiLoader Bootstrap programmer.\n2011 by Bill Westfield (WestfW)\n\n");

  bool isSuccess = true;

  if (target_poweron()) {
    do {
      if (!target_identify()) {
        fp("\nERROR: Unable to identify target MCU.");
        isSuccess = false;
        break;
      }

      if (!target_findimage()) {
        fp("\nERROR: No matching firmware image found.");
        isSuccess = false;
        break;
      }

      if(!chip_erase())
        break;

      if (!target_progfuses()) {
        fp("\nERROR: Failed to program bootloader fuses.");
        isSuccess = false;
        break;
      }

      if (!target_program()) {
        fp("\nERROR: Flash programming or verification failed.");
        isSuccess = false;
        break;
      }

      if (!target_normfuses()) {
        fp("\nERROR: Failed to restore normal fuses.");
        isSuccess = false;
        break;
      }

    } while (0);
  } else {
    fp("\nERROR: Unable to power target.");
    isSuccess = false;
  }

  target_poweroff();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  if (isSuccess) {
    fp("\n*** Programming completed successfully. ***\n");
    BEEP(500, 700); // play 500Hz in 700ms
  } else {
    fp("\n*** Programming FAILED! ***\n");
    fp("Check wiring, target power, fuse settings, or firmware image.\n");
  }

//  fp("\nType 'G' or hit RESET for next chip\n");
  fp("\nHit RESET for next chip\n");

  while (1) {
//    if (Serial.read() == 'G')
//      break;
    if(isSuccess) 
      SuccessStatus();
    else     
      FailStatus();
  }
}

/*
   Low level support functions
*/

/*
   flashprint
   print a text string direct from flash memory to Serial
*/
void flashprint (const char p[])
{
  uint8_t c;
  while (0 != (c = pgm_read_byte(p++))) {
    Serial.write(c);
  }
}

/*
   hexton
   Turn a Hex digit (0..9, A..F) into the equivalent binary value (0-16)
*/
uint8_t hexton (uint8_t h)
{
  if (h >= '0' && h <= '9')
    return (h - '0');
  if (h >= 'A' && h <= 'F')
    return ((h - 'A') + 10);
  error("Bad hex digit!");
  return (0);
}

/*
   pulse
   turn a pin on and off a few times; indicates life via LED
*/
#define PTIME 30
void pulse (int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  }
  while (times--);
}

/*
   spi_init
   initialize the AVR SPI peripheral
*/
void spi_init (void) {
  uint8_t x;
  SPCR = 0x53;  // SPIE | MSTR | SPR1 | SPR0
  x = SPSR;
  x = SPDR;
}

/*
   spi_wait
   wait for SPI transfer to complete
*/
void spi_wait (void) {
  debug("spi_wait");
  do {
  }
  while (!(SPSR & (1 << SPIF)));
}

/*
   spi_send
   send a byte via SPI, wait for the transfer.
*/
uint8_t spi_send (uint8_t b) {
  uint8_t reply;
  SPDR = b;
  spi_wait();
  reply = SPDR;
  return reply;
}


/*
   Functions specific to ISP programming of an AVR
*/

/*
   target_identify
   read the signature bytes (if possible) and check whether it's
   a legal value (atmega8, atmega168, atmega328)
*/

boolean target_identify ()
{
  boolean result;
  target_type = 0;
  fp("\nReading signature:");
  target_type = read_signature();
  if (target_type == 0 || target_type == 0xFFFF) {
    fp(" Bad value: ");
    result = false;
  }
  else {
    result = true;
  }
  Serial.println(target_type, HEX);
  if (target_type == 0) {
    fp("  (no target attached?)\n");
  }
  return result;
}

unsigned long spi_transaction (uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint8_t n, m;
  spi_send(a);
  n = spi_send(b);
  //if (n != a) error = -1;
  m = spi_send(c);
  return 0xFFFFFF & ((((uint32_t)n) << 16) + (m << 8) + spi_send(d));
}

uint16_t start_pmode () {
  uint16_t result;

  pinMode(13, INPUT); // restore to default
  spi_init();
  debug("...spi_init done");
  // following delays may not work on all targets...
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  debug("...spi_transaction");
  result = spi_transaction(0xAC, 0x53, 0x00, 0x00);
  debug("...Done");
  pmode = 1;
  return result;
}

void end_pmode (void) {
  SPCR = 0;         /* reset SPI */
  digitalWrite(MISO, 0);    /* Make sure pullups are off too */
  pinMode(MISO, INPUT);
  digitalWrite(MOSI, 0);
  pinMode(MOSI, INPUT);
  digitalWrite(SCK, 0);
  pinMode(SCK, INPUT);
  digitalWrite(RESET, 0);
  pinMode(RESET, INPUT);
  pmode = 0;
}

/*
   read_image

   Read an intel hex image from a string in pgm memory.
   We assume that the image does not exceed the 512 bytes that we have
   allowed for it to have.  that would be bad.
   Also read other data from the image, such as fuse and protecttion byte
   values during programming, and for after we're done.
*/
void read_image (const image_t *ip)
{
  uint16_t len, totlen = 0, addr;
  const char *hextext = &ip->image_hexcode[0];
  target_startaddr = 0;
  target_pagesize = pgm_read_byte(&ip->image_pagesize);
  uint8_t b, cksum = 0;

  while (1) {
    if (pgm_read_byte(hextext++) != ':') {
      error("No colon");
      break;
    }
    len = hexton(pgm_read_byte(hextext++));
    len = (len << 4) + hexton(pgm_read_byte(hextext++));
    cksum = len;

    b = hexton(pgm_read_byte(hextext++)); /* address byte */
    b = (b << 4) + hexton(pgm_read_byte(hextext++));
    cksum += b;
    addr = b;
    b = hexton(pgm_read_byte(hextext++)); /* address byte */
    b = (b << 4) + hexton(pgm_read_byte(hextext++));
    cksum += b;
    addr = (addr << 8) + b;
    if (target_startaddr == 0) {
      target_startaddr = addr;
      fp("  Start address at ");
      Serial.println(addr, HEX);
    }
    else if (addr == 0) {
      break;
    }

    b = hexton(pgm_read_byte(hextext++)); /* record type */
    b = (b << 4) + hexton(pgm_read_byte(hextext++));
    cksum += b;

    for (uint8_t i = 0; i < len; i++) {
      b = hexton(pgm_read_byte(hextext++));
      b = (b << 4) + hexton(pgm_read_byte(hextext++));
      if (addr - target_startaddr >= sizeof(target_code)) {
        error("Code extends beyond allowed range");
        break;
      }
      target_code[addr++ - target_startaddr] = b;
      cksum += b;
#if VERBOSE
      Serial.print(b, HEX);
      Serial.write(' ');
#endif
      totlen++;
      if (totlen >= sizeof(target_code)) {
        error("Too much code");
        break;
      }
    }
    b = hexton(pgm_read_byte(hextext++)); /* checksum */
    b = (b << 4) + hexton(pgm_read_byte(hextext++));
    cksum += b;
    if (cksum != 0) {
      error("Bad checksum: ");
      Serial.print(cksum, HEX);
    }
    if (pgm_read_byte(hextext++) != '\n') {
      error("No end of line");
      break;
    }
#if VERBOSE
    Serial.println();
#endif
  }
  fp("  Total bytes read: ");
  Serial.println(totlen);
}

/*
   target_findimage

   given target_type loaded with the relevant part of the device signature,
   search the hex images that we have programmed in flash, looking for one
   that matches.
*/

boolean target_findimage ()
{
  const image_t *ip;
  fp("Searching for image...\n");
  /*
     Search through our table of chip aliases first
  */
  for (uint8_t i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
    const alias_t *a = &aliases[i];
    if (a->real_chipsig == target_type) {
      fp("  Compatible bootloader for ");
      Serial.println(a->alias_chipname);
      target_type = a->alias_chipsig;  /* Overwrite chip signature */
      break;
    }
  }
  /*
     Search through our table of self-contained images.
  */
  for (uint8_t i = 0; i < sizeof(images) / sizeof(images[0]); i++) {
    target_flashptr = ip = images[i];
    if (ip && (pgm_read_word(&ip->image_chipsig) == target_type)) {
      fp("  Found \"");
      flashprint(&ip->image_name[0]);
      fp("\" for ");
      flashprint(&ip->image_chipname[0]);
      fp("\n");
      read_image(ip);
      return true;
    }
  }
  fp(" Not Found\n");
  return (false);
}


boolean chip_erase(){
  fp("\nChip Erase\n");
  spi_transaction(0xAC, 0x80, 0x00, 0x00);  /* chip erase */
  delay(1000);
  return true;
}

/*
   target_progfuses
   given initialized target image data, re-program the fuses to allow
   the optiboot image to be programmed.
*/

boolean target_progfuses ()
{
  uint8_t f;
  fp("\nSetting fuses for programming");

  f = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_PROT]);
  if (f) {
    fp("\n  Lock: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xE0, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_LOW]);
  if (f) {
    fp("  Low: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA0, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_HIGH]);
  if (f) {
    fp("  High: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA8, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_EXT]);
  if (f) {
    fp("  Ext: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA4, 0x00, f), HEX);
    delay(20);
  }
  Serial.println();
//  return true;
  return verify_progfuses();      /* */
}

/*
   target_program
   Actually program the image into the target chip
*/

boolean target_program ()
{
  int l;        /* actual length */

  fp("\nProgramming bootloader: ");
  here = target_startaddr >> 1;     /* word address */
  buff = target_code;
  l = 512;
  Serial.print(l, DEC);
  fp(" bytes at 0x");
  Serial.println(here, HEX);

//  fp("\nChip Erase\n");
//  spi_transaction(0xAC, 0x80, 0x00, 0x00);  /* chip erase */
//  delay(1000);
  if (write_flash(l) != STK_OK) {          // write page
    error("\nFlash Write Failed");
    return false;
  }

  fp("\nFlash Write Success");

  here = target_startaddr >> 1;   // <-- reset before verify

  if(verify_flash_page(l) != STK_OK){      // verify page
    error("\nFlash Verify Failed");
    return false;
  }

  fp("\nFlash Verify Success");
  
  return true;      /*  */
}

/*
   target_normfuses
   reprogram the fuses to the state they should be in for bootloader
   based programming
*/
boolean target_normfuses ()
{
  uint8_t f;
  fp("\nRestoring normal fuses");

  f = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_PROT]);
  if (f) {
    fp("\n  Lock: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xE0, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_LOW]);
  if (f) {
    fp("  Low: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA0, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_HIGH]);
  if (f) {
    fp("  High: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA8, 0x00, f), HEX);
    delay(20);
  }
  f = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_EXT]);
  if (f) {
    fp("  Ext: ");
    Serial.print(f, HEX);
    fp(" ");
    Serial.print(spi_transaction(0xAC, 0xA4, 0x00, f), HEX);
    delay(20);
  }
  Serial.println();
//  return true;      /* */
  return verify_normfuses();
}

/*
   target_poweron
   Turn on power to the target chip (assuming that it is powered through
   the relevant IO pin of THIS arduino.)
*/
boolean target_poweron ()
{
  uint16_t result;

  fp("Target power on! ...");
  digitalWrite(POWER, LOW);
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER, HIGH);
  digitalWrite(RESET, LOW);  // reset it right away.
  pinMode(RESET, OUTPUT);
  /*
     Check if the target is pulling RESET HIGH by reverting to input
  */
  delay(5);
  pinMode(RESET, INPUT);
  delay(1);
  if (digitalRead(RESET) != HIGH) {
    fp("No RESET pullup detected! - no target?");
    return false;
  }
  pinMode(RESET, OUTPUT);

  delay(200);
  fp("\nStarting Program Mode");
  result = start_pmode();
  if ((result & 0xFF00) != 0x5300) {
    fp(" - Failed, result = 0x");
    Serial.print(result, HEX);
    return false;
  }
  fp(" [OK]\n");
  return true;
}

boolean target_poweroff ()
{
  end_pmode();
  digitalWrite(POWER, LOW);
  delay(200);
  pinMode(POWER, INPUT);
  fp("\nTarget power OFF!\n");
  return true;
}

/*---------------------------------prog flash -----------------------------------------*/

void flash (uint8_t hilo, int addr, uint8_t data) {
#if VERBOSE
  Serial.print(data, HEX);
  fp(":");
  Serial.print(spi_transaction(0x40 + 8 * hilo,
                               addr >> 8 & 0xFF,
                               addr & 0xFF,
                               data), HEX);
  fp(" ");
#else
  (void) spi_transaction(0x40 + 8 * hilo,
                         addr >> 8 & 0xFF,
                         addr & 0xFF,
                         data);
#endif
}

void commit (int addr) {
  fp("  Commit Page: ");
  Serial.print(addr, HEX);
  fp(":");
  Serial.println(spi_transaction(0x4C, (addr >> 8) & 0xFF, addr & 0xFF, 0), HEX);
  delay(100);
}

//#define _current_page(x) (here & 0xFFFFE0)
int current_page (int addr) {
  if (target_pagesize == 32) return here & 0xFFFFFFF0;
  if (target_pagesize == 64) return here & 0xFFFFFFE0;
  if (target_pagesize == 128) return here & 0xFFFFFFC0;
  return here;
}

uint8_t write_flash (int length) {
  if (target_pagesize < 1) return STK_FAILED;
  //if (target_pagesize != 64) return STK_FAILED;
  int page = current_page(here);
  int x = 0;
  while (x < length) {
    if (page != current_page(here)) {
      commit(page);
      page = current_page(here);
    }
    flash(LOW, here, buff[x]);
    flash(HIGH, here, buff[x + 1]);
    x += 2;
    here++;
  }

  commit(page);

  return STK_OK;
}

/*---------------------------------read/verify flash -----------------------------------------*/

uint8_t flash_read(uint8_t hilo, unsigned int addr) {
  return spi_transaction(0x20 + hilo * 8,
                         (addr >> 8) & 0xFF,
                         addr & 0xFF,
                         0);
}

//char flash_read_page(int length) {
//  for (int x = 0; x < length; x += 2) {
//    uint8_t low = flash_read(LOW, here);
//    SERIAL.print((char) low);
//    uint8_t high = flash_read(HIGH, here);
//    SERIAL.print((char) high);
//    here++;
//  }
//  return STK_OK;
//}

char verify_flash_page(int length) {
  for (int x = 0; x < length; x += 2) {
    uint8_t low = flash_read(LOW, here);
    
    #if VERBOSE
     Serial.print(F("[0x"));
     Serial.print(here, HEX);
     Serial.print(F("] LOW  Flash="));
     Serial.print(low, HEX);
     Serial.print(F(" Buff="));
     Serial.println(buff[x], HEX);
    #endif
    
    if(low !=  buff[x])return STK_FAILED;
    
    uint8_t high = flash_read(HIGH, here);

    #if VERBOSE
     Serial.print(F("[0x"));
     Serial.print(here, HEX);
     Serial.print(F("] HIGH  Flash="));
     Serial.print(high, HEX);
     Serial.print(F(" Buff="));
     Serial.println(buff[x + 1], HEX);
    #endif

    if(high !=  buff[x+1])return STK_FAILED;
     
    here++;
  }
  delay(100);
  return STK_OK;
}

/*---------------------------------read/verify fuse -----------------------------------------*/

boolean verify_progfuses() {
  uint8_t expect, readf;

  fp("\nVerify prog fuses");

  // Lock bits
  expect = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_PROT]);
  if (expect) {
    readf = spi_transaction(0x58, 0x00, 0x00, 0x00);

    fp("\n Lock : expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    readf &= 0x3F; // remove reserve bit
    fp(" =>(remove reserve=0x");
    Serial.print(readf, HEX);
    fp(" )");
    
    if (readf  != expect)
      return false;
  }

  // Low Fuse
  expect = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_LOW]);
  if (expect) {
    readf = spi_transaction(0x50, 0x00, 0x00, 0x00);

    fp("\n LFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    if (readf  != expect)
      return false;
  }

  // High Fuse
  expect = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_HIGH]);
  if (expect) {
    readf = spi_transaction(0x58, 0x08, 0x00, 0x00);

    fp("\n HFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    if (readf  != expect)
      return false;
  }

  // Extended Fuse
  expect = pgm_read_byte(&target_flashptr->image_progfuses[FUSE_EXT]);
  if (expect) {
    readf = spi_transaction(0x50, 0x08, 0x00, 0x00);

    fp("\n EFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    readf &= 0x07; // remove reserve bit

    fp(" =>(remove reserve=0x");
    Serial.print(readf, HEX);
    fp(" )");

    if (readf  != expect)
      return false;
  }

  Serial.println();
  return true;
}

boolean verify_normfuses() {
  uint8_t expect, readf;

  fp("\nVerify normal fuses");

  // Lock bits
  expect = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_PROT]);
  if (expect) {
    readf = spi_transaction(0x58, 0x00, 0x00, 0x00);

    fp("\n Lock : expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    readf &= 0x3F; // remove reserve bit

    fp(" =>(remove reserve=0x");
    Serial.print(readf, HEX);
    fp(" )");

    if (readf  != expect)
      return false;
  }

  // Low Fuse
  expect = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_LOW]);
  if (expect) {
    readf = spi_transaction(0x50, 0x00, 0x00, 0x00);

    fp("\n LFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    if (readf  != expect)
      return false;
  }

  // High Fuse
  expect = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_HIGH]);
  if (expect) {
    readf = spi_transaction(0x58, 0x08, 0x00, 0x00);

    fp("\n HFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    if (readf  != expect)
      return false;
  }

  // Extended Fuse
  expect = pgm_read_byte(&target_flashptr->image_normfuses[FUSE_EXT]);
  if (expect) {
    readf = spi_transaction(0x50, 0x08, 0x00, 0x00);

    fp("\n EFuse: expect=0x");
    Serial.print(expect, HEX);
    fp(" read=0x");
    Serial.print(readf, HEX);

    readf &= 0x07; // remove reserve bit

    fp(" =>(remove reserve=0x");
    Serial.print(readf, HEX);
    fp(" )");
    
    if (readf  != expect)
      return false;
  }

  Serial.println();
  return true;
}

uint16_t read_signature () {
  uint8_t sig_middle = spi_transaction(0x30, 0x00, 0x01, 0x00);
  uint8_t sig_low = spi_transaction(0x30, 0x00, 0x02, 0x00);
  return ((sig_middle << 8) + sig_low);
}

/*
   Bootload images.
   These are the intel Hex files produced by the optiboot makefile,
   with a small amount of automatic editing to turn them into C strings,
   and a header attched to identify them

   Emacs keyboard macro:

      4*SPC     ;; self-insert-command
      "     ;; self-insert-command
      C-e     ;; move-end-of-line
      \     ;; self-insert-command
      n"      ;; self-insert-command * 2
      C-n     ;; next-line
      C-a     ;; move-beginning-of-line

*/

const image_t PROGMEM image_328p = {
  {
    "optiboot_atmega328_2.hex"
  }
  ,
  {
    "atmega328P"
  }
  ,
  0x950F,       /* Signature bytes for 328P */
  { 
    0x3F,       /* Lock Bits: \
                   BLB0 = 11 = BLB0 Mode 1 : Bootloader được phép đọc (LPM) và ghi (SPM) vùng Application. \
                   BLB1 = 11 = BLB1 Mode 1 : Bootloader được phép đọc (LPM) và ghi (SPM) vùng Bootloader. \
                   LB   = 11 = LB Mode 1 : Không khóa bộ nhớ; ISP vẫn đọc/ghi/xóa Flash & EEPROM và thay đổi Fuse. */
    0xFF,       /* Low Fuse: \
                   CKDIV8=1 (không chia clock /8), 
                   CKOUT=1 (không xuất clock output), \
                   SUT=11 Start-up time: 16K CK + (14CK (atmega328P) | 19CK (atmega328pb)) + 65 ms, 
                   CKSEL=1111 (thạch anh ngoài 8–16 MHz, low power). */
    0xDE,       /* High Fuse: \
                   BOOTRST=0 (khởi động từ Bootloader), 
                   BOOTSZ=11 (256 words = 512 B), \
                   SPIEN=0 (cho phép ISP), 
                   WDTON=1 (Watchdog không luôn bật, cấu hình bằng phần mềm), \
                   EESAVE=1 (EEPROM bị xóa khi Chip Erase), 
                   DWEN=1 (debugWIRE tắt), 
                   RESET=1 (chân RESET hoạt động). */
    0x05,       /* Extended Fuse: \
                   BODLEVEL=101 (Brown-out Detection = 2.7 V min: 2.5V, max: 2.9V), 
                   (Riêng với atmega328pb): CFD = 0 = disabled. */
    0
  }
  ,
  {
    0x2F,        /* Lock Bits: \
                   BLB0 = 11 = BLB0 Mode 1 : Bootloader được phép đọc (LPM) và ghi (SPM) vùng Application. \
                   BLB1 = 01 = BLB1 Mode 2 : Cấm SPM ghi lên Bootloader (write-protect), vẫn cho phép LPM đọc. \
                   LB   = 11 = LB Mode 1 : Không khóa bộ nhớ; ISP vẫn đọc/ghi/xóa Flash & EEPROM và thay đổi Fuse. */
    0,           
    0, 
    0, 
    0
  }
  ,
  128,
  {
    ":107E000001C00895112484B7882361F0982F9A70D7\n"
    ":107E1000923041F081FF02C097EF94BF282E80E09E\n"
    ":107E2000C2D0EEC085E08093810082E08093C000E4\n"
    ":107E300088E18093C10086E08093C20080E1809356\n"
    ":107E4000C4008EE0B0D0259A86E020E33CEF91E0BC\n"
    ":107E5000309385002093840096BBB09BFECF1D9A83\n"
    ":107E6000A8954091C00047FD02C0815089F793E07A\n"
    ":107E7000E92EDD24D39425E0C22E31E1B32E87D044\n"
    ":107E8000813459F484D0182F94D083E0113809F448\n"
    ":107E900088E076D080E174D0F2CF823419F484E1A6\n"
    ":107EA00090D0F8CF853411F485E0FACF853541F4D0\n"
    ":107EB0006ED0C82F6CD0D82FCC0FDD1F7AD0EACF70\n"
    ":107EC000863521F484E07DD080E0E3CF843609F06C\n"
    ":107ED00034C05DD05CD0F82E5AD0A82E00E011E05E\n"
    ":107EE00048018FEF881A980A52D0F80180838401E4\n"
    ":107EF000F810F6CF5ED0F5E4AF1201C0FFCFFE015F\n"
    ":107F0000E7BEE89507B600FCFDCFFE01A0E0B1E0BA\n"
    ":107F1000CD0102962D913C910901D7BEE89511241F\n"
    ":107F20003296DC01F812F4CFFE01C7BEE89507B621\n"
    ":107F300000FCFDCFB7BEE895ADCF843791F427D0D4\n"
    ":107F400026D0F82E24D0182F34D0153409F4FFCFC2\n"
    ":107F50008E01F80185918F0113D0FA94F110F9CFB9\n"
    ":107F600099CF853739F425D08EE10AD085E908D03C\n"
    ":107F70008FE08FCF813509F0A1CF88E014D09ECF5C\n"
    ":107F80009091C00095FFFCCF8093C600089580912A\n"
    ":107F9000C00087FFFCCF8091C00084FD01C0A89580\n"
    ":107FA0008091C6000895E0E6F0E098E19083808338\n"
    ":107FB0000895EDDF803219F088E0F5DFFFCF84E12E\n"
    ":107FC000DFCFCF93C82FE3DFC150E9F7CF91F1CFD7\n"
    ":027FFE00030876\n"
    ":0400000300007E007B\n"
    ":00000001FF\n"
  }
};
