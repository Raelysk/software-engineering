#include "Panter.MainWindow.h"

#include <XLib.Program.h>
#include <XLib.Graphics.h>

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

void Program::Run()
{
	Device device;
	if (!device.initialize())
		return;

	MainWindow window(device);
	window.create(1280, 720, L"Panter - Untitled", true);

	while (window.isOpened())
	{
		WindowBase::DispatchPending();
		window.updateAndRedraw();
	}
}
