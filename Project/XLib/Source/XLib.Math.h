#pragma once

#include "XLib.Types.h"
#include "XLib.Util.h"

namespace XLib
{
	class Math abstract final
	{
	public:
		template <typename type> static constexpr inline type Clamp(type val, type minValue, type maxValue)
		{
			if (val < minValue) return minValue;
			if (val > maxValue) return maxValue;
			return val;
		}
		template <typename type> static constexpr inline type Saturate(type val) { return Math::Clamp(val, type(0), type(1)); }
		template <typename vectorType, typename coefType> static constexpr inline vectorType LinInterp(const vectorType& a, const vectorType& b, coefType coef) { return a + vectorType((b - a) * coef); }
		template <typename type> static constexpr inline type LinCoef(type a, type b, type c) { return (a - c) / (a - b); }

		static inline float32 NanF32() { return as<float32>(uint32(0x7fc0'0000)); }
		static inline float64 NanF64() { return as<float64>(uint64(0x7ff8'0000'0000'0000ull)); }
		static constexpr float32 PiF32 = 3.14159265f;
		static constexpr float64 PiF64 = 3.141592653589793238;
		static constexpr float32 EF32 = 2.71828182f;
		static constexpr float64 EF64 = 2.718281828459045233;

		static float32 Sqrt(float32 arg);
		static float32 Sin(float32 arg);
		static float32 Cos(float32 arg);
		static float32 Tan(float32 arg);
		static float32 Asin(float32 arg);
		static float32 Acos(float32 arg);
		static float32 Atan(float32 arg);
		static float32 Pow(float32 value, float32 power);
	};
}

//template <typename type, typename _type> static constexpr inline type intdivceil(type val, _type div) { return (val - 1) / type(div) + 1; }
//template <typename type, typename _type> static constexpr inline type alignval(type val, _type mod) { return val / type(mod) * type(mod); }