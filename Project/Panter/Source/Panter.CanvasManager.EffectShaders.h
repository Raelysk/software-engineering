#pragma once

#include <XLib.Types.h>

namespace Panter
{
	struct ShaderData
	{
		const void* data;
		uint32 size;
	};

	class EffectShaders abstract final
	{
	public:
		static const ShaderData CheckerboardPS;
		static const ShaderData BrightnessContrastGammaPS;
		static const ShaderData BlurPS;
		static const ShaderData SharpenPS;
	};
}