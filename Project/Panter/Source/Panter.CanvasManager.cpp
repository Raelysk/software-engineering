#include <XLib.Debug.h>
#include <XLib.Vectors.Arithmetics.h>

#include "Panter.CanvasManager.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

inline void CanvasManager::updateCanvasTransforms()
{
	viewToCanvasTransform =
		Matrix2x3::Scale(1.0f / canvasScale) *
		Matrix2x3::Translation(-inertCanvasPosition);

	canvasToViewTransform =
		Matrix2x3::Translation(inertCanvasPosition) *
		Matrix2x3::Scale(canvasScale);
}

inline float32x2 CanvasManager::convertViewToCanvasSpace(sint16x2 position)
{
	return float32x2(position) * viewToCanvasTransform;
}

void CanvasManager::initialize(Device& device, uint32x2 canvasSize)
{
	this->canvasSize = canvasSize;
	this->layerCount = 0;
	this->device = &device;

	device.createBuffer(quadVertexBuffer, sizeof(VertexTexturedUnorm2D) * 6);
	geometryGenerator.initialize(device);

	centerView();
}

void CanvasManager::destroy()
{

}

void CanvasManager::resize(uint32x2 newCanvasSize)
{

}

void CanvasManager::updateAndDraw(RenderTarget& target, const rectu32& viewport)
{
	if (pointerPanViewModeEnabled)
	{
		panView(float32x2(pointerPosition - prevPointerPosition));
	}
	else
	{
		// update instrument state
		switch (currentInstrument)
		{
		case CanvasInstrument::None:
			break;

		case CanvasInstrument::Select:
			break;

		case CanvasInstrument::SelectionResize:
			break;

		case CanvasInstrument::Pencil:
			if (pointerIsActive)
			{
				if (!pencilIsDrawing)
				{
					pencilIsDrawing = true;
					break;
				}

				if (prevPointerPosition == pointerPosition)
					break;

				device->setRenderTarget(layerTextures[0]);
				device->setViewport(rectu32(0, 0, canvasSize.x, canvasSize.y));
				device->setTransform2D(Matrix2x3::Identity());

				float32x2 segmentBegin = convertViewToCanvasSpace(prevPointerPosition);
				float32x2 segmentEnd = convertViewToCanvasSpace(pointerPosition);

				geometryGenerator.drawLine(segmentBegin, segmentEnd, 1.0f, currentColor);
				geometryGenerator.flush();
			}
			break;

		case CanvasInstrument::Brush:
			if (pointerIsActive)
			{
				device->setRenderTarget(layerTextures[0]);
				device->setViewport(rectu32(0, 0, canvasSize.x, canvasSize.y));
				device->setTransform2D(Matrix2x3::Identity());

				float32x2 center = float32x2(pointerPosition) * viewToCanvasTransform;

				geometryGenerator.drawRect(rectf32(
					center - float32x2(5.0f, 5.0f),
					center + float32x2(5.0f, 5.0f)),
					currentColor);
				geometryGenerator.flush();
			}
			break;

		default:
			Debug::Crash("invalid instrument");
		}
	}

	uint32x2 viewportSize = viewport.getSize();
	float32 viewportAspect = float32(viewportSize.x) / float32(viewportSize.y);
	float32 canvasAspect = float32(canvasSize.x) / float32(canvasSize.y);

	// TODO: remove from here.
	if (viewCentered)
	{
		float32 centeredViewScale = 0.0f;

		if (canvasAspect > viewportAspect)
		{
			// canvas limited in width
			float32 viewSpaceCanvasWidth = float32(viewportSize.x) * (1.0f - centeredViewMarginRelativeWidth);
			centeredViewScale = viewSpaceCanvasWidth / float32(canvasSize.x);
		}
		else
		{
			// canvas limited in height
			float32 viewSpaceCanvasHeight = float32(viewportSize.y) * (1.0f - centeredViewMarginRelativeWidth);
			centeredViewScale = viewSpaceCanvasHeight / float32(canvasSize.y);
		}

		canvasPosition = float32x2(viewportSize / 2) - float32x2(canvasSize / 2) * centeredViewScale;
		canvasScale = centeredViewScale;
	}

	inertCanvasPosition += (canvasPosition - inertCanvasPosition) * viewIntertiaFactor;
	inertCanvasScale += (canvasScale - inertCanvasScale) * viewIntertiaFactor;

	// TODO: remove from here.
	updateCanvasTransforms();

	rectf32 viewCanvasRect = {};
	viewCanvasRect.leftTop = inertCanvasPosition;
	viewCanvasRect.rightBottom = inertCanvasPosition + float32x2(canvasSize) * inertCanvasScale;

	{
		VertexTexturedUnorm2D vertices[6];
		vertices[0] = { { viewCanvasRect.left,  viewCanvasRect.top    }, { 0,      0      } };
		vertices[1] = { { viewCanvasRect.right, viewCanvasRect.top    }, { 0xFFFF, 0      } };
		vertices[2] = { { viewCanvasRect.left,  viewCanvasRect.bottom }, { 0,      0xFFFF } };
		vertices[3] = { { viewCanvasRect.left,  viewCanvasRect.bottom }, { 0,      0xFFFF } };
		vertices[4] = { { viewCanvasRect.right, viewCanvasRect.top    }, { 0xFFFF, 0      } };
		vertices[5] = { { viewCanvasRect.right, viewCanvasRect.bottom }, { 0xFFFF, 0xFFFF } };

		device->updateBuffer(quadVertexBuffer, vertices, 0, sizeof(vertices));
	}

	device->setRenderTarget(target);
	device->setViewport(viewport);
	device->setTransform2D(Matrix2x3::Identity());

	// background
	geometryGenerator.drawVerticalGradientRect(rectf32(viewport), 0x0D3863_rgb, 0x5682AD_rgb);
	geometryGenerator.drawRectShadow(viewCanvasRect, 12.0f, 0x00000050_rgba);
	geometryGenerator.flush();

	// canvas
	for (uint16 i = 0; i < layerCount; i++)
	{
		device->setTexture(layerTextures[i]);
		device->draw2D(PrimitiveType::TriangleList, Effect::TexturedUnorm,
			quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);
	}

	// foreground
	{
		// selection rendering
		/*rectf32 viewSelectionRect;
		viewSelectionRect.leftTop = float32x2(selection.leftTop) * canvasToViewTransform;
		viewSelectionRect.rightBottom = float32x2(selection.rightBottom) * canvasToViewTransform;

		geometryGenerator.drawRect(viewSelectionRect, 0x008AF440_rgba);
		geometryGenerator.flush();*/
	}

	prevPointerPosition = pointerPosition;
}

// Canvas modification / controls handling ======================================================//

void CanvasManager::setInstrument(CanvasInstrument instrument)
{
	currentInstrument = instrument;
}

void CanvasManager::setColor(Color color)
{
	currentColor = color;
}

void CanvasManager::setPointerState(sint16x2 position, bool isActive)
{
	pointerPosition = position;
	pointerIsActive = isActive;
}

// Layers handling ==============================================================================//

uint16 CanvasManager::createLayer(uint16 insertAtIndex)
{
	device->createTextureRenderTarget(layerTextures[layerCount], canvasSize.x, canvasSize.y);
	return layerCount++;
}

void CanvasManager::removeLayer(uint16 index)
{

}

void CanvasManager::uploadLayerRegion(uint16 dstLayerIndex, const rectu32& dstRegion,
	const void* srcData, uint32 srcDataStride)
{
	Debug::CrashCondition(dstLayerIndex >= layerCount, DbgMsgFmt("invalid layer index"));

	device->updateTexture(layerTextures[dstLayerIndex], dstRegion, srcData, srcDataStride);
}

// View handling ================================================================================//

void CanvasManager::clearLayer(uint16 layerIndex, Color color)
{
	device->clear(layerTextures[layerIndex], color);
}

void CanvasManager::centerView()
{
	viewCentered = true;
}

void CanvasManager::enablePointerPanViewMode(bool enabled)
{
	pointerPanViewModeEnabled = enabled;
}

void CanvasManager::panView(float32x2 offset)
{
	viewCentered = false;
	canvasPosition += offset;
}

void CanvasManager::scaleView(float32 scaleFactor)
{
	viewCentered = false;
	canvasScale *= scaleFactor;
}

void CanvasManager::setAbsoluteCanvasScale(float32 scale)
{
	viewCentered = false;
	canvasScale = scale;
}
