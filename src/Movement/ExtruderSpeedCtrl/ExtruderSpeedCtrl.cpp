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
Platform* platform;
// Members of class ExtruderSpeedCtrl
ExtruderSpeedCtrl::ExtruderSpeedCtrl() noexcept
{

}

ExtruderSpeedCtrl::~ExtruderSpeedCtrl() noexcept
{

}

void ExtruderSpeedCtrl::Configure() noexcept
{

	InitStepTime();

//	driversStepping = reprap.GetPlatform().GetDriversBitmap(5);
//	reprap.GetPlatform().SetDirection(5, 1);
//	reprap.GetPlatform().SetDriverMicrostepping(5, 16, true);
//	reprap.GetPlatform().EnableDrivers(5);

	timer.SetCallback(ExtruderSpeedCtrl::TimerInterrupt, CallbackParameter(this));
//	PulseDuration = StepTimer::GetTickRate()*10e-6;
	PulseDuration = 7; //9.333 us

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
	NextStepTime = InitStepTime();
	timer.ScheduleCallbackFromIsr(startTime + NextStepTime);
}

uint16_t ExtruderSpeedCtrl::GetRawReading() const noexcept
{

//	return max<StepTimer::Ticks>(min<StepTimer::Ticks>(inputPort.ReadAnalogNotInverted() >> (AdcBits - 10), 1000),100);
//	return max<StepTimer::Ticks>(min<StepTimer::Ticks>(platform->GetZProbeOrDefault(0)->GetReading() >> (AdcBits - 10), 1000),100);
	return reprap.GetPlatform().ProbeValue;

}

/*static*/ void ExtruderSpeedCtrl::TimerInterrupt(CallbackParameter param) noexcept
{
	static_cast<ExtruderSpeedCtrl*>(param.vp)->Interrupt();
}

StepTimer::Ticks ExtruderSpeedCtrl::InitStepTime () noexcept
{

	//ExtruderStepTime = StepTimer::GetTickRate()/10; ///SpeedRefreshRate;
	uint32_t newvalue;
	//	float TimeValue;
	// stepper motor anglular resolution is 1.8°
	// stepper motor gear ratio 1:5.18
	// E-steps value is 492.45 -> 492.45 pulses to feed 1mm



	if (Speed != 0)
	{
		// Constant speed 0..1000 set through socket code G9
		newvalue = (uint32_t)Speed;
//		// apply the configured scaling
//		newvalue = min<uint32_t>(newvalue*Scale, 1000);
//		// invert the value 0<->1000
//		newvalue = 1000 - newvalue;
//		// scale it to a multiple of 10us for easiness to convert it to ticks
//		// 0 -> 1ms = 100 * 10us
//		// 1000 -> 81ms = 8000 * 10us + 100 *10us
//		newvalue = (newvalue<<3) + 100;
//
//		// convert the 10us to ticks
//		//Tick is 750kHZ or 1.3333 us
//		// shift by 3
//		newvalue = newvalue<<3;
	}
	else
	{
		// GetRawReading returns a value between 0..1000
		// without scaling -> 1000 = 1mm/s -> um/s
		newvalue = (uint32_t)GetRawReading();

//		newvalue = GetRawReading();
//		// apply the configured scaling
//		newvalue = min<uint16_t>(newvalue*Scale, 1000);
//		// invert the value 0<->1000
//		newvalue = 1000 - newvalue;
//		// scale it to a multiple of 10us for easiness to convert it to ticks
//		// 0 -> 1ms = 100 * 10us
//		// 1000 -> 81ms = 8000 * 10us + 100 *10us
//		newvalue = (newvalue<<3) + 100;
//
//		// convert the 10us to ticks
//		//Tick is 750kHZ or 1.3333 us
//		// shift by 3
//		newvalue = newvalue<<3;
	}

	// apply the configured scaling
	newvalue = newvalue*Scale;
	// limit the max speed to 100mm/s=100'000 um/s
	newvalue = min<uint32_t>(newvalue, 100000);
	// limit the min speed to 1 um/s to avoid division by 0 on the next step
	newvalue = max<uint32_t>(newvalue, 1);
	// turn the um/s into us/um
	// -> TimeValue = 1e6/(float)newvalue;
	// scale the us/um into ticks/um
	// -> newvalue = newvalue * 750000;
	// scale the ticks/um into ticks/E-Step by applying the E-Step scale -> 1mm = 492.45
	// -> newvalue = newvalue * 1000  / 492.45 ~= newvalue 2;

	// merged all into a single division
	newvalue = (uint32_t)(1.523e6/newvalue);
	//newvalue = GetRawReading();
	// apply the configured scaling
	//newvalue = min<uint16_t>(newvalue*Scale, 1000);
	// invert the value 0<->1000
	//newvalue = 1000 - newvalue;
	// scale it to a multiple of 10us for easiness to convert it to ticks
	// 0 -> 1ms = 100 * 10us
	// 1000 -> 81ms = 8000 * 10us + 100 *10us
	//newvalue = (newvalue<<3) + 100;


	 return newvalue;

}

void ExtruderSpeedCtrl::SetConstantSpeed(uint16_t RecSpeed) noexcept
{
	Speed = RecSpeed;

}
void ExtruderSpeedCtrl::Interrupt() noexcept
{
	if (timer.ScheduleCallbackFromIsr())
	{
		StepstartTime = StepTimer::GetTimerTicks();

		StepUp();

		//prepare the next step while we wait for the pulse to finish
		NextStepTime = InitStepTime();

		while (StepTimer::GetTimerTicks() < StepstartTime+PulseDuration) {}

		StepDown();

		startTime = StepTimer::GetTimerTicks();
		timer.ScheduleCallbackFromIsr(startTime + NextStepTime);
		return;

	}
}

void ExtruderSpeedCtrl::StepUp() noexcept
{

	reprap.GetPlatform().GetGpOutPort(ExtruderPortNumber_OutPulses).WriteAnalog(100);

}
void ExtruderSpeedCtrl::StepDown() noexcept
{

	reprap.GetPlatform().GetGpOutPort(ExtruderPortNumber_OutPulses).WriteAnalog(0);

}

void ExtruderSpeedCtrl::StepStepper() noexcept
{
//	uint32_t lastStepPulseTime = lastStepLowTime;
//	while (StepTimer::GetTimerTicks() - lastStepPulseTime < reprap.GetPlatform().GetSlowDriverStepLowClocks()){}

	reprap.GetPlatform().GetGpOutPort(ExtruderPortNumber_OutPulses).WriteAnalog(100);
//	StepPins::StepDriversHigh(driversStepping);
//
//	# if SAME70
//			__DSB();													// without this the step pulse can be far too short
//	# endif

	StepstartTime = StepTimer::GetTimerTicks();
	while (StepTimer::GetTimerTicks() < StepstartTime+PulseDuration) {}
//	lastStepPulseTime = StepTimer::GetTimerTicks();
//	while (StepTimer::GetTimerTicks() - lastStepPulseTime < reprap.GetPlatform().GetSlowDriverStepHighClocks()) {}

	reprap.GetPlatform().GetGpOutPort(ExtruderPortNumber_OutPulses).WriteAnalog(0);
//	StepPins::StepDriversLow(driversStepping);

//	lastStepLowTime = StepTimer::GetTimerTicks();

}
// End
