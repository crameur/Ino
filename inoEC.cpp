#include "Arduino.h"
#include "inoEC.h"

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
#define CalNext		  14
extern void changePage(int page);

inoEC::inoEC(int pinvMeas, int pinvRef) 
	:	_numReadings(50),     
		_msDelay(20),
		pinvMeas(pinvMeas),
		pinvRef(pinvRef),
		vRef(0.237),
		vP1(0.845),
		calibV1min(0.7),
		calibV1max(1.1),
		vP2(2.80),
		calibV2min(2.45),
		calibV2max(3.15)
{
	
	pinMode(pinvMeas, INPUT);
	pinMode(pinvRef, INPUT);
}

// Measures the voltage supplied by the eC module (may take some time (msDelay*numReadings) )
double inoEC::getvMeasured(int pin, int msDelay, int numReadings)
{
	if (msDelay == 0) { msDelay = _msDelay; }
	if (numReadings == 0) { numReadings = _numReadings; }

	readings = new double[numReadings];		// Creating the array
	// Measures voltages and returns the average
	double total = 0;                  // the running total

	for (int i = 0; i < numReadings; i++) {
		readings[i] = 0;							// value clearing
		readings[i] = analogRead(pin)*5.0/1024.0;	// read and convert from the sensor
		total = total + readings[i];				// add the reading to the total
		delay(msDelay);
	}

	if (readings) { delete[] readings;}		// Clearing the array 

	// return the averaged voltage read
	return total / double(numReadings);
	


}

// Calibrates the EC using a 2-point method
void inoEC::calibrEC(double tempDeg, bool devMode)
{
	changePage(calibEC_menu);

	bool calib1 = true, calib2 = true, calibRunning = true;
	unsigned long fsTimer = millis();
	double currentVolt = 0;
	Serial.println(F(" Veuillez immerger la sonde eC dans l'une des solutions de calibration [1413us/cm ou 12.88ms/cm] "));
	while (millis() - fsTimer < 65000 and calibRunning) // Failsafe for calibration escape
	{
		currentVolt = getvMeasured(pinvMeas);
		if (devMode){ Serial.println(currentVolt); }
		if ( (currentVolt >= calibV1min) && (currentVolt <= calibV1max) && (calib1) )
		{
			Serial.println(F(" Solution 1413us/cm detectée. Calibration en cours, laissez la sonde immergée."));

			changePage(ecCalmin);

			delay(5000);
			// Calibrating using the 1413us/cm solution
			vP1 = getvMeasured(pinvMeas);

			// -----------
			calib1 = false;
			calibRunning = calib1 or calib2;
			if (calib2)
			{
				Serial.println(F(" Veuillez rincer la sonde et l'immerger dans la solution de conductivité 12.88ms/cm "));
				changePage(CalNext);
				// Reseting failsafe timer
				fsTimer = millis();
			}

			Serial.println(F(" Fin 1413 "));

		}
		if ( (currentVolt >= calibV2min) && (currentVolt <= calibV2max) && (calib2) )
		{
			Serial.println(F(" Solution 12.88ms/cm detectée. Calibration en cours, laissez la sonde immergée."));
			
			changePage(ecCalmax);
			
			delay(5000);
			// Calibrating using the 12.88ms/cm solution
			vP2 = getvMeasured(pinvMeas);

			changePage(ecCalmax);


			// -----------
			calib2 = false;
			calibRunning = calib1 or calib2;
			if (calib1)
			{
				Serial.println(F(" Veuillez rincer la sonde et l'immerger dans la solution de conductivité 1413us/cm "));
				changePage(CalNext);
				// Reseting failsafe timer
				fsTimer = millis();
			}

			Serial.println(F(" Fin 12.88 "));
			
		}
	}

	// 
	Serial.println(F("Calibration terminée."));

	changePage(ph_ec_CalOK);


	if (devMode) {
		Serial.println(F(" Nouvelles valeurs étalon :"));
		Serial.print(F("vP1 ="));
		Serial.println(vP1);
		Serial.print(F("vP2 ="));
		Serial.println(vP2);
	}
}

// Converts the voltage measured by the eC module to a resistance
double inoEC::Volt2Res(double vMeasured, double vRef)
{
	if (vRef == 0) {
		return (620 * inoEC::vRef) / (vMeasured - inoEC::vRef);
	}
	else
	{
		if (vRef == vMeasured) { return -1; }
		else { return (620 * vRef) / (vMeasured - vRef); }
	}
		
}

// Converts the given resistance to the conductivity
double inoEC::Res2Cond(double resistance, double tempDeg)
{

	// Plotting Cond = f(Res)
	double resEqP1 = Volt2Res(vP1);		// Gives the resistance read by the module for a conductivity of 1413us/cm
	double resEqP2 = Volt2Res(vP2);		// Gives the resistance read by the module for a conductivity of 12.88ms/cm
		// 1st degree linear regression approximation y = mx + c
	double m = (12.88e-3 - 1413e-6) / (resEqP2 - resEqP1);
	double c =  -(m * resEqP1) + 1413e-6;
		// y = mx + c + a(x)
	double a = 0.02;						// Linear conductivity compensation coefficient

	// Returns the conductivity in mS/cm <=> EC
	double y = (m * resistance + c)*(1 + a * (tempDeg - 25)) * 1000;		// Filtering values
	if (resistance == -1 || y < 0 || y > 20) { return 0; }
	else 
		return y; 
}

double inoEC::getEC(int msDelay, int numReadings, double tempDeg)
{
	double vRef = getvMeasured(pinvRef, msDelay, numReadings);
	double vMeas = getvMeasured(pinvMeas, msDelay, numReadings);

	double res;
	// If the reference voltage is accurate, we can use it to have even more precise measurements
	if (0.8*vRef <= 0.237 & 0.237 <= 1.2*vRef)
	{res = Volt2Res(vMeas, vRef);}
	else
	{res = Volt2Res(vMeas);}

	return Res2Cond(res, tempDeg);

}


