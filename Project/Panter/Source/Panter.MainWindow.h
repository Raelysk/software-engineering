#pragma once

#include <XLib.Types.h>
#include <XLib.System.Window.h>
#include <XLib.Graphics.h>

#include "Panter.CanvasManager.h"

#include "imgui\imgui.h"

namespace Panter
{
	class MainWindow : public XLib::WindowBase
	{
	private: // data
		XLib::Graphics::Device& device;
		XLib::Graphics::WindowRenderTarget windowRenderTarget;
		uint16 width = 0, height = 0;

		CanvasManager canvasManager;
        
	private: // code
        virtual void onCreate(XLib::CreationArgs& args) override;
        virtual void onKeyboard(XLib::VirtualKey key, bool state) override;
		virtual void onMouseButton(XLib::MouseState& mouseState, XLib::MouseButton button, bool state) override;
		virtual void onMouseMove(XLib::MouseState& mouseState) override;
		virtual void onMouseWheel(XLib::MouseState& mouseState, float32 delta) override;
		virtual void onResize(XLib::ResizingArgs& args) override;

        void InitGui();
        void ProcessGui();

	public:
		MainWindow(XLib::Graphics::Device& device);
		~MainWindow() = default;

		void updateAndRedraw();
	};
}
