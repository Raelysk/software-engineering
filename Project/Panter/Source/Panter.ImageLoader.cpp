#include <wincodec.h>

#include <XLib.Platform.COMPtr.h>

#include "Panter.ImageLoader.h"

using namespace XLib::Platform;
using namespace Panter;

static COMPtr<IWICImagingFactory> wicFactory;

bool ImageLoader::Load(const wchar* filename, XLib::HeapPtr<byte>& data, uint32& _width, uint32& _height)
{
	if (!wicFactory.isInitialized())
	{
		CoInitialize(nullptr);
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			wicFactory.uuid(), wicFactory.voidInitRef());
	}

	COMPtr<IWICBitmapDecoder> wicBitmapDecoder;
	COMPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	wicFactory->CreateDecoderFromFilename(filename, nullptr,
		GENERIC_READ, WICDecodeMetadataCacheOnLoad, wicBitmapDecoder.initRef());
	wicBitmapDecoder->GetFrame(0, wicBitmapFrameDecode.initRef());
	wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
	wicFormatConverter->Initialize(wicBitmapFrameDecode, GUID_WICPixelFormat32bppPRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);

	UINT width = 0, height = 0;
	wicFormatConverter->GetSize(&width, &height);

	_width = width;
	_height = height;

	data.resize(width * height * 4);

	wicFormatConverter->CopyPixels(nullptr, width * 4, width * height * 4, data);

	return true;
}