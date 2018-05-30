#include <XLib.Debug.h>
#include <XLib.Vectors.Arithmetics.h>

#include "Panter.CanvasManager.h"

#include "Panter.Constants.h"
#include "Panter.CanvasManager.EffectShaders.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

// Public interface =============================================================================//

void CanvasManager::initialize(Device& device, uint32x2 canvasSize)
{
	this->canvasSize = canvasSize;
	this->layerCount = 0;
	this->device = &device;

	device.createBuffer(quadVertexBuffer, sizeof(VertexTexturedUnorm2D) * 6);
	geometryGenerator.initialize(device);

	device.createCustomEffect(brightnessContrastGammaEffect, Effect::TexturedUnorm,
		EffectShaders::BrightnessContrastGammaPS.data, EffectShaders::BrightnessContrastGammaPS.size);
	device.createCustomEffect(checkerboardEffect, Effect::TexturedUnorm,
		EffectShaders::CheckerboardPS.data, EffectShaders::CheckerboardPS.size);

	centerView();
	selection = { 0, 0, canvasSize };
}

void CanvasManager::destroy()
{

}

void CanvasManager::resizeDiscardingContents(uint32x2 newCanvasSize)
{
	for (uint32 i = 0; i < layerCount; i++)
	{
		layerTextures[i].destroy();
		device->createTextureRenderTarget(layerTextures[i], newCanvasSize.x, newCanvasSize.y);
	}

	tempTexture.destroy();
	device->createTextureRenderTarget(tempTexture, newCanvasSize.x, newCanvasSize.y);

	canvasSize = newCanvasSize;
	resetSelection();
}

void CanvasManager::resizeSavingContents(const rects32& newCanvasRect, Color fillColor)
{
	if (newCanvasRect.left == 0 && newCanvasRect.top == 0 &&
		uint32x2(newCanvasRect.rightBottom) == canvasSize)
	{
		return;
	}

	uint32x2 newCanvasSize = newCanvasRect.getSize();

	rects32 copiedContentsRegion =
	{
		max<sint32>(newCanvasRect.left, 0),
		max<sint32>(newCanvasRect.top, 0),
		min<sint32>(newCanvasRect.right, canvasSize.x),
		min<sint32>(newCanvasRect.bottom, canvasSize.y)
	};
	uint32x2 copiedContentsLocation =
	{
		uint32(max<sint32>(-newCanvasRect.left, 0)),
		uint32(max<sint32>(-newCanvasRect.top, 0)),
	};

	if (copiedContentsRegion.getWidth() <= 0 || copiedContentsRegion.getHeight() <= 0)
	{
		for (uint32 i = 0; i < layerCount; i++)
		{
			layerTextures[i].destroy();
			device->createTextureRenderTarget(layerTextures[i], newCanvasSize.x, newCanvasSize.y);
			device->clear(layerTextures[i], fillColor);
		}
	}
	else
	{
		uint32x2 copiedContentsSize = uint32x2(copiedContentsRegion.getSize());

		bool blankSpaceLeft = copiedContentsSize != newCanvasSize;

		for (uint32 i = 0; i < layerCount; i++)
		{
			device->copyTexture(tempTexture, layerTextures[i],
				{ 0, 0 }, rectu32(copiedContentsRegion));
			layerTextures[i].destroy();

			device->createTextureRenderTarget(layerTextures[i], newCanvasSize.x, newCanvasSize.y);

			if (blankSpaceLeft)
				device->clear(layerTextures[i], fillColor);
			device->copyTexture(layerTextures[i], tempTexture,
				copiedContentsLocation, { 0, 0, copiedContentsSize });
		}
	}

	tempTexture.destroy();
	device->createTextureRenderTarget(tempTexture, newCanvasSize.x, newCanvasSize.y);

	canvasSize = newCanvasSize;
	resetSelection();
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
			case Instrument::None:
				break;

			case Instrument::Selection:
				updateInstrument_selection();
				break;

			case Instrument::Pencil:
				updateInstrument_pencil();
				break;

			case Instrument::Brush:
				updateInstrument_brush();
				break;

			case Instrument::Line:
				updateInstrument_line();
				break;

			case Instrument::BrightnessContrastGammaFilter:
				updateInstrument_brightnessContrastGammaFilter();
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
	viewToCanvasTransform =
		Matrix2x3::Scale(1.0f / inertCanvasScale) *
		Matrix2x3::Translation(-inertCanvasPosition);

	canvasToViewTransform =
		Matrix2x3::Translation(inertCanvasPosition) *
		Matrix2x3::Scale(inertCanvasScale);

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

		device->uploadBuffer(quadVertexBuffer, vertices, 0, sizeof(vertices));
	}

	device->setRenderTarget(target);
	device->setViewport(viewport);
	device->setScissorRect(viewport);
	device->setTransform2D(Matrix2x3::Identity());

	// background
	geometryGenerator.drawVerticalGradientRect(rectf32(viewport), 0x0D3863_rgb, 0x5682AD_rgb);
	geometryGenerator.drawRectShadow(viewCanvasRect, 12.0f, 0x00000050_rgba);
	geometryGenerator.flush();

	device->draw2D(PrimitiveType::TriangleList, checkerboardEffect,
		quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);

	// canvas
	for (uint16 i = 0; i < layerCount; i++)
	{
		if (!layerRenderingFlags[i])
			continue;

		if (i != currentLayer || !disableCurrentLayerRendering)
		{
			device->setTexture(layerTextures[i]);
			device->draw2D(PrimitiveType::TriangleList, Effect::TexturedUnorm,
				quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);
		}

		if (i == currentLayer && enableTempLayerRendering)
		{
			device->setTexture(tempTexture);
			device->draw2D(PrimitiveType::TriangleList, Effect::TexturedUnorm,
				quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);
		}
	}

	// canvas space foreground
	{
		device->setTransform2D(canvasToViewTransform);

		// selection rendering
		rectf32 selectionF(selection);
		float32x2 canvasSizeF(canvasSize);

		if (selection.left > 0)
		{
			geometryGenerator.drawRect(
				rectf32(0.0f, 0.0f, selectionF.left, canvasSizeF.y),
				SelectionShadowColor);
		}

		if (selection.right < canvasSize.x)
		{
			geometryGenerator.drawRect(
				rectf32(selectionF.right, 0.0f, canvasSizeF.x, canvasSizeF.y),
				SelectionShadowColor);
		}

		if (selection.top > 0)
		{
			geometryGenerator.drawRect(
				rectf32(selectionF.left, 0.0f, selectionF.right, selectionF.top),
				SelectionShadowColor);
		}

		if (selection.bottom < canvasSize.y)
		{
			geometryGenerator.drawRect(
				rectf32(selectionF.left, selectionF.bottom, selectionF.right, canvasSizeF.y),
				SelectionShadowColor);
		}

		geometryGenerator.flush();
	}

	// view space foreground
	{
		//device->setTransform2D(Matrix2x3::Identity());
	}

	prevPointerPosition = pointerPosition;
}

// Basic controls ===============================================================================//

void CanvasManager::resetSelection()
{
	selection = { 0, 0, canvasSize };
}

void CanvasManager::setPointerState(sint16x2 position, bool isActive)
{
	pointerPosition = position;
	pointerIsActive = isActive;
}

void CanvasManager::setCurrentLayer(uint16 layerIndex)
{
	Debug::CrashCondition(layerIndex >= layerCount, DbgMsgFmt("invalid layer index"));

	currentLayer = layerIndex;
}

// Layers handling ==============================================================================//

uint16 CanvasManager::createLayer(uint16 insertAtIndex)
{
	device->createTextureRenderTarget(layerTextures[layerCount], canvasSize.x, canvasSize.y);
	layerRenderingFlags[layerCount] = true;

	if (!layerCount)
		device->createTextureRenderTarget(tempTexture, canvasSize.x, canvasSize.y);

	return layerCount++;
}

void CanvasManager::removeLayer(uint16 index)
{
	Debug::CrashCondition(index >= layerCount, DbgMsgFmt("invalid layer index"));

	layerTextures[index].destroy();
	--layerCount;
	if (index == currentLayer && currentLayer == layerCount) 
	{
		--currentLayer;
	}

	for (uint16 i = index + 1; i <= layerCount; i++)
	{
		layerTextures[i - 1] = move(layerTextures[i]);
		layerRenderingFlags[i - 1] = layerRenderingFlags[i];
	}
}

void CanvasManager::enableLayer(uint16 index, bool enabled)
{
	Debug::CrashCondition(index >= layerCount, DbgMsgFmt("invalid layer index"));

	layerRenderingFlags[index] = enabled;
}

void CanvasManager::uploadLayerRegion(uint16 dstLayerIndex, const rectu32& dstRegion,
	const void* srcData, uint32 srcDataStride)
{
	Debug::CrashCondition(dstLayerIndex >= layerCount, DbgMsgFmt("invalid layer index"));

	device->uploadTexture(layerTextures[dstLayerIndex], dstRegion, srcData, srcDataStride);
}

void CanvasManager::downloadLayerRegion(uint16 srcLayerIndex, const rectu32& srcRegion,
	void* dstData, uint32 dstDataStride)
{
	Debug::CrashCondition(srcLayerIndex >= layerCount, DbgMsgFmt("invalid layer index"));

	device->downloadTexture(layerTextures[srcLayerIndex], srcRegion, dstData, dstDataStride);
}

void CanvasManager::clearLayer(uint16 layerIndex, Color color)
{
	device->clear(layerTextures[layerIndex], color);
}

// View handling ================================================================================//

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

	float32 newCanvasScale = canvasScale * scaleFactor;
	float32x2 scaleCenter = float32x2(pointerPosition) * viewToCanvasTransform;
	canvasPosition += scaleCenter * (canvasScale - newCanvasScale);
	canvasScale = newCanvasScale;
}

void CanvasManager::setAbsoluteCanvasScale(float32 scale)
{
	viewCentered = false;
	canvasScale = scale;
}
