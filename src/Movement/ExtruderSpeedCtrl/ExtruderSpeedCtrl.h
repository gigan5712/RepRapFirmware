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

class ExtruderSpeedCtrl
{
public:
	//DECLARE_FREELIST_NEW_DELETE(LocalZProbe)

	ExtruderSpeedCtrl() noexcept;
	~ExtruderSpeedCtrl() noexcept;

	void SetIREmitter(bool on) const noexcept;
	uint16_t GetRawReading() const noexcept;
	void Configure() noexcept;
	void InitStepTime() noexcept;

	void StepStepper() noexcept;
	void Start() noexcept;


private:
	IoPort inputPort;
	IoPort modulationPort;			// the modulation port we are using
	PwmPort port;
	IoPort tachoPort;										// port used to read the tacho

	// Variable for programming Smart Effector and other programmable Z probes
	static void TimerInterrupt(CallbackParameter param) noexcept;
	void Interrupt() noexcept;

	unsigned int bitsSent;

	static const unsigned int SpeedRefreshRate = 1000;
	uint32_t driversStepping = 0;

	StepTimer timer;
	StepTimer::Ticks startTime;
	StepTimer::Ticks StepstartTime;
	StepTimer::Ticks ExtruderStepTime;
	StepTimer::Ticks PulseDuration;
	StepTimer::Ticks lastStepLowTime;
	bool StepDir=false;
};


#endif /* SRC_MOVEMENT_EXTRUDERSPEEDCTRL_H_ */
