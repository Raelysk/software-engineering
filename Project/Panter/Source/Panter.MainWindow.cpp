#include "SystemUtil.h"

#include "Panter.MainWindow.h"
#include "Panter.ImageLoader.h"

#include "imgui\imgui_impl_dx11.h"
//#include <Windows.h>

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

    InitGui();
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

void Panter::MainWindow::InitGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplDX11_Init(getHandle(), device.getD11Device(), device.getD11DeviceContext());
    ImGui::StyleColorsDark();
}

void Panter::MainWindow::ProcessGui() {
    float buttonSize = min(0.07f * (float)width, 0.07f * (float)height);
    CanvasInstrument currentInstrument = canvasManager.getCanvasInstrument();
    XLib::Color currentColor = canvasManager.getColor();
    ImVec4 mainColor = ImVec4((float)currentColor.r / 255.0f, (float)currentColor.g / 255.0f,
                        (float)currentColor.b / 255.0f, (float)currentColor.a / 255.0f);

    ImGui_ImplDX11_NewFrame();

    ImGuiWindowFlags windowFlags = 0;
    windowFlags |= ImGuiWindowFlags_NoTitleBar;
    windowFlags |= ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoScrollbar;
    windowFlags |= ImGuiWindowFlags_NoResize;
    windowFlags |= ImGuiWindowFlags_NoCollapse;

    static bool showMain = true;
    static bool showInstrument = true;
    static bool showInstrumentProperties = true;
    static bool showMainColorPicker = true;
    static bool showFilterOptions = false;
    static BrightnessContrastGammaFilterSettings settings = {0.0f, 0.0f, 0.0f};

    {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always); //-1 in size means adjust to components size
        ImGui::Begin("Main", &showMain, windowFlags);

        if (ImGui::Button("File", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
            ImGui::OpenPopup("File");
        }
        
        if (ImGui::BeginPopup("File")) {
            //if (ImGui::Button("New", ImVec2(buttonSize * 1.0f, buttonSize * 0.5f))) {
            //
            //}

            if (ImGui::Button("Open", ImVec2(buttonSize * 1.0f, buttonSize * 0.5f))) {
                wchar filename[260];
                if (OpenFileDialog(getHandle(), filename, countof(filename))) {
                    HeapPtr<byte> imageData;
                    uint32 width = 0, height = 0;
                    ImageLoader::Load(filename, imageData, width, height);

                    uint32 dstWidth = min(width, canvasManager.getCanvasWidth());
                    uint32 dstHeight = min(height, canvasManager.getCanvasHeight());

                    rectu32 dstRegion(0, 0, dstWidth, dstHeight);

                    canvasManager.uploadLayerRegion(0, dstRegion, imageData, width * 4);
                }
            }

            //if (ImGui::Button("Save", ImVec2(buttonSize * 1.0f, buttonSize * 0.5f))) {
            //
            //}

            ImGui::EndPopup();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Tools", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
            ImGui::OpenPopup("Tools");
        }
        if (ImGui::BeginPopup("Tools")) {
            if (ImGui::Button("Center view", ImVec2(buttonSize * 2.0f, buttonSize * 0.5f))) {
                canvasManager.centerView();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Filters", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
            ImGui::OpenPopup("Filters");
        }
        if (ImGui::BeginPopup("Filters")) {
            if (ImGui::Button("Brightness Contrast Gamma", ImVec2(buttonSize * 4.0f, buttonSize * 0.5f))) {
                showFilterOptions = true;
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Button("Layer", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f));
        
        ImGui::SameLine();
        ImGui::Button("Help", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f));

        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0, buttonSize * 1.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always);
        ImGui::Begin("Instruments", &showInstrument, windowFlags);

        if (ImGui::Button("Select", ImVec2(buttonSize * 1.5f, buttonSize))) {
            canvasManager.setInstrument(CanvasInstrument::Select);
        }
        
        if (ImGui::Button("Pencil", ImVec2(buttonSize * 1.5f, buttonSize))) {
            canvasManager.setInstrument(CanvasInstrument::Pencil);
        }
        
        if (ImGui::Button("Brush", ImVec2(buttonSize * 1.5f, buttonSize))) {
            canvasManager.setInstrument(CanvasInstrument::Brush);
        }

        ImGui::End();
    }

    if (currentInstrument != CanvasInstrument::None) {
        ImGui::SetNextWindowPos(ImVec2(buttonSize * 2.0f, buttonSize * 1.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always);
        ImGui::Begin("Instrument properties", &showInstrumentProperties, windowFlags);

        ImGui::Text("Main color: ");
        ImGui::SameLine();
        if (ImGui::ColorButton("Main color", mainColor)) {
            ImGui::OpenPopup("Main color picker");
        }
        if (ImGui::BeginPopup("Main color picker")) {
            ImGui::ColorPicker3("Main color", (float*)&mainColor);
            canvasManager.setColor(XLib::Color(mainColor.x * 255.0f, mainColor.y * 255.0f, mainColor.z * 255.0f, 255));
            //ImGui::ColorPicker4("Main color", (float*)&mainColor);
            //canvasManager.setColor(XLib::Color(mainColor.x * 255.0f, mainColor.y * 255.0f, mainColor.z * 255.0f, mainColor.w * 255.0f));
            ImGui::EndPopup();
        }

        if (currentInstrument == CanvasInstrument::Brush || currentInstrument == CanvasInstrument::Pencil) {
            static float lineWidth = 1.0f;
            ImGui::SliderFloat("Line width", &lineWidth, 1.0f, 10.0f);
        } else if (currentInstrument == CanvasInstrument::Select) {
            ImGui::Button("Crop", ImVec2(buttonSize, buttonSize));
        }

        ImGui::End();
    }

    if (showFilterOptions) {
        ImGui::SetNextWindowPos(ImVec2(0.4f * width, 0.4f * height), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always);
        ImGui::Begin("Brightness Contrast Gamma Filter options", &showInstrumentProperties);

        ImGui::InputFloat("Brightness", &settings.brightness);
        ImGui::InputFloat("Contrast", &settings.contrast);
        ImGui::InputFloat("Gamma", &settings.gamma);

        if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
            canvasManager.brightnessContrastGammaFilter(true, settings);
            settings = {0.0f, 0.0f, 0.0f};
            showFilterOptions = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonSize, buttonSize * 0.5f))) {
            showFilterOptions = false;
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void MainWindow::updateAndRedraw()
{
	canvasManager.updateAndDraw(windowRenderTarget, { 0, 0, width, height });
    ProcessGui();
	windowRenderTarget.present();
}
