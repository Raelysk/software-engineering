#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
//#include <XLib.Containers.Vector.h>
#include <XLib.Color.h>
#include <XLib.Vectors.h>
#include <XLib.Graphics.h>
#include <XLib.Graphics.GeometryGenerator.h>

// TODO: Handle current layer change during filter preview.

namespace Panter
{
	struct PencilSettings
	{
		XLib::Color color;
	};

	struct BrushSettings
	{
		XLib::Color color;
		float32 width;
	};

	struct LineSettings
	{
		XLib::Color color;
		float32 width;
		bool roundedStart;
		bool roundedEnd;
	};

	struct BrightnessContrastGammaFilterSettings
	{
		float32 brightness;
		float32 contrast;
		float32 gamma;
	};

	struct GaussianBlurFilterSettings
	{

	};

    enum class Instrument : uint8 {
        None = 0,

        Selection,
        Pencil,
        Brush,
		Line,

        BrightnessContrastGammaFilter,
        GaussianFilter,
    };

	class CanvasManager : public XLib::NonCopyable
	{
	private: // meta
		static constexpr float32 centeredViewMarginRelativeWidth = 0.1f;
		static constexpr float32 viewIntertiaFactor = 0.15f;

		//using Layers = XLib::Vector<XLib::Graphics::TextureRenderTarget>;

		struct InstrumentState_Selection
		{
			uint32x2 firstCornerPosition;
			bool inProgress;
		};

		struct InstrumentState_Pencil { };

		struct InstrumentState_Brush { };

		struct InstrumentState_Line
		{
			float32x2 startPosition;
			float32x2 endPosition;
			bool inProgress;
			bool outOfDate;
			bool notEmpty;
			bool apply;
		};

		struct InstrumentState_BrightnessContrastGammaFilter
		{
			bool outOfDate;
			bool apply;
		};

	private: // data
		// graphics resources
		XLib::Graphics::Device *device = nullptr;
		XLib::Graphics::Buffer quadVertexBuffer;
		XLib::Graphics::GeometryGenerator geometryGenerator;

		XLib::Graphics::CustomEffect brightnessContrastGammaEffect;
		XLib::Graphics::CustomEffect checkerboardEffect;

		// canvas data
		XLib::Graphics::TextureRenderTarget layerTextures[16];
		XLib::Graphics::TextureRenderTarget tempTexture;
		uint32x2 canvasSize = { 0, 0 };
		uint16 layerCount = 0;

		// canvas modification state
		rectu32 selection = {};
		uint16 currentLayer = 0;
		Instrument currentInstrument = Instrument::None;
		bool disableCurrentLayerRendering = false;
		bool enableTempLayerRendering = false;

		union
		{
			PencilSettings pencil;
			BrushSettings brush;
			LineSettings line;
			BrightnessContrastGammaFilterSettings brightnessContrastGamma;
		} instrumentSettings;

		union
		{
			InstrumentState_Selection selection;
			InstrumentState_Pencil pencil;
			InstrumentState_Brush brush;
			InstrumentState_Line line;
			InstrumentState_BrightnessContrastGammaFilter brightnessContrastGammaFilter;
		} instrumentState;

		// view state
		float32x2 canvasPosition = { 0.0f, 0.0f };
		float32 canvasScale = 0.0f;
		bool viewCentered = false;

		float32x2 inertCanvasPosition = { 0.0f, 0.0f };
		float32 inertCanvasScale = 0.0f;

		XLib::Matrix2x3 viewToCanvasTransform = {};
		XLib::Matrix2x3 canvasToViewTransform = {};

		// pointer state
		sint16x2 pointerPosition = { 0, 0 };
		sint16x2 prevPointerPosition = { 0, 0 };
		bool pointerIsActive = false;
		bool pointerPanViewModeEnabled = false;

	private: // code
		void updateInstrument_selection();
		void updateInstrument_pencil();
		void updateInstrument_brush();
		void updateInstrument_line();
		void updateInstrument_brightnessContrastGammaFilter();

	public:
		CanvasManager() = default;
		~CanvasManager() = default;

		void initialize(XLib::Graphics::Device& device, uint32x2 canvasSize);
		void destroy();

		void resizeDiscardingContents(uint32x2 newCanvasSize);
		void resizeSavingContents(const rects32& newCanvasRect, XLib::Color fillColor = 0);
		void updateAndDraw(XLib::Graphics::RenderTarget& target, const rectu32& viewport /* TODO: move from here */);
		//void setViewport();

		void resetSelection();
		void setPointerState(sint16x2 position, bool isActive);
		void setCurrentLayer(uint16 layerIndex);

		void resetInstrument();
		void setInstrument_selection();
		PencilSettings&	setInstrument_pencil(XLib::Color color = 0);
		BrushSettings&	setInstrument_brush(XLib::Color color = 0, float32 width = 1.0f);
		LineSettings&	setInstrument_line(XLib::Color = 0, float32 width = 1.0f, bool roundedStart = false, bool roundedEnd = false);
		BrightnessContrastGammaFilterSettings&	setInstrument_brightnessContrastGammaFilter(float32 brightness = 0.0f, float32 contrast = 1.0f, float32 gamma = 1.0f);
		GaussianBlurFilterSettings&				setInstrument_gaussianBlurFilter();
		void updateInstrumentSettings();
		void applyInstrument();

		uint16 createLayer(uint16 insertAtIndex = uint16(-1));
		void removeLayer(uint16 index);
		//void moveLayer(uint16 fromIndex, uint16 toIndex);

		void uploadLayerRegion(uint16 dstLayerIndex, const rectu32& dstRegion,
			const void* srcData, uint32 srcDataStride = 0);
		void downloadLayerRegion(uint16 srcLayerIndex, const rectu32& srcRegion,
			void* dstData, uint32 dstDataStride = 0);
		void clearLayer(uint16 layerIndex, XLib::Color color);

		void centerView();
		void enablePointerPanViewMode(bool enabled);
		void panView(float32x2 offset);
		void scaleView(float32 scaleFactor); // TODO: add scaleViewToPointer
		void setAbsoluteCanvasScale(float32 scale);

		//void undo();
		//void redo();

        inline Instrument getCanvasInstrument() const { return currentInstrument; }
		inline PencilSettings&	getInstrumentSettings_pencil()	{ return instrumentSettings.pencil; }
		inline BrushSettings&	getInstrumentSettings_brush()	{ return instrumentSettings.brush; }
		inline LineSettings&	getInstrumentSettings_line()	{ return instrumentSettings.line; }
		inline BrightnessContrastGammaFilterSettings&	getInstrumentSettings_brightnessContrastGammaFilter() { return instrumentSettings.brightnessContrastGamma; }
		inline GaussianBlurFilterSettings&				getInstrumentSettings_gaussianBlurFilter();

		inline uint32x2 getCanvasSize() const { return canvasSize; }
		inline uint32 getCanvasWidth() const { return canvasSize.x; }
		inline uint32 getCanvasHeight() const { return canvasSize.y; }
		
		inline uint16 getLayerCount() const { return layerCount; }
        inline uint16 getCurrentLayerId() const { return currentLayer; }
		inline const rectu32& getSelection() const { return selection; }

		inline float32 getCanvasScale() const { return inertCanvasScale; }
		inline float32x2 getCanvasSpacePointerPosition() const { return float32x2(pointerPosition) * viewToCanvasTransform; }
		
		inline bool isInitialized() const { return device != nullptr; }
	};
}
