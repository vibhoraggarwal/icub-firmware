#include "dsp56f807.h"
#include "qd0.h"
#include "qd1.h"
#include "qd2.h"
#include "qd3.h"
#include "encoders_interface.h"


/***************************************************************************/
/**
 *	this function inits the encoder sensors
 ***************************************************************************/ 
void init_position_encoder(void)
{
	QD0_init ();
	QD1_init ();
	QD2_init ();
	QD3_init ();
	QD0_ResetPosition ();
	QD1_ResetPosition ();
	QD2_ResetPosition ();
	QD3_ResetPosition ();		
}

/***************************************************************************/
/**
 * this function reads the current position which will be used in the PID.
 * Measurament is given by the quadrature encoder (joints 0,1) or
 * software (joints 2,3)
 * @param   jnt is the joint number 
 * @return  the reading of the sensor
 ***************************************************************************/
Int32 get_position_encoder(byte jnt)
{
	dword temp=0;
	
	switch (jnt)
	{
		case 0:
			QD0_getPosition (&temp); 
		break;		
		case 1:
			QD1_getPosition (&temp);
		break;
		case 2:
			QD2_getPosition (&temp);
		break;
		case 3:
			QD3_getPosition (&temp);
		break;
	}
	
	return temp;
}

/***************************************************************************/
/**
 * this function set the current position of the specified encoder
 * @param  jnt is the joint number 
 * @param  position is the new position of the encoder
 ***************************************************************************/
void set_position_encoder(byte jnt,dword position)
{
	switch (jnt)
	{
		case 0:
			QD0_setPosition (position); 
		break;		
		case 1:
			QD1_setPosition (position);
		break;
		case 2:
			QD2_setPosition (position);
		break;
		case 3:
			QD3_setPosition (position);
		break;
	}
}