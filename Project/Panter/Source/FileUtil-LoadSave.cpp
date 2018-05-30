#include <wincodec.h>
#include <string.h>

#include <XLib.Platform.COMPtr.h>

#include "FileUtil.h"

using namespace XLib::Platform;

static COMPtr<IWICImagingFactory> wicFactory;

inline void checkWICInitialization()
{
	if (!wicFactory.isInitialized())
	{
		CoInitialize(nullptr);
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			wicFactory.uuid(), wicFactory.voidInitRef());
	}
}

bool LoadImageFromFile(const wchar* filename, XLib::HeapPtr<byte>& data,
	uint32& _width, uint32& _height, ImageFormat& _format)
{
	checkWICInitialization();

	COMPtr<IWICBitmapDecoder> wicDecoder;
	COMPtr<IWICBitmapFrameDecode> wicFrameDecode;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	wicFactory->CreateDecoderFromFilename(filename, nullptr,
		GENERIC_READ, WICDecodeMetadataCacheOnLoad, wicDecoder.initRef());

	GUID wicImageFormat;
	wicDecoder->GetContainerFormat(&wicImageFormat);

	ImageFormat format = ImageFormat::None;
	if (wicImageFormat == GUID_ContainerFormatPng)
		format = ImageFormat::Png;
	else if (wicImageFormat == GUID_ContainerFormatJpeg)
		format = ImageFormat::Jpeg;
	else if (wicImageFormat == GUID_ContainerFormatBmp)
		format = ImageFormat::Bmp;
	else
		return false;

	wicDecoder->GetFrame(0, wicFrameDecode.initRef());
	wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
	wicFormatConverter->Initialize(wicFrameDecode, GUID_WICPixelFormat32bppPRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);

	UINT width = 0, height = 0;
	wicFormatConverter->GetSize(&width, &height);

	_width = width;
	_height = height;
	_format = format;

	data.resize(width * height * 4);

	wicFormatConverter->CopyPixels(nullptr, width * 4, width * height * 4, data);

	return true;
}

bool SaveImageToFile(const wchar* filename, ImageFormat format, const void* data, uint32 width, uint32 height)
{
	GUID wicImageFormat;

	switch (format)
	{
		case ImageFormat::Png:
			wicImageFormat = GUID_ContainerFormatPng;
			break;

		case ImageFormat::Jpeg:
			wicImageFormat = GUID_ContainerFormatJpeg;
			break;

		case ImageFormat::Bmp:
			wicImageFormat = GUID_ContainerFormatBmp;
			break;

		default:
			return false;
	}

	checkWICInitialization();

	COMPtr<IWICStream> wicStream;
	COMPtr<IWICBitmapEncoder> wicEncoder;
	COMPtr<IWICBitmapFrameEncode> wicFrameEncode;
	COMPtr<IWICBitmap> wicSourceBitmap;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	wicFactory->CreateStream(wicStream.initRef());
	wicStream->InitializeFromFilename(filename, GENERIC_WRITE);

	wicFactory->CreateEncoder(wicImageFormat, nullptr, wicEncoder.initRef());
	wicEncoder->Initialize(wicStream, WICBitmapEncoderNoCache);
	wicEncoder->CreateNewFrame(wicFrameEncode.initRef(), nullptr);

	wicFrameEncode->Initialize(nullptr);
	wicFrameEncode->SetSize(width, height);
	GUID wicDstPixelFormat = GUID_WICPixelFormat32bppPRGBA;
	wicFrameEncode->SetPixelFormat(&wicDstPixelFormat);

	if (wicDstPixelFormat != GUID_WICPixelFormat32bppPRGBA)
	{
		wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
		wicFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppPRGBA,
			width * 4, width * height * 4, (BYTE*) data, wicSourceBitmap.initRef());
		wicFormatConverter->Initialize(wicSourceBitmap, wicDstPixelFormat,
			WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);

		WICRect wicRect = { 0, 0, int(width), int(height) };
		wicFrameEncode->WriteSource(wicFormatConverter, &wicRect);
	}
	else
	{
		wicFrameEncode->WritePixels(height, width * 4, width * height * 4, (BYTE*) data);
	}

	wicFrameEncode->Commit();
	wicEncoder->Commit();
	wicStream->Commit(STGC_DEFAULT);

	return true;
}