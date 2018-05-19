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
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[(uint8)key] = state;

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
}

void MainWindow::onCharacter(wchar character) {
    ImGuiIO& io = ImGui::GetIO();

    io.AddInputCharacter((unsigned short)character);
}

void Panter::MainWindow::openFile() {
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

void Panter::MainWindow::InitGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplDX11_Init(getHandle(), device.getD11Device(), device.getD11DeviceContext());
    ImGui::StyleColorsDark();

    ImGuiStyle* style = &ImGui::GetStyle();
    style->Alpha = 0.8f;
    style->WindowRounding = 5.0f;
    style->ChildRounding = 5.0f;
    style->FramePadding = ImVec2(2.0f, 2.0f);
    style->FrameRounding = 4.0f;
    style->ItemSpacing = ImVec2(6.0f, 6.0f);
    style->ItemInnerSpacing = ImVec2(2.0f, 2.0f);
    style->GrabMinSize = 15.0f;
    style->GrabRounding = 3.0f;

    for (int i = 0; i < IM_ARRAYSIZE(colorPalette); i++) {
        ImGui::ColorConvertHSVtoRGB((float)i / 31.0f, 0.8f, 0.8f, colorPalette[i].x, colorPalette[i].y, colorPalette[i].z);
        colorPalette[i].w = 1.0f; // Alpha
    }
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

    const uint16 widgetsXOffset = 5;
    const uint16 widgetsYOffset = 25;

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    openFile();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {}
                if (ImGui::MenuItem("Save As..")) {}
                ImGui::Separator();
                if (ImGui::BeginMenu("Options")) {
                    
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Center view")) {
                    canvasManager.centerView();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Filters")) {
                if (ImGui::MenuItem("Brightness contrast gamma") && currentInstrument != Instrument::BrightnessContrastGammaFilter) {
                    canvasManager.setInstrument_brightnessContrastGammaFilter(0.0f, 1.0f, 1.0f);
                }
                if (ImGui::MenuItem("Gaussian Blur")) {
                
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Layers")) {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    {
        ImGui::SetNextWindowPos(ImVec2(widgetsXOffset, widgetsYOffset), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always); //-1 in size means adjust to components size
        ImGui::Begin("Instruments", &showMain, windowFlags);
        if (ImGui::Button("Select", ImVec2(buttonSize, buttonSize)) && currentInstrument != Instrument::Selection) {
            canvasManager.setInstrument_selection();
        }
        if (ImGui::Button("Pencil", ImVec2(buttonSize, buttonSize)) && currentInstrument != Instrument::Pencil) {
            canvasManager.setInstrument_pencil(toRGB(mainColor));
        }
        if (ImGui::Button("Brush", ImVec2(buttonSize, buttonSize)) && currentInstrument != Instrument::Brush) {
            canvasManager.setInstrument_brush(toRGBA(mainColor), 1.0f);
        }
        if (ImGui::Button("Eraser", ImVec2(buttonSize, buttonSize))) {

        }
        if (ImGui::Button("Line", ImVec2(buttonSize, buttonSize))) {

        }
        if (ImGui::Button("Rectangle", ImVec2(buttonSize, buttonSize))) {

        }
        if (ImGui::Button("Ellipse", ImVec2(buttonSize, buttonSize))) {

        }

        ImGui::End();
    }

    currentInstrument = canvasManager.getCanvasInstrument();

    {
        const uint16 windowXSize = 256;
        const uint16 colorButtonSize = 65;
        const uint16 colorPaletteButtonSize = 25;

        uint16 windowXOffset = width - windowXSize;

        ImGui::SetNextWindowPos(ImVec2(windowXOffset - widgetsXOffset, widgetsYOffset), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowXSize, -1), ImGuiCond_Always);
        ImGui::Begin("Color picker", &showColorPicker, windowFlags);

        ImGui::BeginGroup();
        
        if (ImGui::ColorButton("Main color", mainColor, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(colorButtonSize, colorButtonSize))) {
            isMainColorPickerChoosen = true;
        }

        if (ImGui::ColorButton("Secondary color", secondaryColor, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(colorButtonSize, colorButtonSize))) {
            isMainColorPickerChoosen = false;
        }
        
        ImGui::EndGroup();
        ImGui::SameLine();

        ImVec4* changedColor = isMainColorPickerChoosen ? &mainColor : &secondaryColor;
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview;
        ImGui::ColorPicker4("##picker", (float*)changedColor, flags);
        
        ImGui::Separator();

        ImGui::BeginGroup();
        for (int i = 0; i < IM_ARRAYSIZE(colorPalette); i++) {
            ImGui::PushID(i);
            if ((i % 8) != 0)
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);
            if (ImGui::ColorButton("##palette", colorPalette[i], ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip, ImVec2(colorPaletteButtonSize, colorPaletteButtonSize)))
                *changedColor = ImVec4(colorPalette[i].x, colorPalette[i].y, colorPalette[i].z, changedColor->w); // Preserve alpha!

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                    memcpy((float*)&colorPalette[i], payload->Data, sizeof(float) * 3);
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                    memcpy((float*)&colorPalette[i], payload->Data, sizeof(float) * 4);
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }
        ImGui::EndGroup();

        if (currentInstrument != Instrument::None) {
            ImGui::Separator();

            ImGui::Text(kInstrumentNames[currentInstrument].c_str());

            if (currentInstrument == Instrument::Selection) {
                ImGui::Button("Crop", ImVec2(buttonSize, buttonSize));
            } else if (currentInstrument == Instrument::Brush) {
                auto& settings = canvasManager.getInstrumentSettings_brush();

                ImGui::SliderFloat("Line Width", &settings.width, 1.0f, 40.0f);

                settings.color = toRGBA(mainColor);

                canvasManager.updateInstrumentSettings();
            } else if (currentInstrument == Instrument::Pencil) {
                auto& settings = canvasManager.getInstrumentSettings_pencil();

                settings.color = toRGB(mainColor);

                canvasManager.updateInstrumentSettings();
            } else if (currentInstrument == Instrument::BrightnessContrastGammaFilter) {
                auto& settings = canvasManager.getInstrumentSettings_brightnessContrastGammaFilter();

                ImGui::SliderFloat("Brightness", &settings.brightness, 0.0f, 1.0f);
                ImGui::SliderFloat("Contrast", &settings.contrast, 0.0f, 10.0f);
                ImGui::SliderFloat("Gamma", &settings.gamma, 0.0f, 10.0f);

                canvasManager.updateInstrumentSettings();

                if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
                    canvasManager.applyInstrument();
                }
            }
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
