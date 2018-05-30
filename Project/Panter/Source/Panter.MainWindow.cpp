#include "Panter.MainWindow.h"

#include "FileUtil.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

MainWindow::MainWindow(Device& device) : device(device) {}

void MainWindow::onCreate(CreationArgs& args)
{
	device.createWindowRenderTarget(windowRenderTarget, getHandle(), args.width, args.height);

	canvasManager.initialize(device, { 720, 1280 });
	canvasManager.createLayer();
	canvasManager.clearLayer(0, 0xFFFFFF_rgb);

	width = args.width;
	height = args.height;

    InitGui();
}

void MainWindow::onKeyboard(VirtualKey key, bool state)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[(uint8)key] = state;

	if (!state)
		return;

	switch (key)
	{
		case VirtualKey('V'):
			canvasManager.centerView();
			break;

		case VirtualKey('S'):
			currentColorChangeTarget = nullptr;
			someParameterChangeTarget = nullptr;
			canvasManager.setInstrument_selection();
			break;

		case VirtualKey('P'):
		{
			someParameterChangeTarget = nullptr;
			auto &settings = canvasManager.setInstrument_pencil(currentColor);
			currentColorChangeTarget = &settings.color;
			break;
		}

		case VirtualKey('B'):
		{
			auto &settings = canvasManager.setInstrument_brush(currentColor, 5.0f);
			currentColorChangeTarget = &settings.color;
			someParameterChangeTarget = &settings.width;
			break;
		}

		case VirtualKey('L'):
		{
			auto &settings = canvasManager.setInstrument_line(currentColor, 5.0f);
			currentColorChangeTarget = &settings.color;
			someParameterChangeTarget = &settings.width;
			break;
		}

		case VirtualKey('F'):
		{
			currentColorChangeTarget = nullptr;
			auto &settings = canvasManager.setInstrument_brightnessContrastGammaFilter(0.0f, 1.0f, 5.0f);
			someParameterChangeTarget = &settings.gamma;
			break;
		}

		case VirtualKey('1'):
			currentColor = 0xFF0000_rgb;
			if (currentColorChangeTarget)
				*currentColorChangeTarget = currentColor;
			break;

		case VirtualKey('2'):
			currentColor = 0x00FF00_rgb;
			if (currentColorChangeTarget)
				*currentColorChangeTarget = currentColor;
			break;

		case VirtualKey('3'):
			currentColor = 0x0000FF_rgb;
			if (currentColorChangeTarget)
				*currentColorChangeTarget = currentColor;
			break;

		case VirtualKey('O'):
		{
			wchar filename[260];
			if (!OpenImageFileDialog(getHandle(), filename, countof(filename)))
				break;

			HeapPtr<byte> imageData;
			uint32 width = 0, height = 0;
			LoadImageFromFile(filename, imageData, width, height);

			uint32 dstWidth = min(width, canvasManager.getCanvasWidth());
			uint32 dstHeight = min(height, canvasManager.getCanvasHeight());

			rectu32 dstRegion(0, 0, dstWidth, dstHeight);

			canvasManager.uploadLayerRegion(0, dstRegion, imageData, width * 4);

			break;
		}

		case VirtualKey('Q'):
			canvasManager.resetSelection();
			break;

		case VirtualKey('K'):
			canvasManager.resizeSavingContents({ -50, -100, 1100, 700 }, 0x00FF00_rgb);
			break;

		case VirtualKey('C'):
			canvasManager.resizeSavingContents(canvasManager.getSelection());
			break;

		case VirtualKey::Up:
			if (someParameterChangeTarget)
			{
				*someParameterChangeTarget *= 1.1f;
				canvasManager.updateInstrumentSettings();
			}
			break;

		case VirtualKey::Down:
			if (someParameterChangeTarget)
			{
				*someParameterChangeTarget *= 0.9f;
				canvasManager.updateInstrumentSettings();
			}
			break;

		case VirtualKey::Enter:
			canvasManager.applyInstrument();
			break;

		case VirtualKey::Escape:
			canvasManager.resetInstrument();
			break;
	}
}

void MainWindow::onMouseButton(MouseState& mouseState, MouseButton button, bool state)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(mouseState.x, mouseState.y);
    io.MouseDown[0] = mouseState.leftButton;
    io.MouseDown[1] = mouseState.rightButton;
    io.MouseDown[2] = mouseState.middleButton;

    if (io.WantCaptureMouse) return;

	canvasManager.enablePointerPanViewMode(mouseState.rightButton);
	canvasManager.setPointerState(sint16x2(mouseState.x, mouseState.y), mouseState.leftButton);
}

void MainWindow::onMouseMove(MouseState& mouseState)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(mouseState.x, mouseState.y);
    io.MouseDown[0] = mouseState.leftButton;
    io.MouseDown[1] = mouseState.rightButton;
    io.MouseDown[2] = mouseState.middleButton;

    if (io.WantCaptureMouse) return;

	canvasManager.setPointerState(sint16x2(mouseState.x, mouseState.y), mouseState.leftButton);
}

void MainWindow::onMouseWheel(MouseState& mouseState, float32 delta)
{
	canvasManager.scaleView(1.0f - delta * 0.1f);
}

void MainWindow::onResize(ResizingArgs& args)
{
	device.resizeWindowRenderTarget(windowRenderTarget, args.width, args.height);

	width = args.width;
	height = args.height;

	updateAndRedraw();
}

void MainWindow::onCharacter(wchar character) {
    ImGuiIO& io = ImGui::GetIO();

    io.AddInputCharacter((unsigned short)character);
}

void Panter::MainWindow::openFile() {
    wchar filename[260];
    if (OpenImageFileDialog(getHandle(), filename, countof(filename))) {
        HeapPtr<byte> imageData;
        uint32 width = 0, height = 0;
        LoadImageFromFile(filename, imageData, width, height);

        rectu32 dstRegion(0, 0, width, height);

        for (int i = canvasManager.getLayerCount() - 1; i > 0; --i) {
            canvasManager.removeLayer((uint16)i);
            layerNames[i] = "";
        }
        layerNames[0] = "Layer 0";
        lastLayerNumber = 0;

        canvasManager.resizeDiscardingContents({width, height});
        canvasManager.uploadLayerRegion(canvasManager.getCurrentLayerId(), dstRegion, imageData, width * 4);

        currentFileName.assign(filename);

        std::wstring title = L"Panter - " + currentFileName;
        setTitle(title.c_str());
    }
}

void Panter::MainWindow::saveFileWithDialog()
{
	wchar filename[260];
	ImageFormat format;
	if (SaveImageFileDialog(getHandle(), filename, countof(filename), &format)) {
		currentFileName.assign(filename);
		currentFileImageFormat = format;
		saveCurrentFile();

		std::wstring title = L"Panter - " + currentFileName;
		setTitle(title.c_str());
	}
}

void Panter::MainWindow::saveCurrentFile()
{
	//Save to currentFileName
}

void MainWindow::updateAndRedraw()
{
	canvasManager.updateAndDraw(windowRenderTarget, { 0, 0, width, height });
    ProcessGui();
	windowRenderTarget.present();
}
