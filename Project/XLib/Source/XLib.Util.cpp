#include <intrin.h>

#include "XLib.Util.h"

uint32 clz(uint32 value)
{
	unsigned long result = 0;
	return _BitScanReverse(&result, value) ? 31 - uint32(result) : 32;
}