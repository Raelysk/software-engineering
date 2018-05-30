#pragma once

#include <XLib.Types.h>
#include <XLib.Heap.h>

enum class ImageFormat
{
	None = 0,
	Png,
	Jpeg,
	Bmp,
};

bool OpenImageFileDialog(void* parentWindowHandle, wchar* filenameBuffer, uint32 filenameBufferLength);
bool SaveImageFileDialog(void* parentWindowHandle, wchar* filenameBuffer, uint32 filenameBufferLength, ImageFormat* format);

bool LoadImageFromFile(const wchar* filename, XLib::HeapPtr<byte>& data, uint32& width, uint32& height, ImageFormat& format);
bool SaveImageToFile(const wchar* filename, ImageFormat format, const void* data, uint32 width, uint32 height);