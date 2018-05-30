#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include "FileUtil.h"

bool OpenImageFileDialog(void* parentWindowHandle, wchar* filenameBuffer, uint32 filenameBufferLength)
{
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = HWND(parentWindowHandle);
	ofn.lpstrFile = filenameBuffer;
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = filenameBufferLength;
	ofn.lpstrFilter = L"All supported image formats\0*.png;*.jpg;*.bmp\0PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0BMP (*.bmp)\0*.bmp\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	BOOL result = GetOpenFileNameW(&ofn);

	return result != 0;
}

bool SaveImageFileDialog(void* parentWindowHandle, wchar* filenameBuffer, uint32 filenameBufferLength, ImageFormat* format)
{
	*format = ImageFormat::None;

	const wchar *extensions[] = { L".png", L".jpg", L".bmp" };

	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = HWND(parentWindowHandle);
	ofn.lpstrFile = filenameBuffer;
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = filenameBufferLength;
	ofn.lpstrFilter = L"PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0BMP (*.bmp)\0*.bmp\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST;

	BOOL result = GetSaveFileNameW(&ofn);

	if (result != 0)
	{
		const wchar* fileExtension = PathFindExtensionW(filenameBuffer);

		if (wcscmp(fileExtension, L"") == 0 || wcscmp(fileExtension, extensions[ofn.nFilterIndex - 1]) != 0)
		{
			size_t stringLength = wcslen(filenameBuffer);
			wcscat(filenameBuffer + stringLength, extensions[ofn.nFilterIndex - 1]);
		}
	}

	switch (ofn.nFilterIndex - 1)
	{
		case 0:
			*format = ImageFormat::Png;
			break;

		case 1:
			*format = ImageFormat::Jpeg;
			break;

		case 2:
			*format = ImageFormat::Bmp;
			break;
	}

	return result != 0;
}
