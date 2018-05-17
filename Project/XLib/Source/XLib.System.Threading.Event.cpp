#include <Windows.h>

#include "XLib.System.Threading.Event.h"
#include "XLib.Debug.h"

using namespace XLib;

void Event::initialize(bool state, bool manualReset)
{
	destroy();
	handle = CreateEvent(nullptr, manualReset, state, nullptr);
}
void Event::set()
{
	Debug::CrashConditionOnDebug(!isInitialized(), DbgMsgFmt("not initialized"));
	SetEvent(handle);
}
void Event::reset()
{
	Debug::CrashConditionOnDebug(!isInitialized(), DbgMsgFmt("not initialized"));
	ResetEvent(handle);
}
void Event::pulse()
{
	Debug::CrashConditionOnDebug(!isInitialized(), DbgMsgFmt("not initialized"));
	PulseEvent(handle);
}