#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include "SystemUtil.h"

bool OpenFileDialog(void* parentWindowHandle, wchar* buffer, uint32 bufferLength)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = HWND(parentWindowHandle);
	ofn.lpstrFile = buffer;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = bufferLength;
	ofn.lpstrFilter = L"All supported image formats\0*.png;*.jpg;*.bmp\0PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0BMP (*.bmp)\0*.bmp\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	BOOL result = GetOpenFileNameW(&ofn);

	return result != 0;
}

bool SaveFileDialog(void * parentWindowHandle, wchar * buffer, uint32 bufferLength)
{
	OPENFILENAME ofn;

	const int supportedExtensionNumber = 3;
	const wchar* extensions[supportedExtensionNumber] = {L".png", L".jpg", L".bmp"};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = HWND(parentWindowHandle);
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile = bufferLength;
	ofn.lpstrFilter = L"PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0BMP (*.bmp)\0*.bmp\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST;

	BOOL result = GetSaveFileNameW(&ofn);

	if (result != 0) {
		const wchar* fileExtension = PathFindExtensionW(buffer);

		if (wcscmp(fileExtension, L"") == 0 || wcscmp(fileExtension, extensions[ofn.nFilterIndex - 1]) != 0) {
			size_t stringLength = wcslen(buffer);
			wcscat(buffer + stringLength, extensions[ofn.nFilterIndex - 1]);
		}
	}

	return result != 0;
}
