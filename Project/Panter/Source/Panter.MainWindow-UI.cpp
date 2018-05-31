#include <map>

#include "Panter.MainWindow.h"

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

std::map<Instrument, const char*> kInstrumentNames = {
	{ Instrument::None, "None" },
	{ Instrument::Selection, "Selection" },
	{ Instrument::Pencil, "Pencil" },
	{ Instrument::Brush, "Brush" },
	{ Instrument::Line, "Line" },
	{ Instrument::BrightnessContrastGammaFilter, "Brightness Contrast Gamma Filter" },
	{ Instrument::GaussianBlurFilter, "Gaussian Blur Filter" },
	{ Instrument::SharpenFilter, "Sharpen Filter" },
};


void Panter::MainWindow::InitGui() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplDX11_Init(getHandle(), device.getD3Device(), device.getD3DeviceContext());
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
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;

	static bool showInstrument = true;
	static bool showInstrumentProperties = true;
	static bool showLayers = true;
	static bool showNewCanvasProperties = true;

	const uint16 widgetsXOffset = 5;
	const uint16 widgetsYOffset = 25;

	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New")) {
					openCreateWindow = true;
					createWidth = canvasManager.getCanvasWidth();
					createHeight = canvasManager.getCanvasHeight();
				}
				if (ImGui::MenuItem("Open")) {
					openFile();
				}
				if (ImGui::MenuItem("Save")) {
					if (currentFileName.empty()) {
						saveFileWithDialog();
					}
					else {
						saveCurrentFile();
					}
				}
				if (ImGui::MenuItem("Save As...")) {
					saveFileWithDialog();
				}

				/*ImGui::Separator();
				if (ImGui::BeginMenu("Options")) {

					ImGui::EndMenu();
				}*/
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools")) {
				if (ImGui::MenuItem("Center view")) {
					canvasManager.centerView();
				}
				if (ImGui::MenuItem("Reset selection")) {
					canvasManager.resetSelection();
				}
				if (ImGui::MenuItem("Resize")) {
					openResizeWindow = true;
					resizeWidth = canvasManager.getCanvasWidth();
					resizeHeight = canvasManager.getCanvasHeight();
					resizeXOffset = 0;
					resizeYOffset = 0;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Filters")) {
				if (ImGui::MenuItem("Brightness contrast gamma") && currentInstrument != Instrument::BrightnessContrastGammaFilter) {
					canvasManager.setInstrument_brightnessContrastGammaFilter();
				}
				if (ImGui::MenuItem("Gaussian Blur")) {
					canvasManager.setInstrument_gaussianBlurFilter();
				}
				if (ImGui::MenuItem("Sharpen")) {
					canvasManager.setInstrument_sharpenFilter();
				}
				ImGui::EndMenu();
			}
			/*
			if (ImGui::MenuItem("Help")) {
				//Show help
			}
			*/
			{
				float32x2 pointerPosition = canvasManager.getCanvasSpacePointerPosition();
				ImGui::SetCursorPosX(width - 200.0f);
				ImGui::Text("%d,%d", int(pointerPosition.x), int(pointerPosition.y));
				ImGui::SetCursorPosX(width - 130.0f);
				ImGui::Text("%3d%%", int(canvasManager.getCanvasScale() * 100.0f));
				ImGui::SetCursorPosX(width - 80.0f);
				ImGui::Text("%dx%d", canvasManager.getCanvasWidth(), canvasManager.getCanvasHeight());
			}

			ImGui::EndMainMenuBar();
		}
	}

	{
		ImGui::SetNextWindowPos(ImVec2(widgetsXOffset, widgetsYOffset), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_Always); //-1 in size means adjust to components size
		ImGui::Begin("Instruments", &showInstrument, windowFlags);
		if (ImGui::Button("Select", ImVec2(buttonSize, buttonSize)) && currentInstrument != Instrument::Selection) {
			canvasManager.setInstrument_selection();
		}
		if (ImGui::Button("Pencil", ImVec2(buttonSize, buttonSize)) && currentInstrument != Instrument::Pencil) {
			canvasManager.setInstrument_pencil(toRGB(mainColor));
		}
		if (ImGui::Button("Brush", ImVec2(buttonSize, buttonSize))) {
			if (currentInstrument != Instrument::Brush || !canvasManager.getInstrumentSettings_brush().blendEnabled) {
				canvasManager.setInstrument_brush(toRGBA(mainColor));
			}
		}
		if (ImGui::Button("Eraser", ImVec2(buttonSize, buttonSize))) {
			if (currentInstrument != Instrument::Brush || canvasManager.getInstrumentSettings_brush().blendEnabled) {
				canvasManager.setInstrument_brush(toRGBA(secondaryColor), 5.0f, false);
			}
		}
		if (ImGui::Button("Line", ImVec2(buttonSize, buttonSize))) {
			canvasManager.setInstrument_line(toRGBA(mainColor));
		}
		if (ImGui::Button("Rectangle", ImVec2(buttonSize, buttonSize))) {
			if (currentInstrument != Instrument::Shape || canvasManager.getInstrumentSettings_shape().shape != Shape::Rectangle) {
				canvasManager.setInstrument_shape(toRGBA(secondaryColor), toRGBA(mainColor), 5.0f, Shape::Rectangle);
			}
		}
		if (ImGui::Button("Ellipse", ImVec2(buttonSize, buttonSize))) {
			if (currentInstrument != Instrument::Shape || canvasManager.getInstrumentSettings_shape().shape != Shape::Circle) {
				canvasManager.setInstrument_shape(toRGBA(secondaryColor), toRGBA(mainColor), 5.0f, Shape::Circle);
			}
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
		ImGui::Begin("Instrument properties", &showInstrumentProperties, windowFlags);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);

		ImGui::BeginGroup();
		if (ImGui::ColorButton("Main color", mainColor, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(colorButtonSize, colorButtonSize))) {
			isMainColorPickerChoosen = true;
		}
		if (ImGui::ColorButton("Secondary color", secondaryColor, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(colorButtonSize, colorButtonSize))) {
			isMainColorPickerChoosen = false;
		}

		if (isMainColorPickerChoosen) {
			ImGui::Text("Main");
		} else {
			ImGui::Text("Secondary");
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

		ImGui::PopStyleVar();

		if (currentInstrument != Instrument::None) {
			ImGui::Separator();

			if (currentInstrument == Instrument::Selection) {
				ImGui::Text(kInstrumentNames[Instrument::Selection]);

				if (ImGui::Button("Crop", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.resizeSavingContents(canvasManager.getSelection());
				}
			}
			else if (currentInstrument == Instrument::Brush) {
				auto& settings = canvasManager.getInstrumentSettings_brush();
				
				if (settings.blendEnabled) {
					ImGui::Text(kInstrumentNames[Instrument::Brush]);
				} else {
					ImGui::Text("Eraser");
				}

				bool updateSettings = false;

				XLib::Color color = settings.blendEnabled ? toRGBA(mainColor) : toRGBA(secondaryColor);

				updateSettings |= ImGui::SliderFloat("Line Width", &settings.width, 1.0f, 40.0f);
				updateSettings |= (settings.color != color);
				settings.color = color;

				if (updateSettings) canvasManager.updateInstrumentSettings();
			}
			else if (currentInstrument == Instrument::Pencil) {
				ImGui::Text(kInstrumentNames[Instrument::Pencil]);

				auto& settings = canvasManager.getInstrumentSettings_pencil();
				bool updateSettings = false;

				XLib::Color color = toRGB(mainColor);

				updateSettings |= (settings.color != color);
				settings.color = color;

				if (updateSettings) canvasManager.updateInstrumentSettings();
			}
			else if (currentInstrument == Instrument::Line) {
				ImGui::Text(kInstrumentNames[Instrument::Line]);

				auto& settings = canvasManager.getInstrumentSettings_line();
				bool updateSettings = false;

				XLib::Color color = toRGBA(mainColor);

				updateSettings |= ImGui::SliderFloat("Line Width", &settings.width, 1.0f, 40.0f);

				updateSettings |= ImGui::Checkbox("Rounded Start", &settings.roundedStart);
				updateSettings |= ImGui::Checkbox("Rounded End", &settings.roundedEnd);
				updateSettings |= (settings.color != color);

				settings.color = color;

				if (updateSettings) canvasManager.updateInstrumentSettings();

				if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.applyInstrument();
				}
			} 
			else if (currentInstrument == Instrument::Shape) {

				auto& settings = canvasManager.getInstrumentSettings_shape();
				if (settings.shape == Shape::Rectangle) {
					ImGui::Text("Rectangle");
				} else if (settings.shape == Shape::Circle) {
					ImGui::Text("Ellipse");
				}

				bool updateSettings = false;

				XLib::Color borderColor = toRGBA(mainColor);
				XLib::Color fillColor = toRGBA(secondaryColor);

				updateSettings |= ImGui::SliderFloat("Border Width", &settings.borderWidth, 1.0f, 40.0f);

				updateSettings |= (settings.borderColor != borderColor);
				updateSettings |= (settings.fillColor != fillColor);

				settings.borderColor = borderColor;
				settings.fillColor = fillColor;

				if (updateSettings) canvasManager.updateInstrumentSettings();

				if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.applyInstrument();
				}
			}
			else if (currentInstrument == Instrument::BrightnessContrastGammaFilter) {
				ImGui::Text(kInstrumentNames[Instrument::BrightnessContrastGammaFilter]);

				auto& settings = canvasManager.getInstrumentSettings_brightnessContrastGammaFilter();
				bool updateSettings = false;

				updateSettings |= ImGui::SliderFloat("Brightness", &settings.brightness, 0.0f, 1.0f);
				updateSettings |= ImGui::SliderFloat("Contrast", &settings.contrast, 0.0f, 10.0f);
				updateSettings |= ImGui::SliderFloat("Gamma", &settings.gamma, 0.0f, 10.0f);

				if (updateSettings) canvasManager.updateInstrumentSettings();

				if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.applyInstrument();
				}
			}
			else if (currentInstrument == Instrument::GaussianBlurFilter) {
				ImGui::Text(kInstrumentNames[Instrument::GaussianBlurFilter]);

				auto& settings = canvasManager.getInstrumentSettings_gaussianBlurFilter();
				bool updateSettings = false;

				updateSettings |= ImGui::SliderInt("Radius", (int*)&settings.radius, 1, 16);

				if (updateSettings) canvasManager.updateInstrumentSettings();

				if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.applyInstrument();
				}
			}
			else if (currentInstrument == Instrument::SharpenFilter) {
				ImGui::Text(kInstrumentNames[Instrument::SharpenFilter]);

				auto& settings = canvasManager.getInstrumentSettings_sharpenFilter();
				bool updateSettings = false;

				updateSettings |= ImGui::SliderFloat("Intensity", &settings.intensity, 0.0f, 1.0f);
				
				if (updateSettings) canvasManager.updateInstrumentSettings();

				if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
					canvasManager.applyInstrument();
				}
			}
			else {
				ImGui::Text(kInstrumentNames[Instrument::None]);
			}
		}

		ImGui::End();
	}

	{
		ImGui::SetNextWindowPos(ImVec2(width * 0.85f - widgetsXOffset, height * 0.7f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width * 0.15f, height * 0.3f), ImGuiCond_Always);
		ImGui::Begin("Layers", &showLayers, windowFlags);

		uint16 numberOfLayers = canvasManager.getLayerCount();
		uint16 currentLayerId = canvasManager.getCurrentLayerId();

		// New / Layer up /Layer down /Delete
		if (ImGui::Button("Add", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f)) && numberOfLayers < 16) {
			canvasManager.createLayer();

			numberOfLayers = canvasManager.getLayerCount();
			currentLayerId = numberOfLayers - 1;
			canvasManager.setCurrentLayer(currentLayerId);

			canvasManager.clearLayer(currentLayerId, 0xFFFFFF00_rgba);

			layerNames[currentLayerId] = "Layer " + std::to_string(++lastLayerNumber);
			enableLayer[currentLayerId] = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f)) && numberOfLayers > 1) {
			canvasManager.removeLayer(currentLayerId);

			for (int i = currentLayerId; i < numberOfLayers - 1; ++i) {
				layerNames[i] = std::move(layerNames[i + 1]);
				enableLayer[i] = enableLayer[i + 1];
			}

			numberOfLayers = canvasManager.getLayerCount();
			currentLayerId = canvasManager.getCurrentLayerId();
		}
		if (ImGui::Button("Clear", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
			canvasManager.clearLayer(currentLayerId, toRGBA(secondaryColor));
		}
		ImGui::Separator();

		ImGui::BeginGroup();

		for (int i = numberOfLayers - 1; i >= 0; --i) {
			ImGui::PushID(i);

			if (ImGui::Checkbox("##checkbox", &enableLayer[i])) {
				canvasManager.enableLayer(i, enableLayer[i]);
			}
			ImGui::SameLine();

			if (ImGui::Selectable("", (i == currentLayerId))) {
				currentLayerId = i;
				canvasManager.setCurrentLayer(currentLayerId);
			}
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::Text(layerNames[i].c_str());
		}

		ImGui::EndGroup();

		ImGui::Separator();
		if (ImGui::Button("Up", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
			if (currentLayerId != numberOfLayers - 1) {
				/*
				canvasManager.moveLayer(currentLayerId, currentLayerId + 1);
				std::swap(layerNames[currentLayerId], layerNames[currentLayerId + 1]);
				std::swap(enableLayer[currentLayerId], enableLayer[currentLayerId + 1]);
				++currentLayerId;
				canvasManager.setCurrentLayer(currentLayerId);
				*/
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Down", ImVec2(buttonSize * 1.5f, buttonSize * 0.5f))) {
			if (currentLayerId != 0) {
				/*
				canvasManager.moveLayer(currentLayerId, currentLayerId - 1);
				std::swap(layerNames[currentLayerId], layerNames[currentLayerId - 1]);
				std::swap(enableLayer[currentLayerId], enableLayer[currentLayerId - 1]);
				--currentLayerId;
				canvasManager.setCurrentLayer(currentLayerId);
				*/
			}
		}

		ImGui::End();
	}

	if (openResizeWindow) {
		ImGui::SetNextWindowPos(ImVec2(width * 0.4f, height * 0.4f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width * 0.2f, -1), ImGuiCond_Always);
		ImGui::Begin("Resize", &openResizeWindow, windowFlags & ~ImGuiWindowFlags_NoTitleBar);

		ImGui::Text("New canvas size:");
		if (ImGui::InputInt("Width", &resizeWidth)) {
			if (resizeWidth < 1) {
				resizeWidth = 1;
			}
		}
		if (ImGui::InputInt("Height", &resizeHeight)) {
			if (resizeHeight < 1) {
				resizeHeight = 1;
			}
		}

		ImGui::Text("Source offset:");
		if (ImGui::InputInt("X offset", &resizeXOffset)) {
		}
		if (ImGui::InputInt("Y offset", &resizeYOffset)) {
		}

		if (ImGui::Button("Apply", ImVec2(buttonSize, buttonSize * 0.5f))) {
			canvasManager.resizeSavingContents({resizeXOffset, resizeYOffset, resizeWidth, resizeHeight }, toRGBA(secondaryColor));
			setTitle(L"Panter");
			openResizeWindow = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonSize, buttonSize * 0.5f))) {
			openResizeWindow = false;
		}

		ImGui::End();
	}

	if (openCreateWindow) {
		ImGui::SetNextWindowPos(ImVec2(width * 0.4f, height * 0.4f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width * 0.2f, -1), ImGuiCond_Always);
		ImGui::Begin("New canvas", &openCreateWindow, windowFlags & ~ImGuiWindowFlags_NoTitleBar);

		ImGui::Text("Canvas size:");
		if (ImGui::InputInt("Width", &createWidth)) {
			if (createWidth < 1) {
				createWidth = 1;
			}
		}
		if (ImGui::InputInt("Height", &createHeight)) {
			if (createHeight < 1) {
				createHeight = 1;
			}
		}

		if (ImGui::Button("Create", ImVec2(buttonSize, buttonSize * 0.5f))) {
			for (int i = canvasManager.getLayerCount() - 1; i > 0; --i) {
				canvasManager.removeLayer((uint16)i);
				layerNames[i] = "";
			}
			layerNames[0] = "Layer 0";
			lastLayerNumber = 0;

			canvasManager.resizeDiscardingContents({(uint32)createWidth, (uint32)createHeight });
			currentFileName.clear();
			std::wstring title = L"Panter - Untitled";
			setTitle(title.c_str());

			openCreateWindow = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonSize, buttonSize * 0.5f))) {
			openCreateWindow = false;
		}

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}