#include <map>

#include "SystemUtil.h"

#include "Panter.MainWindow.h"
#include "Panter.ImageLoader.h"

#include "imgui\imgui_impl_dx11.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

inline ImVec4 toImVec(const XLib::Color& color) {
    return ImVec4((float)color.r / 255.0f, (float)color.g / 255.0f,
        (float)color.b / 255.0f, (float)color.a / 255.0f);
}

inline XLib::Color toRGB(const ImVec4& color) {
    return XLib::Color(color.x * 255.0f, color.y * 255.0f, color.z * 255.0f, 255);
}

inline XLib::Color toRGBA(const ImVec4& color) {
    return XLib::Color(color.x * 255.0f, color.y * 255.0f, color.z * 255.0f, color.w * 255.0f);
}

std::map<Instrument, std::string> kInstrumentNames = {
    {Instrument::None, "None"},
    {Instrument::Selection, "Selection"},
    {Instrument::Pencil, "Pencil"},
    {Instrument::Brush, "Brush"},
    {Instrument::BrightnessContrastGammaFilter, "Brightness Contrast Gamma Filter"},
    {Instrument::GaussianFilter, "Gaussian Blur Filter"},
};


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
	if (!state)
		return;

	switch (key)
	{
		case VirtualKey('C'):
			canvasManager.centerView();
			break;

		case VirtualKey('R'):
			canvasManager.resetSelection();
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

		case VirtualKey('Q'):
			canvasManager.resetSelection();
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
    Instrument currentInstrument = canvasManager.getCanvasInstrument();

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
    static bool showColorPicker = true;
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

        if (ImGui::Button("Select", ImVec2(buttonSize * 1.5f, buttonSize)) && currentInstrument != Instrument::Selection) {
            canvasManager.setInstrument_selection();
        }
        
        if (ImGui::Button("Pencil", ImVec2(buttonSize * 1.5f, buttonSize)) && currentInstrument != Instrument::Pencil) {
            canvasManager.setInstrument_pencil(toRGB(mainColor));
        }
        
        if (ImGui::Button("Brush", ImVec2(buttonSize * 1.5f, buttonSize)) && currentInstrument != Instrument::Brush) {
            canvasManager.setInstrument_brush(toRGBA(mainColor), 1.0f);
        }

        if (ImGui::Button("Filters", ImVec2(buttonSize * 1.5f, buttonSize))) {
            ImGui::OpenPopup("Filters");
        }
        if (ImGui::BeginPopup("Filters")) {
            if (ImGui::Button("Brightness Contrast Gamma", ImVec2(buttonSize * 4.0f, buttonSize * 0.5f)) && currentInstrument != Instrument::BrightnessContrastGammaFilter) {
                canvasManager.setInstrument_brightnessContrastGammaFilter(0.0f, 0.0f, 0.0f);
            }
            if (ImGui::Button("Gaussian Blur", ImVec2(buttonSize * 4.0f, buttonSize * 0.5f)) && currentInstrument != Instrument::GaussianFilter) {
                //canvasManager.setInstrument_gaussianBlurFilter();
            }
            ImGui::EndPopup();
        }

        currentInstrument = canvasManager.getCanvasInstrument();

        ImGui::End();
    }

    if (currentInstrument != Instrument::None) {
        ImGui::SetNextWindowPos(ImVec2(buttonSize * 2.0f, buttonSize * 1.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width * 0.2f, -1), ImGuiCond_Always);

        std::string title = kInstrumentNames[currentInstrument] + " properties";
        ImGui::Begin(title.c_str(), &showInstrumentProperties, windowFlags & ~ImGuiWindowFlags_NoTitleBar);

        if (currentInstrument == Instrument::Selection) {
            ImGui::Button("Crop", ImVec2(buttonSize, buttonSize));
        } else if (currentInstrument == Instrument::Brush) {
            auto& settings = canvasManager.getInstrumentSettings_brush();

            ImGui::InputFloat("Width", &settings.width);
            
            settings.color = toRGBA(mainColor);

            canvasManager.updateInstrumentSettings();
        } else if (currentInstrument == Instrument::Pencil) {
            auto& settings = canvasManager.getInstrumentSettings_pencil();
            
            settings.color = toRGB(mainColor);

            canvasManager.updateInstrumentSettings();
        } else if (currentInstrument == Instrument::BrightnessContrastGammaFilter) {
            auto& settings = canvasManager.getInstrumentSettings_brightnessContrastGammaFilter();

            ImGui::InputFloat("Brightness", &settings.brightness);
            ImGui::InputFloat("Contrast", &settings.contrast);
            ImGui::InputFloat("Gamma", &settings.gamma);

            canvasManager.updateInstrumentSettings();

            if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
                canvasManager.applyInstrument();
            }
        }
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0, 0.6f * height), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always);
        ImGui::Begin("Color picker", &showColorPicker, windowFlags);

        if (ImGui::ColorButton("Main color", mainColor)) {
            isMainColorPickerChoosen = true;
        }
        ImGui::SameLine();
        if (ImGui::ColorButton("Secondary color", secondaryColor)) {
            isMainColorPickerChoosen = false;
        }

        ImVec4* changedColor = isMainColorPickerChoosen ? &mainColor : &secondaryColor;

        ImGui::ColorPicker4(isMainColorPickerChoosen? "Main" : "Second", (float*)changedColor);

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
