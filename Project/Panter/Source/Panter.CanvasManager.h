#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
//#include <XLib.Containers.Vector.h>
#include <XLib.Color.h>
#include <XLib.Vectors.h>
#include <XLib.Graphics.h>
#include <XLib.Graphics.GeometryGenerator.h>

namespace Panter
{
	enum class CanvasInstrument : uint8
	{
		None = 0,

		Select,
		SelectionResize,
		Pencil,
		Brush,
	};

	struct BrightnessContrastGammaFilterSettings
	{
		float32 brightness;
		float32 contrast;
		float32 gamma;
	};

	class CanvasManager : public XLib::NonCopyable
	{
	private: // meta
		static constexpr float32 centeredViewMarginRelativeWidth = 0.1f;
		static constexpr float32 viewIntertiaFactor = 0.15f;

		//using Layers = XLib::Vector<XLib::Graphics::TextureRenderTarget>;

	private: // data
		XLib::Graphics::Device *device = nullptr;
		XLib::Graphics::Buffer quadVertexBuffer;
		XLib::Graphics::GeometryGenerator geometryGenerator;
		XLib::Graphics::CustomEffect brightnessContrastGammaEffect;

		XLib::Graphics::TextureRenderTarget layerTextures[16];
		XLib::Graphics::TextureRenderTarget tempTexture;
		uint32x2 canvasSize = { 0, 0 };
		uint16 layerCount = 0;

		rectu32 selection = {};

		float32x2 canvasPosition = { 0.0f, 0.0f };
		float32 canvasScale = 0.0f;
		bool viewCentered = false;

		float32x2 inertCanvasPosition = { 0.0f, 0.0f };
		float32 inertCanvasScale = 0.0f;

		XLib::Matrix2x3 viewToCanvasTransform = {};
		XLib::Matrix2x3 canvasToViewTransform = {};

		//union
		//{
		bool selectionInProgress = false;
		uint32x2 selectionFirstCornerPosition = { 0, 0 };
		bool filterPreview = false;
		//} instrumentsData;

		sint16x2 pointerPosition = { 0, 0 };
		sint16x2 prevPointerPosition = { 0, 0 };
		bool pointerIsActive = false;
		bool pointerPanViewModeEnabled = false;

		CanvasInstrument currentInstrument = CanvasInstrument::None;
		XLib::Color currentColor;

	private: // code
		inline void updateCanvasTransforms();
		inline float32x2 convertViewToCanvasSpace(sint16x2 position);

	public:
		CanvasManager() = default;
		~CanvasManager() = default;

		void initialize(XLib::Graphics::Device& device, uint32x2 canvasSize);
		void destroy();

		void resize(uint32x2 newCanvasSize);
		void updateAndDraw(XLib::Graphics::RenderTarget& target, const rectu32& viewport /* TODO: move from here */);
		//void setViewport();

		void brightnessContrastGammaFilter(bool preview, const BrightnessContrastGammaFilterSettings& settings);

		void resetSelection();
		void setInstrument(CanvasInstrument instrument);
		void setColor(XLib::Color color);
		void setPointerState(sint16x2 position, bool isActive);
		//void setCurrentLayer(uint16 layerIndex);

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

		inline uint32x2 getCanvasSize() const { return canvasSize; }
		inline uint32 getCanvasWidth() const { return canvasSize.x; }
		inline uint32 getCanvasHeight() const { return canvasSize.y; }
		inline float32 getCanvasScale() const { return canvasScale; }
		inline bool isInitialized() const { return device != nullptr; }
	};
}
