#pragma once

#include <string>

#include "imgui\imgui.h"

#include <XLib.Types.h>
#include <XLib.System.Window.h>
#include <XLib.Graphics.h>

#include "Panter.CanvasManager.h"

#include "FileUtil.h"

namespace Panter
{
	class MainWindow : public XLib::WindowBase
	{
	private: // data
		XLib::Graphics::Device& device;
		XLib::Graphics::WindowRenderTarget windowRenderTarget;
		uint16 width = 0, height = 0;

		XLib::Color currentColor = 0;
		XLib::Color *currentColorChangeTarget = nullptr;
		float32 *someParameterChangeTarget = nullptr;

		CanvasManager canvasManager;
        
		std::wstring currentFileName = L"";
		ImageFormat currentFileImageFormat = ImageFormat::None;

        ImVec4 mainColor = {0.0f, 0.0f, 0.0f, 1.0f};
        ImVec4 secondaryColor = {1.0f, 1.0f, 1.0f, 1.0f};

        ImVec4 colorPalette[32];

        bool isMainColorPickerChoosen = true;

        std::string layerNames[16] = {"Layer 0"};
		bool enableLayer[16] = { true };
        uint16 lastLayerNumber = 0;

		bool openResizeWindow = false;
		int resizeWidth = 0;
		int resizeHeight = 0;
		int resizeXOffset = 0;
		int resizeYOffset = 0;

		bool openCreateWindow = false;
		int createWidth = 0;
		int createHeight = 0;

	private: // code
        virtual void onCreate(XLib::CreationArgs& args) override;
        virtual void onKeyboard(XLib::VirtualKey key, bool state) override;
		virtual void onMouseButton(XLib::MouseState& mouseState, XLib::MouseButton button, bool state) override;
		virtual void onMouseMove(XLib::MouseState& mouseState) override;
		virtual void onMouseWheel(XLib::MouseState& mouseState, float32 delta) override;
		virtual void onResize(XLib::ResizingArgs& args) override;
        virtual void onCharacter(wchar character) override;

        void openFile();
		void saveFileWithDialog();
		void saveCurrentFile();

        void InitGui();
        void ProcessGui();

	public:
		MainWindow(XLib::Graphics::Device& device);
		~MainWindow() = default;

		void updateAndRedraw();
	};
}
