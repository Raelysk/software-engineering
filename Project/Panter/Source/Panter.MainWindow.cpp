#include "SystemUtil.h"

#include "Panter.MainWindow.h"

#include "Panter.ImageLoader.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

MainWindow::MainWindow(Device& device) : device(device) {}

void MainWindow::onCreate(CreationArgs& args)
{
	device.initialize();
	device.createWindowRenderTarget(windowRenderTarget, getHandle(), args.width, args.height);

	canvasManager.initialize(device, { 720, 1280 });
	canvasManager.createLayer();
	canvasManager.clearLayer(0, 0xFFFFFF_rgb);

	width = args.width;
	height = args.height;
}

void MainWindow::onKeyboard(VirtualKey key, bool state)
{
	switch (key)
	{
		case VirtualKey('C'):
			canvasManager.centerView();
			break;

		case VirtualKey('R'):
			canvasManager.resetSelection();
			break;

		case VirtualKey('S'):
			canvasManager.setInstrument(CanvasInstrument::Select);
			break;

		case VirtualKey('P'):
			canvasManager.setInstrument(CanvasInstrument::Pencil);
			break;

		case VirtualKey('B'):
			canvasManager.setInstrument(CanvasInstrument::Brush);
			break;

		case VirtualKey('1'):
			canvasManager.setColor(0xFF0000_rgb);
			break;

		case VirtualKey('2'):
			canvasManager.setColor(0x00FF00_rgb);
			break;

		case VirtualKey('3'):
			canvasManager.setColor(0x0000FF_rgb);
			break;

		case VirtualKey('O'):
		{
			wchar filename[260];
			if (!OpenFileDialog(getHandle(), filename, countof(filename)))
				break;

			HeapPtr<byte> imageData;
			uint32 width = 0, height = 0;
			ImageLoader::Load(filename, imageData, width, height);

			uint32 dstWidth = min(width, canvasManager.getCanvasWidth());
			uint32 dstHeight = min(height, canvasManager.getCanvasHeight());

			rectu32 dstRegion(0, 0, dstWidth, dstHeight);

			canvasManager.uploadLayerRegion(0, dstRegion, imageData, width * 4);

			break;
		}

		case VirtualKey('F'):
		{
			BrightnessContrastGammaFilterSettings settings;
			settings.brightness = 0.0f;
			settings.contrast = 1.0f;
			settings.gamma = 5.0f;

			canvasManager.brightnessContrastGammaFilter(true, settings);

			break;
		}
	}
}

void MainWindow::onMouseButton(MouseState& mouseState, MouseButton button, bool state)
{
	canvasManager.enablePointerPanViewMode(mouseState.rightButton);
	canvasManager.setPointerState(sint16x2(mouseState.x, mouseState.y), mouseState.leftButton);
}

void MainWindow::onMouseMove(MouseState& mouseState)
{
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
}

void MainWindow::updateAndRedraw()
{
	canvasManager.updateAndDraw(windowRenderTarget, { 0, 0, width, height });
	windowRenderTarget.present();
}
