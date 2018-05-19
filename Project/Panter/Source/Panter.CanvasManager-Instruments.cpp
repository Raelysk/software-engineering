#include <XLib.Vectors.Arithmetics.h>

#include "Panter.CanvasManager.h"

using namespace XLib;
using namespace XLib::Graphics;
using namespace Panter;

void CanvasManager::updateInstrument_selection()
{
	InstrumentState_Selection &state = instrumentState.selection;

	if (pointerIsActive)
	{
		if (!state.inProgress)
		{
			float32x2 firstCornerPositionF = float32x2(pointerPosition) * viewToCanvasTransform;
			// TODO: implement vector operations
			firstCornerPositionF.x = max(firstCornerPositionF.x, 0.0f);
			firstCornerPositionF.y = max(firstCornerPositionF.y, 0.0f);
			state.firstCornerPosition = uint32x2(firstCornerPositionF);
			state.firstCornerPosition.x = min(state.firstCornerPosition.x, canvasSize.x);
			state.firstCornerPosition.y = min(state.firstCornerPosition.y, canvasSize.y);

			// reset selection initially
			selection = { 0, 0, canvasSize };

			state.inProgress = true;
		}
		else
		{
			float32x2 selectionSecondCornerPositionF = float32x2(pointerPosition) * viewToCanvasTransform;
			// TODO: implement vector operations
			selectionSecondCornerPositionF.x = max(selectionSecondCornerPositionF.x, 0.0f);
			selectionSecondCornerPositionF.y = max(selectionSecondCornerPositionF.y, 0.0f);
			uint32x2 secondCornerPosition = uint32x2(selectionSecondCornerPositionF);
			secondCornerPosition.x = min(secondCornerPosition.x, canvasSize.x);
			secondCornerPosition.y = min(secondCornerPosition.y, canvasSize.y);

			if (state.firstCornerPosition == secondCornerPosition)
			{
				// reset selection
				selection = { 0, 0, canvasSize };
			}
			else
			{
				selection.left   = min(state.firstCornerPosition.x, secondCornerPosition.x);
				selection.top    = min(state.firstCornerPosition.y, secondCornerPosition.y);
				selection.right  = max(state.firstCornerPosition.x, secondCornerPosition.x);
				selection.bottom = max(state.firstCornerPosition.y, secondCornerPosition.y);
			}
		}
	}
	else
	{
		state.inProgress = false;
	}
}

void CanvasManager::updateInstrument_pencil()
{
	InstrumentState_Pencil &state = instrumentState.pencil;
	PencilSettings &settings = instrumentSettings.pencil;

	if (!pointerIsActive)
		return;
	if (prevPointerPosition == pointerPosition)
		return;

	device->setRenderTarget(layerTextures[0]);
	device->setViewport(rectu32(0, 0, canvasSize));
	device->setScissorRect(selection);
	device->setTransform2D(Matrix2x3::Identity());

	float32x2 segmentBegin = float32x2(prevPointerPosition) * viewToCanvasTransform;
	float32x2 segmentEnd = float32x2(pointerPosition) * viewToCanvasTransform;

	geometryGenerator.drawLine(segmentBegin, segmentEnd, 1.0f, settings.color);
	geometryGenerator.flush();
}

void CanvasManager::updateInstrument_brush()
{
	InstrumentState_Brush &state = instrumentState.brush;
	BrushSettings &settings = instrumentSettings.brush;

	if (!pointerIsActive)
		return;
	if (prevPointerPosition == pointerPosition)
		return;

	device->setRenderTarget(layerTextures[currentLayer]);
	device->setViewport(rectu32(0, 0, canvasSize));
	device->setScissorRect(selection);
	device->setTransform2D(Matrix2x3::Identity());

	float32x2 segmentBegin = float32x2(prevPointerPosition) * viewToCanvasTransform;
	float32x2 segmentEnd = float32x2(pointerPosition) * viewToCanvasTransform;

	geometryGenerator.drawLine(segmentBegin, segmentEnd, settings.width, settings.color, true, true);
	geometryGenerator.flush();
}

void CanvasManager::updateInstrument_line()
{
	InstrumentState_Line &state = instrumentState.line;
	LineSettings &settings = instrumentSettings.line;

	if (pointerIsActive)
	{
		if (state.inProgress)
		{
			float32x2 currentPosition = float32x2(pointerPosition) * viewToCanvasTransform;
			if (state.endPosition != currentPosition)
			{
				state.endPosition = currentPosition;
				goto label_rendering;
			}
		}
		else
		{
			state.inProgress = true;
			state.startPosition = float32x2(pointerPosition) * viewToCanvasTransform;
			state.endPosition = state.startPosition;
		}
	}
	else
	{
		state.inProgress = false;

		if (state.outOfDate)
		{
			state.outOfDate = false;
			goto label_rendering;
		}
	}

	return;

label_rendering:
	device->clear(tempTexture, 0xFFFFFF00_rgba);
	device->setRenderTarget(tempTexture);
	device->setViewport(rectu32(0, 0, canvasSize));
	device->setScissorRect(selection);
	device->setTransform2D(Matrix2x3::Identity());

	geometryGenerator.drawLine(state.startPosition, state.endPosition, settings.width,
		settings.color, settings.roundedStart, settings.roundedEnd);
	geometryGenerator.flush();
}

void CanvasManager::updateInstrument_brightnessContrastGammaFilter()
{
	InstrumentState_BrightnessContrastGammaFilter &state = instrumentState.brightnessContrastGammaFilter;
	BrightnessContrastGammaFilterSettings &settings = instrumentSettings.brightnessContrastGamma;

	if (state.outOfDate)
	{
		state.outOfDate = false;

		{
			float32x2 canvasSizeF(canvasSize);

			VertexTexturedUnorm2D vertices[6];
			vertices[0] = { { 0.0f,          0.0f          }, { 0,      0      } };
			vertices[1] = { { canvasSizeF.x, 0.0f          }, { 0xFFFF, 0      } };
			vertices[2] = { { 0.0f,          canvasSizeF.y }, { 0,      0xFFFF } };
			vertices[3] = { { 0.0f,          canvasSizeF.y }, { 0,      0xFFFF } };
			vertices[4] = { { canvasSizeF.x, 0.0f          }, { 0xFFFF, 0      } };
			vertices[5] = { { canvasSizeF.x, canvasSizeF.y }, { 0xFFFF, 0xFFFF } };

			device->uploadBuffer(quadVertexBuffer, vertices, 0, sizeof(vertices));
		}

		device->setRenderTarget(tempTexture);
		device->setViewport(rectu32(0, 0, canvasSize));
		device->setScissorRect(selection);
		device->setTransform2D(Matrix2x3::Identity());
		device->setTexture(layerTextures[currentLayer]);
		device->setCustomEffectConstants(settings);

		device->clear(tempTexture, 0);
		device->draw2D(PrimitiveType::TriangleList, brightnessContrastGammaEffect,
			quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);
	}

	if (state.apply)
	{
		state.apply = false;

		disableCurrentLayerRendering = false;
		enableTempLayerRendering = false;

		device->copyTexture(layerTextures[currentLayer], tempTexture, selection.leftTop, selection);

		resetInstrument();
	}
}

void CanvasManager::resetInstrument()
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	currentInstrument = Instrument::None;
}

void CanvasManager::setInstrument_selection()
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentState.selection.inProgress = false;
	currentInstrument = Instrument::Selection;
}

PencilSettings& CanvasManager::setInstrument_pencil(Color color)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentSettings.pencil.color = color;
	currentInstrument = Instrument::Pencil;

	return instrumentSettings.pencil;
}

BrushSettings& CanvasManager::setInstrument_brush(Color color, float32 width)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentSettings.brush.color = color;
	instrumentSettings.brush.width = width;
	currentInstrument = Instrument::Brush;

	return instrumentSettings.brush;
}

LineSettings& CanvasManager::setInstrument_line(XLib::Color color, float32 width,
	bool roundedStart, bool roundedEnd)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = true;

	instrumentSettings.line.color = color;
	instrumentSettings.line.width = width;
	instrumentSettings.line.roundedStart = roundedStart;
	instrumentSettings.line.roundedEnd = roundedEnd;
	instrumentState.line.inProgress = false;
	instrumentState.line.outOfDate = false;
	currentInstrument = Instrument::Line;

	return instrumentSettings.line;
}

BrightnessContrastGammaFilterSettings& CanvasManager::setInstrument_brightnessContrastGammaFilter(
	float32 brightness, float32 contrast, float32 gamma)
{
	disableCurrentLayerRendering = true;
	enableTempLayerRendering = true;

	instrumentSettings.brightnessContrastGamma.brightness = brightness;
	instrumentSettings.brightnessContrastGamma.contrast = contrast;
	instrumentSettings.brightnessContrastGamma.gamma = gamma;
	instrumentState.brightnessContrastGammaFilter.outOfDate = true;
	instrumentState.brightnessContrastGammaFilter.apply = false;
	currentInstrument = Instrument::BrightnessContrastGammaFilter;

	return instrumentSettings.brightnessContrastGamma;
}

void CanvasManager::updateInstrumentSettings()
{
	switch (currentInstrument)
	{
		case Instrument::Line:
			instrumentState.line.outOfDate = true;
			break;

		case Instrument::BrightnessContrastGammaFilter:
			instrumentState.brightnessContrastGammaFilter.outOfDate = true;
			disableCurrentLayerRendering = true;
			enableTempLayerRendering = true;
			break;
	}
}

void CanvasManager::applyInstrument()
{
	switch (currentInstrument)
	{
		case Instrument::BrightnessContrastGammaFilter:
			instrumentState.brightnessContrastGammaFilter.apply = true;
			break;
	}
}