#include "TimerObject.h"

#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"

TimerObject::TimerObject(unsigned long int ms){
	Create(ms, NULL, false);
}

TimerObject::TimerObject(unsigned long int ms, CallBackType callback){
	Create(ms, callback, false);
}

TimerObject::TimerObject(unsigned long int ms, CallBackType callback, bool isSingle){
	Create(ms, callback, isSingle);
}

void TimerObject::Create(unsigned long int ms, CallBackType callback, bool isSingle){
	setInterval(ms);
	setEnabled(false);
	setSingleShot(isSingle);
	setOnTimer(callback);
	LastTime = 0;
}

void TimerObject::setInterval(unsigned long int ms){
	msInterval = (ms > 0) ? ms : 0;
}

void TimerObject::setEnabled(bool Enabled){
	blEnabled = Enabled;
}

void TimerObject::setSingleShot(bool isSingle){
	blSingleShot = isSingle;
}

void TimerObject::setOnTimer(CallBackType callback){
	onRun = callback;
}

void TimerObject::Start(){
	LastTime = HAL_GetTick();
	setEnabled(true);
}

void TimerObject::Resume(){
	LastTime = HAL_GetTick() - DiffTime;
	setEnabled(true);
}

void TimerObject::Stop(){
	setEnabled(false);

}

void TimerObject::Pause(){
	DiffTime = HAL_GetTick() - LastTime;
	setEnabled(false);

}

void TimerObject::Update(){
	if(Tick())
		onRun();
}

bool TimerObject::Tick(){
	if(!blEnabled)
		return false;
	if(LastTime > HAL_GetTick()*2)//millis restarted
		LastTime = 0;
	if ((unsigned long int)(HAL_GetTick() - LastTime) >= msInterval) {
		LastTime = HAL_GetTick();
		if(isSingleShot())
			setEnabled(false);
	    return true;
	}
	return false;
}


unsigned long int TimerObject::getInterval(){
	return msInterval;
}

unsigned long int TimerObject::getCurrentTime(){
	return (unsigned long int)(HAL_GetTick() - LastTime);
}
CallBackType TimerObject::getOnTimerCallback(){
	return onRun;
}

bool TimerObject::isEnabled(){
	return blEnabled;
}

bool TimerObject::isSingleShot(){
	return blSingleShot;
}
