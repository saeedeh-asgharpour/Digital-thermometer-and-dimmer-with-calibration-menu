#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TimerOne.h>
#include <Adafruit_MAX31865.h>
#include "logo.h"
// change detect
int buttonPushCounter = 0; // counter for the number of button presses
int buttonState = 0;       // current state of the button
int lastButtonState = 0;   // previous state of the button

// reset
unsigned long timeStart;
bool bCheckingSwitch = false;

// LCD
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// calibration
const int setButton = A2; // set button at pin A2
volatile float offset = 0.0;
bool set = false;
volatile int m = 0;
volatile int v = 0;
volatile int n = 0;
volatile float w = 0.0;
bool dahgan = false;
bool yekan = true;

volatile int tempRange = 0;
volatile int h = 0;

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

void menu1()
{
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
void menu2()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(50, 80);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Temp Range:");
  tft.setCursor(184, 80);
}

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
  tft.setRotation(45);
  tft.drawRGBBitmap(
      120,
      80,
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
  delay(5000);

  // menu1
  menu1();
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

  // Dimmer
  if (powerchange)
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

    if (temp > tempRange)
    {

      tft.print(temp + offset);
      tft.print(" c");
    }

    else
    {
      tft.print(temp);
      tft.print(" c");
    }

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
      delay(30000); //fan will start after 30sec
      digitalWrite(En, HIGH);
      delay(300000); // fan will working for 5min

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
    tft.setCursor(50, 80);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.println("Temp Range:");
    tft.setCursor(60, 130);
    tft.println("Offset: ");

    digitalWrite(RunButton, HIGH);
    if (digitalRead(RunButton) == LOW)
    {
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(50, 80);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE);
      tft.println("Temp Range: ");
      h += 5;
      tft.setCursor(184, 80);
      tft.setTextColor(ILI9341_GREEN);
      tft.print(h);
      tft.print("-to above");
    }

    if (yekan)
    {
      // up
      digitalWrite(buttonUp, HIGH);
      if (digitalRead(buttonUp) == LOW)
      {
        menu2();
        tft.print(h);
        tft.print("-to above");
        tft.setCursor(60, 130);
        tft.println("Offset: ");

        m++;

        tft.setCursor(150, 130);
        tft.setTextColor(ILI9341_GREEN);
        tft.print(m);
        tft.setTextColor(ILI9341_WHITE);
        tft.print(" :");
        tft.print(n);
      }

      // down

      digitalWrite(buttonDown, HIGH);
      if (digitalRead(buttonDown) == LOW)
      {
        menu2();
        tft.print(h);
        tft.print("-to above");
        tft.setCursor(60, 130);
        tft.println("Offset: ");

        m--;

        tft.setTextSize(2);
        tft.setCursor(150, 130);
        tft.setTextColor(ILI9341_GREEN);
        tft.print(m);
        tft.setTextColor(ILI9341_WHITE);
        tft.print(" :");
        tft.print(n);
      }
    }

    if (dahgan)
    {
      digitalWrite(buttonUp, HIGH);
      if (digitalRead(buttonUp) == LOW)
      {
        menu2();
        tft.print(h);
        tft.print("-to above");
        tft.setCursor(60, 130);
        tft.println("Offset: ");
        tft.setTextSize(2);
        tft.setCursor(150, 130);
        tft.print(v);

        n++;

        tft.setCursor(165, 130);
        tft.print(" :");
        tft.setTextColor(ILI9341_GREEN);
        tft.print(n);
      }

      digitalWrite(buttonDown, HIGH);
      if (digitalRead(buttonDown) == LOW)
      {
        menu2();
        tft.print(h);
        tft.print("-to above");
        tft.setCursor(60, 130);
        tft.println("Offset: ");
        tft.setTextSize(2);
        tft.setCursor(150, 130);
        tft.print(v);
        if (n > 0)
        {
          n--;
        }

        tft.setCursor(165, 130);
        tft.print(" :");
        tft.setTextColor(ILI9341_GREEN);
        tft.print(n);
      }
      tempRange = h;
      w = n;
      if (v < 0)
      {

        offset = v - w / 10;
      }
      else
      {
        offset = v + w / 10;
      }
    }

    buttonState = digitalRead(setButton);
    if (buttonState != lastButtonState)
    {

      if (buttonState == HIGH)
      {

        buttonPushCounter++;
      }

      // if low now, the change was unpressed to pressed
      if (buttonState == LOW)
      {
        //  get the time now to start the
        // 5-sec delay
        timeStart = millis();
        //indicate we're now timing a press...
        bCheckingSwitch = true;
      }
      else
      {

        bCheckingSwitch = false;
      }

      lastButtonState = buttonState;
    }

    if (bCheckingSwitch)
    {

      if ((millis() - timeStart) >= 5000)     // 5sec push button to reset

      {
        offset = 0.0;
        n = 0;
        m = 0;
        v = 0;
        w = 0.0;
        powerchange = true;
        set = false;
        yekan = true;
        // menu1
        tft.setCursor(60, 40);
        tft.setTextColor(ILI9341_YELLOW);
        tft.println("Reset to default");
        menu1();
      }
    }

    lastButtonState = buttonState;

    switch (buttonPushCounter)
    {
    case 2:

      v = m;
      yekan = false;
      dahgan = true;
      break;

    case 3:

      powerchange = true;
      set = false;
      buttonPushCounter = 0;
      tft.fillScreen(ILI9341_BLACK);
      menu1();
      break;
    }
  }
}
