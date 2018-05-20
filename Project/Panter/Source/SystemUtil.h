#pragma once

#include <XLib.Types.h>

bool OpenFileDialog(void* parentWindowHandle, wchar* buffer, uint32 bufferLength);
bool SaveFileDialog(void* parentWindowHandle, wchar* buffer, uint32 bufferLength);