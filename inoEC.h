/*
  inoEC.h - Library for the eC meter
  2019 Melvyn FOWELL.
*/

#ifndef inoEC_h
#define inoEC_h

#include "Arduino.h"


class inoEC
{
  public:
    inoEC(int pinvMeas, int pinvRef);


	double getvMeasured(int pin, int msDelay = 0, int numReadings = 0);		// Measures the voltage supplied by the eC module
	void calibrEC(double tempDeg = 25, bool devMode = false);					// Calibrates the EC using a 2-point method
	double Volt2Res(double vMeasured, double vRef = 0);						// Converts the voltage measured by the eC module to a resistance
	double Res2Cond(double resistance, double tempDeg = 25);					// Converts the resistance to the conductivity
	double getEC(int msDelay = 0, int numReadings = 0, double tempDeg = 25);	// Measures the conductivity of the solution

	double getvP1();
	double getvP2();
	void setvP1(double vP1_);
	void setvP2(double vP2_);
	
  private:
	int _numReadings;				// The number of readings
	int _msDelay;					// The delay between readings
	double* readings;				// The readings from the analog input
	const int pinvMeas;				// Pin number for the measured voltage	( E2 on eC module )
	const int pinvRef;				// Pin number for the reference voltage ( E1 on eC module )

	// Calibration voltage points
	double vRef;					// Voltage entering the amplifier circuit
		// For automatic detection of the calibration solution 1413us.cm
	double vP1;					// Initial voltage for the 1413us/cm standard _@25°C
	double calibV1min;
	double calibV1max;
		// For automatic detection of the calibration solution 12.88ms.		
	double vP2;					// Initial voltage for the 12.88ms/cm standard _@25°C
	double calibV2min;
	double calibV2max;
};

inline double inoEC::getvP1()
{
	return vP1;
}
inline double inoEC::getvP2()
{
	return vP2;
}
inline void inoEC::setvP1(double vP1_)
{
	vP1 = vP1_;
}
inline void inoEC::setvP2(double vP2_)
{
	vP2 = vP2_;
}


#endif

