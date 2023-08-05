#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TimerOne.h>
#include <Adafruit_MAX31865.h>

// LCD
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// calibre
const int setButton = A2; // set button at pin A2
volatile float offset = 0.0;
bool set = false;

volatile int m = 0;
volatile int v = 0;
volatile float n = 0.0;
volatile float w = 0.0;

// Dimmer
volatile int i = 0;              // Variable to use as a counter of dimming steps. It is volatile since it is passed between interrupts
volatile int j = 0;              // Variable to use as a counter of power level
volatile boolean zero_cross = 0; // Flag to indicate we have crossed zero
int AC_pin = 3;                  // Output to Opto Triac
int buttonDown = A0;             // Down button at pin A0
int buttonUp = A1;               // Up button at pin A1
int dim2 = 0;                    // Heater control
int dim = 128;                   // Dimming level (0-128)  0 = on, 128 = 0ff
int pas = 11;                    // Step for count;
int freqStep = 75;               // This is the delay-per-brightness step in microseconds. It allows for 128 steps
void zero_cross_detect();
void dim_check();
bool powerchange = true;

// Fan
const int RunButton = 4;
const int En = 7;

// Max31865
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13);
#define RREF 430.0     // The value of the Rref resistor. Use 430.0 for PT100
#define RNOMINAL 100.0 // The 'nominal' 0-degrees-C resistance of the sensor, 100.0 for PT100

void setup()
{
  // Fan
  pinMode(RunButton, INPUT);
  pinMode(En, OUTPUT);

  // Dimmer
  pinMode(buttonDown, INPUT);                    // Set buttonDown pin as input
  pinMode(buttonUp, INPUT);                      // Set buttonUp pin as input
  pinMode(AC_pin, OUTPUT);                       // Set the Triac pin as output
  attachInterrupt(0, zero_cross_detect, RISING); // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                   // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);   // Go to dim_check procedure every 75 uS (50Hz)
                                                 // Use the TimerOne Library to attach an interrupt
  Serial.begin(9600);
  SPSR |= (1 << SPI2X); // 2x SPI speed

  // Max31865
  thermo.begin(MAX31865_4WIRE); // set to 2WIRE or 3WIRE as necessary

  // LCD
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  // Display logo
  /*tft.setRotation(45);
  tft.setCursor(50, 50);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("aram gostar");
  delay(1000);*/

  /*tft.drawRGBBitmap(
      100,
      100,
#if defined(__AVR__)
      logoBitmap,
#else
      // Some non-AVR MCU's have a "flat" memory model and don't
      // distinguish between flash and RAM addresses.  In this case,
      // the RAM-resident-optimized drawRGBBitmap in the ILI9341
      // library can be invoked by forcibly type-converting the
      // PROGMEM bitmap pointer to a non-const uint16_t *.
      (uint16_t *)logoBitmap,
#endif
      LOGO_WIDTH, LOGO_HEIGHT);
  delay(5000);*/

  // menu1
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(45);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Stop");
  tft.setCursor(50, 170);
  tft.setTextSize(3);
  tft.println("Power: 0/110");
}

// Dimmer
void zero_cross_detect()
{
  zero_cross = true; // set flag for dim_check function that a zero cross has occured
  i = 0;             // stepcounter to 0.... as we start a new cycle
  digitalWrite(AC_pin, LOW);
}
// Turn on the TRIAC at the appropriate time
// We arrive here every 75 uS
// First check if a flag has been set
// Then check if the counter 'i' has reached the dimming level
// if so.... switch on the TRIAC and reset the counter

void dim_check()
{
  if (zero_cross == true)
  {
    if (i >= dim)
    {
      digitalWrite(AC_pin, HIGH); // Turn on heater
      i = 0;                      // Reset time step counter
      zero_cross = false;         // Reset zero cross detection flag
    }
    else
    {
      i++; // Increment time step counter
    }
  }
}

void loop()
{

  digitalWrite(RunButton, HIGH);
  if (digitalRead(RunButton) == LOW)
  {
    tft.fillScreen(ILI9341_BLACK);
    set = false;
    powerchange = true;
    tft.setRotation(45);
    tft.setCursor(50, 170);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.print("Power: ");
    tft.print(j);
    tft.println("/110");
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.println("Run");
  }

  // Dimmer
  if (powerchange)
  {
    digitalWrite(buttonUp, HIGH);
    digitalWrite(buttonDown, HIGH);

    if (digitalRead(buttonUp) == LOW)
    {
      if (dim > 7)
      {
        tft.fillScreen(ILI9341_BLACK);
        dim = dim - pas;
        j += 10;
        tft.setRotation(45);
        tft.setCursor(50, 170);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(3);
        tft.print("Power: ");
        tft.print(j);
        tft.println("/110");
        tft.setCursor(10, 10);
        tft.setTextSize(2);
        tft.println("Run");
        if (dim < 0)
        {
          dim = 0;
        }
      }
    }

    if (digitalRead(buttonDown) == LOW)
    {
      if (dim < 127)
      {
        tft.fillScreen(ILI9341_BLACK);
        dim = dim + pas;
        j -= 10;
        tft.setRotation(45);
        tft.setCursor(50, 170);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(3);
        tft.print("Power: ");
        tft.print(j);
        tft.println("/110");
        tft.setCursor(10, 10);
        tft.setTextSize(2);
        tft.println("Run");
        if (dim > 127)
        {
          dim = 128;
        }
      }
    }

    dim2 = 255 - 2 * dim;
    if (dim2 < 0)
    {
      dim2 = 0;
    }

    // Display pt100 temp
    uint16_t rtd = thermo.readRTD();
    // Serial.print("RTD value: "); Serial.println(rtd);
    float ratio = rtd;
    ratio /= 32768;
    // Serial.print("Ratio = "); Serial.println(ratio,8);
    // tft.print("Resistance = "); tft.println(RREF*ratio,8);
    tft.setRotation(45);
    tft.setCursor(50, 70);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.print("Temp: ");
    float temp = thermo.temperature(RNOMINAL, RREF);
    tft.print(temp + offset);
    tft.print(" c");

    uint8_t fault = thermo.readFault();
    if (fault)
    {
      tft.setCursor(60, 110);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_RED);
      tft.print("Fault 0x");
      tft.println(fault, HEX);
      if (fault & MAX31865_FAULT_OVUV)
      {
        tft.setCursor(60, 130);
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(2);
        tft.println("Under/Over voltage");
      }
      thermo.clearFault();
      tft.println();
      delay(1);
    }
    tft.println();
    delay(500);

    // Fan

    digitalWrite(RunButton, HIGH);
    if (digitalRead(RunButton) == HIGH)
    {
      digitalWrite(En, LOW);
    }

    if (digitalRead(RunButton) == LOW)
    {
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(50, 170);
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_WHITE);
      tft.println("Power: 0/110");
      tft.setCursor(10, 10);
      tft.setTextSize(2);
      tft.println("Cooling");

      dim = 128;
      delay(1000);
      digitalWrite(En, HIGH);
      delay(3000);

      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(50, 170);
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_WHITE);
      tft.println("Power: 0/110");
      tft.setCursor(10, 10);
      tft.setTextSize(2);
      tft.println("Stop");
    }
  }


  // calibre

  digitalWrite(setButton, HIGH);
  if (digitalRead(setButton) == LOW)
  {
    powerchange = false;
    set = true;
    tft.fillScreen(ILI9341_BLACK);
  }

  if (set)
  {
    tft.setCursor(60, 80);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.println("Temp Range: 0-400 c");
    tft.setCursor(60, 130);
    tft.setTextSize(2);
    tft.println("Offset: ");
    //////////////////////////////////////////////////
  }
  // up

  /*digitalWrite(buttonUp, HIGH);
  if (digitalRead(buttonUp) == LOW)
  {
    m++;
    tft.setTextSize(2);
    tft.setCursor(90, 130);
    tft.print(m);
    digitalWrite(setButton, HIGH);
    if (digitalRead(setButton) == LOW)
    {
      v = m;

      digitalWrite(buttonUp, HIGH);
      if (digitalRead(buttonUp) == LOW)
      {
        n++;
        tft.setCursor(120, 130);
        tft.setTextSize(2);
        tft.print(":");
        tft.print(n);
      }
    }
    digitalWrite(setButton, HIGH);
    if (digitalRead(setButton) == LOW)
    {
      w = n;
      offset = v + w / 10;
    }
  }
  // down
  digitalWrite(buttonDown, HIGH);
  if (digitalRead(buttonDown) == LOW)
  {
    m--;
    tft.setTextSize(2);
    tft.setCursor(90, 130);

    tft.print(m);

    digitalWrite(setButton, HIGH);
    if (digitalRead(setButton) == LOW)
    {
      v = m;

      digitalWrite(buttonDown, HIGH);
      if (digitalRead(buttonDown) == LOW)
      {
        n--;
        tft.setCursor(120, 130);
        tft.setTextSize(2);
        tft.print(":");
        tft.print(n);
      }
    }
    digitalWrite(setButton, HIGH);
    if (digitalRead(setButton) == LOW)
    {
      w = n;
      offset = v + w / 10;
    }
  }*/

  ////////////////////////////////////////////////
}
