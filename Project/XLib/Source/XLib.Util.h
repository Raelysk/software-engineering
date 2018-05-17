#pragma once

#include "XLib.Types.h"

#ifndef __PLACEMENT_NEW_INLINE

inline void* operator new (size_t, void* block) { return block; }
inline void operator delete (void* block, void*) {}

#endif

template <typename type, uint32 size> constexpr uint32 countof(type(&)[size]) { return size; }
template <typename type, uint32 size> constexpr uint32 byteSizeOfArray(type(&)[size]) { return size * sizeof(type); }
template <typename type> inline void construct(type& value) { new (&value) type(); }

#undef offsetof
#define offsetof(type, field) uintptr(&((type*)nullptr)->field)

template <typename _type> struct removeReference abstract final { using type = _type; };
template <typename _type> struct removeReference<_type&> abstract final { using type = _type; };
template <typename _type> struct removeReference<_type&&> abstract final { using type = _type; };

template <typename type> inline typename removeReference<type>::type&& move(type&& object) { return (typename removeReference<type>::type&&)object; }

template <typename resultType, typename argumentType>
inline typename removeReference<resultType>::type as(argumentType&& value)
{
	static_assert(sizeof(resultType) == sizeof(argumentType), "Xlib.Util: as() function types must be same size");
	return *((resultType*)&value);
}
template <typename resultType, typename argumentType>
inline typename removeReference<resultType>::type& as(argumentType& value)
{
	static_assert(sizeof(resultType) == sizeof(argumentType), "XLib.Util: as() function types must be same size");
	return *((resultType*)&value);
}
template <typename resultType, typename argumentType>
inline typename removeReference<resultType>::type to(const argumentType& value) { return resultType(value); }

template <typename type>
inline void swap(type& a, type& b)
{
	type tmp(move(a));
	a = move(b);
	b = move(tmp);
}

#undef min
#undef max

template <typename type> constexpr inline type sqr(type val) { return val * val; }
template <typename type> constexpr inline type abs(type val) { return val >= type(0) ? val : -val; }
template <typename type> constexpr inline type min(type val1, type val2) { return val1 < val2 ? val1 : val2; }
template <typename type> constexpr inline type max(type val1, type val2) { return val1 > val2 ? val1 : val2; }
template <typename type> constexpr inline type intdivceil(type val, type divider) { return (val - 1) / divider + 1; }
template <typename valueType, typename alignmentType> constexpr inline auto alignup(valueType value, alignmentType alignment) { return valueType(value + alignment - 1) / alignment * alignment; }
template <typename valueType, typename alignmentType> constexpr inline auto aligndown(valueType value, alignmentType alignment) { return value / alignment * alignment; }

template <typename type>
constexpr inline type clamp(type val, type _min, type _max)
{
	if (val < _min) val = _min;
	if (val > _max) val = _max;
	return val;
}
template <typename type> constexpr inline type saturate(type val) { return clamp(val, type(0), type(1)); }
template <typename type> constexpr inline type lincoef(type left, type right, type x) { return (left - x) / (left - right); }
template <typename vectorType, typename scalarType> constexpr inline vectorType lerp(const vectorType& x, const vectorType& y, scalarType coef) { return x + (y - x) * coef; }
template <typename vectorType> constexpr inline vectorType lerp(const vectorType& x, const vectorType& y, uint32 numerator, uint32 denominator) { return x + (y - x) * float32(numerator) / float32(denominator); }

uint32 clz(uint32 value);
uint32 clo(uint32 value);
uint32 cto(uint32 value);
uint32 ctz(uint32 value);
inline uint32 flo(uint32 value) { return 32 - clz(value); }
inline uint32 flz(uint32 value) { return 32 - clo(value); }

inline uint32 sgn(uint32 value) { return uint32(value != 0); }