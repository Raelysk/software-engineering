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

	if (!wicFactory.isInitialized())
		return false;

	COMPtr<IWICBitmapDecoder> wicDecoder;
	COMPtr<IWICBitmapFrameDecode> wicFrameDecode;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	HRESULT hResult = S_OK;

	hResult = wicFactory->CreateDecoderFromFilename(filename, nullptr,
		GENERIC_READ, WICDecodeMetadataCacheOnLoad, wicDecoder.initRef());
	if (FAILED(hResult)) return false;

	GUID wicImageFormat;
	hResult = wicDecoder->GetContainerFormat(&wicImageFormat);
	if (FAILED(hResult)) return false;

	ImageFormat format = ImageFormat::None;
	if (wicImageFormat == GUID_ContainerFormatPng)
		format = ImageFormat::Png;
	else if (wicImageFormat == GUID_ContainerFormatJpeg)
		format = ImageFormat::Jpeg;
	else if (wicImageFormat == GUID_ContainerFormatBmp)
		format = ImageFormat::Bmp;
	else
		return false;

	hResult = wicDecoder->GetFrame(0, wicFrameDecode.initRef());
	if (FAILED(hResult)) return false;

	hResult = wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
	if (FAILED(hResult)) return false;

	hResult = wicFormatConverter->Initialize(wicFrameDecode, GUID_WICPixelFormat32bppPRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hResult)) return false;

	UINT width = 0, height = 0;
	hResult = wicFormatConverter->GetSize(&width, &height);
	if (FAILED(hResult)) return false;

	_width = width;
	_height = height;
	_format = format;

	data.resize(width * height * 4);

	hResult = wicFormatConverter->CopyPixels(nullptr, width * 4, width * height * 4, data);
	if (FAILED(hResult)) return false;

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

	if (!wicFactory.isInitialized())
		return false;

	COMPtr<IWICStream> wicStream;
	COMPtr<IWICBitmapEncoder> wicEncoder;
	COMPtr<IWICBitmapFrameEncode> wicFrameEncode;
	COMPtr<IWICBitmap> wicSourceBitmap;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	HRESULT hResult = S_OK;

	hResult = wicFactory->CreateStream(wicStream.initRef());
	if (FAILED(hResult)) return false;

	hResult = wicStream->InitializeFromFilename(filename, GENERIC_WRITE);
	if (FAILED(hResult)) return false;

	hResult = wicFactory->CreateEncoder(wicImageFormat, nullptr, wicEncoder.initRef());
	if (FAILED(hResult)) return false;

	hResult = wicEncoder->Initialize(wicStream, WICBitmapEncoderNoCache);
	if (FAILED(hResult)) return false;

	hResult = wicEncoder->CreateNewFrame(wicFrameEncode.initRef(), nullptr);
	if (FAILED(hResult)) return false;

	hResult = wicFrameEncode->Initialize(nullptr);
	if (FAILED(hResult)) return false;

	wicFrameEncode->SetSize(width, height);
	GUID wicDstPixelFormat = GUID_WICPixelFormat32bppPRGBA;
	wicFrameEncode->SetPixelFormat(&wicDstPixelFormat);

	if (wicDstPixelFormat != GUID_WICPixelFormat32bppPRGBA)
	{
		hResult = wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
		if (FAILED(hResult)) return false;

		hResult = wicFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppPRGBA,
			width * 4, width * height * 4, (BYTE*) data, wicSourceBitmap.initRef());
		if (FAILED(hResult)) return false;

		hResult = wicFormatConverter->Initialize(wicSourceBitmap, wicDstPixelFormat,
			WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hResult)) return false;

		WICRect wicRect = { 0, 0, int(width), int(height) };
		hResult = wicFrameEncode->WriteSource(wicFormatConverter, &wicRect);
		if (FAILED(hResult)) return false;
	}
	else
	{
		hResult = wicFrameEncode->WritePixels(height, width * 4, width * height * 4, (BYTE*) data);
		if (FAILED(hResult)) return false;
	}

	hResult = wicFrameEncode->Commit();
	if (FAILED(hResult)) return false;

	hResult = wicEncoder->Commit();
	if (FAILED(hResult)) return false;

	hResult = wicStream->Commit(STGC_DEFAULT);
	if (FAILED(hResult)) return false;

	return true;
}