#pragma once

#include <XLib.Types.h>

namespace XLib::Graphics::Internal
{
	struct ShaderData
	{
		const void* data;
		uint32 size;
	};

	class Shaders abstract final
	{
	public:
		static ShaderData Color2DVS;
		static ShaderData ColorPS;

		static ShaderData Textured2DVS;
		static ShaderData TexturedPS;
	};
}