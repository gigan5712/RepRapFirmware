/*
 * ExtruderSpeedCtrl.cpp
 *
 *  Created on: 06 Sept 2022
 *      Author: Giuliano
 */

#include "ExtruderSpeedCtrl.h"

#include <GCodes/GCodeBuffer/GCodeBuffer.h>
#include <Platform/RepRap.h>
#include <Platform/Platform.h>
#include <Endstops/ZProbe.h>
#include <AnalogIn.h>
#include <Hardware/IoPorts.h>
using
#if SAME5x
	AnalogIn
#else
	LegacyAnalogIn
#endif
	::AdcBits;

// Members of class ExtruderSpeedCtrl
ExtruderSpeedCtrl::ExtruderSpeedCtrl() noexcept
{

}

ExtruderSpeedCtrl::~ExtruderSpeedCtrl() noexcept
{
	inputPort.Release();
	modulationPort.Release();

}

void ExtruderSpeedCtrl::Configure() noexcept
{
	InitStepTime();

	driversStepping = reprap.GetPlatform().GetDriversBitmap(5);
	reprap.GetPlatform().SetDirection(5, 1);
	reprap.GetPlatform().SetDriverMicrostepping(5, 16, true);
	reprap.GetPlatform().EnableDrivers(5, true);

	timer.SetCallback(ExtruderSpeedCtrl::TimerInterrupt, CallbackParameter(this));
	PulseDuration = StepTimer::GetTickRate()*1e-3; //1ms

	if (reprap.Debug(moduleSpeedExtr))
	{
		debugPrintf("moduleSpeedExtr: Configured\n");
	}
}
void ExtruderSpeedCtrl::Start() noexcept
{
	if (reprap.Debug(moduleSpeedExtr))
	{
		debugPrintf("moduleSpeedExtr: Started\n");
	}
	startTime = StepTimer::GetTimerTicks();
	timer.ScheduleCallbackFromIsr(startTime + ExtruderStepTime);
}

uint16_t ExtruderSpeedCtrl::GetRawReading() const noexcept
{
	return max<uint16_t>(min<uint16_t>(inputPort.ReadAnalog() >> (AdcBits - 10), 1000),100);
}


/*static*/ void ExtruderSpeedCtrl::TimerInterrupt(CallbackParameter param) noexcept
{
	static_cast<ExtruderSpeedCtrl*>(param.vp)->Interrupt();
}

void ExtruderSpeedCtrl::InitStepTime () noexcept
{

	// TODO:Convert from ADC to step time
	ExtruderStepTime = StepTimer::GetTickRate()/10; ///SpeedRefreshRate;
	//ExtruderStepTime = GetRawReading()<<10;

}

void ExtruderSpeedCtrl::Interrupt() noexcept
{
	if (timer.ScheduleCallbackFromIsr())
	{
		StepStepper();
		//StepDir = not StepDir;

		//InitStepTime();
		startTime = StepTimer::GetTimerTicks();
		timer.ScheduleCallbackFromIsr(startTime + ExtruderStepTime);
		return;

	}
}

void ExtruderSpeedCtrl::StepStepper() noexcept
{
	uint32_t lastStepPulseTime = lastStepLowTime;
	while (StepTimer::GetTimerTicks() - lastStepPulseTime < reprap.GetPlatform().GetSlowDriverStepLowClocks()){}

	reprap.GetPlatform().GetGpOutPort(6).WriteAnalog(100);
	StepPins::StepDriversHigh(driversStepping);

	# if SAME70
			__DSB();													// without this the step pulse can be far too short
	# endif

//	StepstartTime = StepTimer::GetTimerTicks();
//	while (StepTimer::GetTimerTicks() < StepstartTime+PulseDuration) {}
	lastStepPulseTime = StepTimer::GetTimerTicks();
	while (StepTimer::GetTimerTicks() - lastStepPulseTime < reprap.GetPlatform().GetSlowDriverStepHighClocks()) {}

	reprap.GetPlatform().GetGpOutPort(6).WriteAnalog(0);
	StepPins::StepDriversLow(driversStepping);

	lastStepLowTime = StepTimer::GetTimerTicks();

}
// End
