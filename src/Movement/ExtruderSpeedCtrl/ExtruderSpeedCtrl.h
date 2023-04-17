/*
 * ExtruderSpeedCtrl.h
 *
 *  Created on: 06 Sept 2022
 *      Author: Giuliano
 */

#ifndef SRC_MOVEMENT_EXTRUDERSPEEDCTRL_H_
#define SRC_MOVEMENT_EXTRUDERSPEEDCTRL_H_

#include <Endstops/ZProbe.h>
#include <Movement/StepTimer.h>

const size_t ExtruderPortNumber_OutEnable 	= 2;	// to PoStep
const size_t ExtruderPortNumber_OutDir 		= 6;	// to PoStep
const size_t ExtruderPortNumber_OutIsEnabled = 4; 	// to Omnicore
const size_t ExtruderPortNumber_InEnable 	= 4;	// from Omnicore
const size_t ExtruderPortNumber_OutErr 		= 5;	// to Omnicore
const size_t ExtruderPortNumber_InDir 		= 5;	// from Omnicore
const size_t ExtruderPortNumber_InErr 		= 8;	// from PoStep
const size_t ExtruderPortNumber_OutPulses 	= 8;	// to PoStep

class ExtruderSpeedCtrl
{
public:
	//DECLARE_FREELIST_NEW_DELETE(LocalZProbe)

	ExtruderSpeedCtrl() noexcept;
	~ExtruderSpeedCtrl() noexcept;

	uint16_t GetRawReading() const noexcept;
	void Configure() noexcept;
	void SetScale(uint16_t RecScale) {Scale = RecScale;}
	uint16_t GetScale() {return Scale;}
	StepTimer::Ticks InitStepTime() noexcept;
	void SetConstantSpeed(uint16_t RecSpeed) noexcept;
	void StepStepper() noexcept;
	void StepUp() noexcept;
	void StepDown() noexcept;
	void Start() noexcept;


private:
	IoPort ProbePort;
	IoPort modulationPort;			// the modulation port we are using
	PwmPort port;
	IoPort tachoPort;										// port used to read the tacho
	uint16_t Speed=0;
	uint16_t Scale=1;

	// Variable for programming Smart Effector and other programmable Z probes
	static void TimerInterrupt(CallbackParameter param) noexcept;
	void Interrupt() noexcept;

	unsigned int bitsSent;

	static const unsigned int SpeedRefreshRate = 1000;
	uint32_t driversStepping = 0;

	StepTimer timer;
	StepTimer::Ticks startTime;
	StepTimer::Ticks StepstartTime;
	StepTimer::Ticks NextStepTime;
	StepTimer::Ticks PulseDuration;
	StepTimer::Ticks lastStepLowTime;
	bool StepDir=false;
};


#endif /* SRC_MOVEMENT_EXTRUDERSPEEDCTRL_H_ */
