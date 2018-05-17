#pragma once

#include <XLib.Types.h>
#include <XLib.Heap.h>

namespace Panter
{
	class ImageLoader abstract final
	{
	public:
		static bool Load(const wchar* filename, XLib::HeapPtr<byte>& data, uint32& width, uint32& height);
	};
}
