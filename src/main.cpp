#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <logo.h>
#include <TimerOne.h>
#include <Adafruit_MAX31865.h>

// LCD
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// dimmer
volatile int i = 0;              // Variable to use as a counter of dimming steps. It is volatile since it is passed between interrupts
volatile int j = 0;              // Variable to use as a counter of power level
volatile boolean zero_cross = 0; // Flag to indicate we have crossed zero
int AC_pin = 3;                  // Output to Opto Triac
int buttonDown = A0;             // first button at pin A0
int buttonUp = A1;               // second button at pin A1
int dim2 = 0;                    // led control
int dim = 128;                   // Dimming level (0-128)  0 = on, 128 = 0ff
int pas = 11;                    // step for count;
int freqStep = 75;               // This is the delay-per-brightness step in microseconds. It allows for 128 steps

void zero_cross_detect();
void dim_check();

// fan
const int RunButton = 4;
const int En = 7;
int RunButtonState;

// max31865

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13);

// The value of the Rref resistor. Use 430.0 for PT100
#define RREF 430.0

// The 'nominal' 0-degrees-C resistance of the sensor, 100.0 for PT100
#define RNOMINAL 100.0

void setup()
{
  // fan
  pinMode(RunButton, INPUT);
  pinMode(En, OUTPUT);

  // dimmer
  pinMode(buttonDown, INPUT);                    // set buttonDown pin as input
  pinMode(buttonUp, INPUT);                      // set buttonUp pin as input
  pinMode(AC_pin, OUTPUT);                       // Set the Triac pin as output
  attachInterrupt(0, zero_cross_detect, RISING); // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                   // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);   // Go to dim_check procedure every 75 uS (50Hz)
                                                 // Use the TimerOne Library to attach an interrupt
  Serial.begin(9600);
  SPSR |= (1 << SPI2X); // 2x SPI speed

  // max31865
  thermo.begin(MAX31865_4WIRE); // set to 2WIRE or 3WIRE as necessary

  // LCD
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  // Display logo
  tft.drawRGBBitmap(
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

  delay(1000);
  tft.fillScreen(ILI9341_BLACK);

  // Display pt100 temp

  /* uint16_t rtd = thermo.readRTD();
   tft.print("RTD value: ");
   tft.println(rtd);
   float ratio = rtd;
   ratio /= 32768;
   tft.print("Ratio = ");
   tft.println(ratio, 8);
   tft.print("Resistance = ");
   tft.println(RREF*ratio, 8);*/
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Temperature = ");
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println(thermo.temperature(RNOMINAL, RREF));

  // Check and print any faults
  /*uint8_t fault = thermo.readFault();
  if (fault)
  {
      tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
    tft.print("Fault 0x");
    tft.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH)
    {
      tft.println("RTD High Threshold");
    }
    if (fault & MAX31865_FAULT_LOWTHRESH)
    {
      tft.println("RTD Low Threshold");
    }
    if (fault & MAX31865_FAULT_REFINLOW)
    {
      tft.println("REFIN- > 0.85 x Bias");
    }
    if (fault & MAX31865_FAULT_REFINHIGH)
    {
      tft.println("REFIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_RTDINLOW)
    {
      tft.println("RTDIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_OVUV)
    {
      tft.println("Under/Over voltage");
    }
    thermo.clearFault();
  }*/
  tft.println();
  delay(1000);
}

// dimmer
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
      digitalWrite(AC_pin, HIGH); // turn on heater
      i = 0;                      // reset time step counter
      zero_cross = false;         // reset zero cross detection flag
    }
    else
    {
      i++; // increment time step counter
    }
  }
}

void loop()
{
  // dimmer
  digitalWrite(buttonDown, HIGH);
  digitalWrite(buttonUp, HIGH);

  if (digitalRead(buttonDown) == LOW)
  {
    if (dim < 127)
    {
      tft.fillScreen(ILI9341_BLACK);

      dim = dim + pas;
      j -= 10;
      tft.setCursor(0, 75);
      tft.println(j);
      if (dim > 127)
      {
        dim = 128;
      }
    }
  }
  if (digitalRead(buttonUp) == LOW)
  {
    if (dim > 7)
    {
      tft.fillScreen(ILI9341_BLACK);
      dim = dim - pas;
      j += 10;
      tft.setCursor(0, 75);
      tft.println(j);
      if (dim < 0)
      {
        dim = 0;
      }
    }
  }
  while (digitalRead(buttonDown) == LOW)
  {
  }
  delay(10);
  while (digitalRead(buttonUp) == LOW)
  {
  }
  delay(10);

  dim2 = 255 - 2 * dim;
  if (dim2 < 0)
  {
    dim2 = 0;
  }

  // fan
  RunButtonState = digitalRead(RunButton);
  if (RunButtonState == HIGH)
  {
    digitalWrite(En, LOW);
  }

  if (RunButtonState == LOW)
  {
    tft.println("cooling");
    dim = 128;
    delay(30000);
    digitalWrite(En, HIGH);
    delay(300000);
  }
}