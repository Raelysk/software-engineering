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
	enum class Instrument : uint8
	{
		None = 0,

		Selection,
		Pencil,
		Brush,
		Line,
		Shape,

		BrightnessContrastGammaFilter,
		GaussianBlurFilter,
		SharpenFilter,
	};

	enum class Shape : uint8
	{
		None = 0,

		Rectangle,
		Circle,
	};

	struct PencilSettings
	{
		XLib::Color color;
	};

	struct BrushSettings
	{
		XLib::Color color;
		float32 width;
		bool blendEnabled;
	};

	struct LineSettings
	{
		XLib::Color color;
		float32 width;
		bool roundedStart;
		bool roundedEnd;
	};

	struct ShapeSettings
	{
		XLib::Color fillColor;
		XLib::Color borderColor;
		float32 borderWidth;
		Shape shape;
	};

	struct BrightnessContrastGammaFilterSettings
	{
		float32 brightness;
		float32 contrast;
		float32 gamma;
	};

	struct GaussianBlurFilterSettings
	{
		uint32 radius;
	};

	struct SharpenFilterSettings
	{
		float32 intensity;
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
			enum class UserState : uint8
			{
				Standby = 0,
				Draw,		// End position is modified.
				Modify,		// Anchor position is modified.
			};

			float32x2 startPosition;
			float32x2 endPosition;
			float32x2 pointerFromAnchorOffset;
			sint16x2 prevModifyPointerPosition;
			UserState userState;
			bool anchorIndex;
			bool notEmpty;
			bool outOfDate;
			bool apply;
		};

		struct InstrumentState_Shape
		{
			enum class UserState : uint8
			{
				Standby = 0,
				Draw,		// End position is modified.
				Modify,		// Anchor position is modified.
			};

			union
			{
				struct
				{
					float32x2 startPosition;
					float32x2 endPosition;
				};

				rectf32 rect;
			};
			UserState userState;
			bool notEmpty;
			bool outOfDate;
			bool apply;
		};

		struct InstrumentState_Filter
		{
			bool outOfDate;
			bool apply;
		};

	private: // data
		// graphics resources
		XLib::Graphics::Device *device = nullptr;
		XLib::Graphics::Buffer quadVertexBuffer;
		XLib::Graphics::GeometryGenerator geometryGenerator;

		XLib::Graphics::CustomEffect checkerboardEffect;
		XLib::Graphics::CustomEffect brightnessContrastGammaEffect;
		XLib::Graphics::CustomEffect blurEffect;
		XLib::Graphics::CustomEffect sharpenEffect;

		// canvas data
		XLib::Graphics::TextureRenderTarget layerTextures[16];
		bool layerRenderingFlags[16] = {};
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
			ShapeSettings shape;
			BrightnessContrastGammaFilterSettings brightnessContrastGamma;
			GaussianBlurFilterSettings gaussianBlur;
			SharpenFilterSettings sharpen;
		} instrumentSettings;

		union
		{
			InstrumentState_Selection selection;
			InstrumentState_Pencil pencil;
			InstrumentState_Brush brush;
			InstrumentState_Line line;
			InstrumentState_Shape shape;
			InstrumentState_Filter filter;
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
		void updateInstrument_shape();
		void updateInstrument_filter(XLib::Graphics::CustomEffect& filterEffect,
			const void* settings = nullptr, uint32 settingsSize = 0);

		template <typename SettingsType>
		inline void updateInstrument_filter(XLib::Graphics::CustomEffect& filterEffect, const SettingsType& settings)
			{ updateInstrument_filter(filterEffect, &settings, sizeof(SettingsType)); }

		void mergeCurrentLayerWithTemp();

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
		BrushSettings&	setInstrument_brush(XLib::Color color = 0, float32 width = 5.0f, bool blendEnabled = true);
		LineSettings&	setInstrument_line(XLib::Color color = 0, float32 width = 5.0f, bool roundedStart = false, bool roundedEnd = false);
		ShapeSettings&	setInstrument_shape(XLib::Color fillColor = 0, XLib::Color borderColor = 0, float32 borderWidth = 5.0f, Shape shape = Shape::Rectangle);

		BrightnessContrastGammaFilterSettings&	setInstrument_brightnessContrastGammaFilter(float32 brightness = 0.0f, float32 contrast = 1.0f, float32 gamma = 1.0f);
		GaussianBlurFilterSettings&				setInstrument_gaussianBlurFilter(uint32 radius = 8);
		SharpenFilterSettings&					setInstrument_sharpenFilter(float32 intensity = 1.0f);
		void updateInstrumentSettings();
		void applyInstrument();

		uint16 createLayer(uint16 insertAtIndex = uint16(-1));
		void removeLayer(uint16 index);
		void moveLayer(uint16 fromIndex, uint16 toIndex);
		void enableLayer(uint16 index, bool enabled);

		void uploadLayerRegion(uint16 dstLayerIndex, const rectu32& dstRegion,
			const void* srcData, uint32 srcDataStride = 0);
		void downloadLayerRegion(uint16 srcLayerIndex, const rectu32& srcRegion,
			void* dstData, uint32 dstDataStride = 0);
		void downloadMergedLayers(void* dstData, uint32 dstDataStride = 0);
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
		inline ShapeSettings&	getInstrumentSettings_shape() { return instrumentSettings.shape; }
		inline BrightnessContrastGammaFilterSettings&	getInstrumentSettings_brightnessContrastGammaFilter() { return instrumentSettings.brightnessContrastGamma; }
		inline GaussianBlurFilterSettings&				getInstrumentSettings_gaussianBlurFilter() { return instrumentSettings.gaussianBlur; }
		inline SharpenFilterSettings&					getInstrumentSettings_sharpenFilter() { return instrumentSettings.sharpen; }

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
