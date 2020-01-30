#define DEBUG_OK 0
#define printEC_PH 1
/* INO */

#include <math.h>

// Pour le module de mesure d'EC
#include <inoEC.h>
#define pinMeas A7
#define pinRef  A6
#define pinEC pinMeas
inoEC ECmodule(pinMeas, pinRef);
unsigned long lastUpdateEC;
uint32_t sampleTimeEC = 1;  // Time between 2 automatic updates ( in seconds)
double currentEC;

// Pour le module de mesure de PH
#define pinPH A0
unsigned long lastUpdatePH;
uint32_t sampleTimePH = 1;  // Time between 2 automatic updates ( in seconds)
void calibrPH(double tempDeg = 25);     // Calibrates the pH probe
double getPH(int pinPH, int msDelayPH = 0, int numReadings = 0, double tempDeg = 25);   // Measures the pH
double vPH1 = 1.96, vPH2 = 1.11;
double calibPH1min = 1.7, calibPH1max = 2.2, calibPH2min = 0.9, calibPH2max = 1.25;
double currentPH;

// Pour le module de mesure de pression
#include <Adafruit_LPS35HW.h>
#define pSCL A5     //  SCK
#define pSDA A4     //  SDI
Adafruit_LPS35HW TPmodule = Adafruit_LPS35HW();
unsigned long lastUpdateTP;
uint32_t sampleTimeTP = 1;  // Time between 2 automatic updates ( in seconds)
double getTemp();     // Temperature in °C
double getPressure(); // Pressure in hPa
double offWaterPressure = 1003.01;
double getDepth(double offWaterPressure = offWaterPressure);    // Depth in cm
double currentTemp;
double currentDepth;

// Pour les liaisons séries Display-MCU
#include <Nextion.h>
#define dispRX 10 // -> Blue cable 
#define dispTX 11 // -> Yellow cable
SoftwareSerial myDisplay(dispRX, dispTX);

void refresh();

/***   Nextion pages  ***/
#define menu          0
#define phscreen      1
#define ecscreen      2
#define dscreen       3
#define phTimer       4
#define calibPH_menu  5
#define phCal4        6
#define phCal7        7
#define ph_ec_CalOK   8
#define ecTimer       9
#define calibEC_menu  10
#define ecCalmin      11
#define ecCalmax      12
#define loading       13
#define CalNext       14


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.println(F(" Liaison série PC | OK "));

  // Initializing LPS35HW
  if (!TPmodule.begin()) {
    Serial.println(F("Erreur LPS35HW "));
  }
  else {
    Serial.println(F("LPS35HW | OK"));
  }

  nexInit();
  // Opening sofware serial & changing baudrate
  //myDisplay.begin(9600);
  //delay(300);
  //myDisplay.print(F("baud=115200")); // Works with 38400
  //sendOff();
  myDisplay.begin(115200);

  Serial.println(F(" Refreshing values "));

  lastUpdateEC = millis();
  lastUpdatePH = millis();
  lastUpdateTP = millis();

  // Setting up current sensor values :
  refresh();
  Serial.println(F(" End of setup "));

}

void loop() // Run over and over
{
  if (printEC_PH)
  {
    Serial.println(ECmodule.getvMeasured(pinEC));
    Serial.println(ECmodule.getvMeasured(pinPH));
  }
  if (DEBUG_OK){
  Serial.println(F("loop"));}
  // Auto-update EC
  if (abs(millis() - lastUpdateEC) >= sampleTimeEC * 1000UL)
  {
    // Code for EC data update
    lastUpdateEC = millis();
    //getEC(int msDelay = 0, int numReadings = 0, double tempDeg = 25);
    currentEC = ECmodule.getEC(0, 0, currentTemp);
    setValue(F("ecArd"), round10(currentEC));


    if (DEBUG_OK) {
      Serial.println(F("Auto_update_EC"));
    }
  }
  // Auto-update pH
  if (abs(millis() - lastUpdatePH) >= sampleTimePH * 1000UL)
  {
    // Code for PH data update
    lastUpdatePH = millis();

    currentPH = getPH(pinPH, 0, 0, currentTemp);
    setValue(F("phtext"), round10(currentPH));

    if (DEBUG_OK) {
      Serial.println(F("Auto_update_PH"));
    }

  }
  // Auto-update Temperature / Water Height
  if (abs(millis() - lastUpdateTP) >= sampleTimeTP * 1000UL)
  {
    // Code for TP data update
    lastUpdateTP = millis();

    currentDepth = getDepth(offWaterPressure);
    currentTemp = getTemp();
    setValue(F("depthtext"), round10(currentDepth));
    setValue(F("temptext"), round10(currentTemp));

    if (DEBUG_OK) {
      Serial.println(F("Auto_update_TP"));
    }
  }





  // Fetching sampleTime updates (prevent from using buttons that takes much memory)
  sampleTimePH = getVal(F("TimPHsec"), sampleTimePH);
  sampleTimeEC = getVal(F("TimECsec"), sampleTimeEC);


  // Ino loop for touch event listenning
  // If registered touch event on refreshbtn
  if ( getVal(F("vaRefresh"), 0) == 1 ) {
    digitalWrite(13, HIGH);
    refresh();
    setValue(F("vaRefresh"), 0);
    digitalWrite(13, LOW);
  }
  // If registered touch event on ec_calibrate
  if ( getVal(F("vaEC"), 0) == 1 ) {
    digitalWrite(13, HIGH);
    ECmodule.calibrEC(currentTemp,DEBUG_OK);
    setValue(F("vaEC"), 0);
    digitalWrite(13, LOW);
  }
  // If registered touch event on ph_calibrate
  if ( getVal("vaPH", 0) == 1 ) {
    digitalWrite(13, HIGH);
    calibrPH(currentTemp);
    setValue(F("vaPH"), 0);
    digitalWrite(13, LOW);
  }
  // If registered touch event on depth_cal
  if ( getVal("vaDepth", 0) == 1 ) {
    digitalWrite(13, HIGH);
    changePage(loading);
    delay(100);
    offWaterPressure = getPressure();
    changePage(menu);
    setValue(F("vaDepth"), 0);
    digitalWrite(13, LOW);
  }
}

void refresh()
{
  
  changePage(loading);
  currentDepth = getDepth(offWaterPressure);
  currentTemp = getTemp();
  currentPH = getPH(pinPH, 0, 0, currentTemp);
  currentEC = ECmodule.getEC(0, 0, currentTemp);

  setValue(F("ecArd"), round10(currentEC));
  setValue(F("phtext"), round10(currentPH));
  setValue(F("depthtext"), round10(currentDepth));
  setValue(F("temptext"), round10(currentTemp));

  changePage(menu);
}

int round10(double number)
{
  return int(roundf(number * 100) / 10);
}
void changePage(int page)
{
  // Formatting data
  String cmd = String("page ");
  cmd += page;
  // Sending data
  myDisplay.print(cmd);
  sendOff();
}
void sendOff()
{
  myDisplay.write(0xFF);
  myDisplay.write(0xFF);
  myDisplay.write(0xFF);
}
void setValue(String box, int value)
{
  // Clearing display buffer
  while (myDisplay.available())
  {
    myDisplay.read();
  }

  // Formatting data
  box += ".val=";
  box += value;
  // Sending data
  myDisplay.print(box);
  sendOff();

  if (DEBUG_OK) {
    Serial.println(box);
  }
}

uint32_t getVal(String box, uint32_t resultOnError)
{
  uint32_t result = resultOnError;
  int nTry = 3;
  while (result == resultOnError & nTry-- > 0 )
  {
    String cmd = String(F("get "));
    cmd += box;
    cmd += F(".val");
    sendCommand(cmd.c_str());
    recvRetNumber(&result);

    if (DEBUG_OK) {
      Serial.println(result);
    }
  }

  return result;
}

double avgPin(int pin, int msDelay = 0, int numReadings = 0, double tempDeg = 25)
{
  if (msDelay == 0)    {
    msDelay = 30;
  }
  if (numReadings == 0)  {
    numReadings = 50;
  }

  double total = 0;                                 // the running total
  double* readings = new double[numReadings];       // creating the array

  for (int i = 0; i < numReadings ; i++)            //  get  multiple sample values
  {
    readings[i] = 0;                                // value clearing
    readings[i] = analogRead(pin) * 5.0 / 1024;     // read and convert from the probe
    total = total + readings[i];                    // add the reading to the total
    delay(msDelay);
  }

  if (readings) {
    delete[] readings; // clearing the array
  }

  double Value = total / numReadings;

  return Value;
}

double getPH(int pinPH, int msDelayPH = 0, int numReadings = 0, double tempDeg = 25)
{
  double phValue = avgPin(pinPH,msDelayPH,numReadings,tempDeg);
  float m = (4.0-7.0)/(vPH2-vPH1);
  float b = m*vPH1-7.0;
  return phValue * m + b;
}


void calibrPH(double tempDeg = 25)
{
  changePage(calibPH_menu);

  double temp;
  int timeOut = 0;
  
  bool calib1 = true, calib2 = true, calibRunning = true;
  unsigned long fsTimer = millis();
  double currentPH = 0;
  Serial.println(F(" Veuillez immerger la sonde pH dans l'une des solutions de calibration [pH 7 ou pH 4] "));

  while (millis() - fsTimer < 65000 and calibRunning) // Failsafe for calibration escape
  {
    currentPH = avgPin(pinPH);
    if (DEBUG_OK) {
      Serial.println(currentPH);
    }
    if ( (currentPH >= calibPH1min) && (currentPH <= calibPH1max) && (calib1) )
    {
      Serial.println(F(" Solution pH 7 detectée. Calibration en cours, laissez la sonde immergée."));
      changePage(phCal7);
      
      // Waiting for values to stabilise
      temp = avgPin(pinPH);
      delay(1000);
      // Trying during 15seconds
      while ( temp != avgPin(pinPH) & timeOut++ < 150)
      { delay(100);
        temp = avgPin(pinPH);
      }
      
      // Calibrating using the ph 7 solution
      vPH1 = avgPin(pinPH);


      // -----------
      calib1 = false;
      calibRunning = calib1 or calib2;
      if (calib2)
      {
        Serial.println(F(" Veuillez rincer la sonde et l'immerger dans la solution de pH 4 "));
        changePage(CalNext);
        // Reseting failsafe timer
        fsTimer = millis();
      }

    }
    if ( (currentPH >= calibPH2min) && (currentPH <= calibPH2max) && (calib2) )
    {
      Serial.println(F(" Solution pH 4 detectée. Calibration en cours, laissez la sonde immergée."));
      changePage(phCal4);

      // Waiting for values to stabilise
      temp = avgPin(pinPH);
      delay(1000);
      // Trying during 15seconds
      while ( temp != avgPin(pinPH) & timeOut++ < 150)
      { delay(100);
        temp = avgPin(pinPH);
      }
        
      // Calibrating using the ph 4 solution
      vPH2 = avgPin(pinPH);

      // -----------
      calib2 = false;
      calibRunning = calib1 or calib2;
      if (calib1)
      {
        Serial.println(F(" Veuillez rincer la sonde et l'immerger dans la solution de pH 7 "));
        changePage(CalNext);
        // Reseting failsafe timer
        fsTimer = millis();
      }

    }
  }

  //
  Serial.println(F("Calibration terminée."));
  if (DEBUG_OK) {
    Serial.println(F(" Nouvelles valeurs étalon :"));
    Serial.print(F("vPH1 ="));
    Serial.println(vPH1);
    Serial.print(F("vPH2 ="));
    Serial.println(vPH2);
  }

  changePage(ph_ec_CalOK);
}


double getTemp() {
  return TPmodule.readTemperature();
}
double getPressure() {
  if (DEBUG_OK)
  {
    Serial.println(TPmodule.readPressure());
    delay(100);
  }
  return TPmodule.readPressure();
}

double getDepth(double offWaterPressure) {
  if (DEBUG_OK)
  {
    Serial.print(F(" Off Water : "));
    Serial.println(offWaterPressure);
    Serial.println(abs((getPressure()) - offWaterPressure));
    delay(100);
  }
 return (abs(getPressure()) - offWaterPressure);

}
