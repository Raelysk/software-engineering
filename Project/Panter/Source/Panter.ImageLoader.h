#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

// TODO: imlement proper loader (disk worker, async, separate image info readers, etc)

namespace Panter
{
	class ImageLoader : public XLib::NonCopyable
	{
	private:


	public:
		static bool Load(const char* filename);
	};
}
