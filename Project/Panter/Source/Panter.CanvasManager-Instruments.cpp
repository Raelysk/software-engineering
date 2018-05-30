#include <XLib.Debug.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Vectors.Math.h>

#include "Panter.CanvasManager.h"

#include "Panter.Constants.h"

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

	device->setRenderTarget(layerTextures[currentLayer]);
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
	using UserState = InstrumentState_Line::UserState;

	InstrumentState_Line &state = instrumentState.line;
	LineSettings &settings = instrumentSettings.line;

	auto checkIfIsDrawable = [&]() -> bool
	{
		float32x2 l = state.endPosition - state.startPosition;
		return l.x >= 1.0f || l.x <= -1.0f || l.y >= 1.0f || l.y <= -1.0f;
	};

	auto render = [&]()
	{
		device->clear(tempTexture, 0xFFFFFF00_rgba);
		device->setRenderTarget(tempTexture);
		device->setViewport(rectu32(0, 0, canvasSize));
		device->setScissorRect(selection);
		device->setTransform2D(Matrix2x3::Identity());

		geometryGenerator.drawLine(state.startPosition, state.endPosition, settings.width,
			settings.color, settings.roundedStart, settings.roundedEnd);
		geometryGenerator.flush();
	};

	if (state.apply)
	{
		if (state.notEmpty)
		{
			state.notEmpty = false;

			if (state.outOfDate)
			{
				state.outOfDate = false;
				render();
			}

			mergeCurrentLayerWithTemp();

			enableTempLayerRendering = false;
		}

		state.userState = UserState::Standby;
		state.apply = false;
	}

	if (pointerIsActive)
	{
		if (state.userState == UserState::Standby)
		{
			if (state.notEmpty)
			{
				// Check if we are moving previous line anchors or starting new line.

				float32x2 viewSpaceStartPosition = state.startPosition * canvasToViewTransform;
				float32x2 viewSpaceEndPosition = state.endPosition * canvasToViewTransform;
				float32x2 pointerPositionF = float32x2(pointerPosition);

				float32 startDistance = VectorMath::Length(viewSpaceStartPosition - pointerPositionF);
				float32 endDistance = VectorMath::Length(viewSpaceEndPosition - pointerPositionF);

				bool potentialAnchorIndex = startDistance > endDistance;
				float32 minDistance = potentialAnchorIndex ? endDistance : startDistance;

				if (minDistance <= ViewSpaceAnchorGrabDistance)
				{
					// Grabbing anchor.
					float32x2 canvasSpacePointerPosition = pointerPositionF * viewToCanvasTransform;
					state.pointerFromAnchorOffset = canvasSpacePointerPosition -
						(potentialAnchorIndex ? state.endPosition : state.startPosition);
					state.prevModifyPointerPosition = pointerPosition;
					state.anchorIndex = potentialAnchorIndex;
					state.userState = UserState::Modify;
				}
				else
				{
					// Starting new line.
					mergeCurrentLayerWithTemp();

					state.startPosition = pointerPositionF * viewToCanvasTransform;
					state.endPosition = state.startPosition;
					state.userState = UserState::Draw;
					state.notEmpty = false;

					enableTempLayerRendering = false;
				}
			}
			else
			{
				state.startPosition = float32x2(pointerPosition) * viewToCanvasTransform;
				state.endPosition = state.startPosition;
				state.userState = UserState::Draw;
				state.notEmpty = false;
				state.outOfDate = false;
			}
		}
		else if (state.userState == UserState::Draw)
		{
			float32x2 canvasSpacePointerPosition = float32x2(pointerPosition) * viewToCanvasTransform;
			if (state.endPosition != canvasSpacePointerPosition)
			{
				state.endPosition = canvasSpacePointerPosition;

				state.notEmpty = checkIfIsDrawable();
				if (state.notEmpty)
				{
					state.outOfDate = false;
					render();
				}

				enableTempLayerRendering = state.notEmpty;
			}
		}
		else if (state.userState == UserState::Modify)
		{
			if (state.prevModifyPointerPosition != pointerPosition)
			{
				state.prevModifyPointerPosition = pointerPosition;

				float32x2 canvasSpacePointerPosition = float32x2(pointerPosition) * viewToCanvasTransform;
				float32x2 newAnchorPosition = canvasSpacePointerPosition - state.pointerFromAnchorOffset;
				if (state.anchorIndex)
					state.endPosition = newAnchorPosition;
				else
					state.startPosition = newAnchorPosition;

				state.notEmpty = checkIfIsDrawable();
				if (state.notEmpty)
				{
					state.outOfDate = false;
					render();
				}

				enableTempLayerRendering = state.notEmpty;
			}
		}
	}
	else
		state.userState = UserState::Standby;

	if (state.outOfDate && state.notEmpty)
	{
		state.outOfDate = false;
		render();
	}
}

void CanvasManager::updateInstrument_shape()
{
	using UserState = InstrumentState_Shape::UserState;

	InstrumentState_Shape &state = instrumentState.shape;
	ShapeSettings &settings = instrumentSettings.shape;

	auto checkIfIsDrawable = [&]() -> bool
	{
		float32x2 l = state.endPosition - state.startPosition;
		return l.x >= 1.0f || l.x <= -1.0f || l.y >= 1.0f || l.y <= -1.0f;
	};

	auto render = [&]()
	{
		device->clear(tempTexture, 0xFFFFFF00_rgba);
		device->setRenderTarget(tempTexture);
		device->setViewport(rectu32(0, 0, canvasSize));
		device->setScissorRect(selection);
		device->setTransform2D(Matrix2x3::Identity());

		rectf32 rect;
		if (state.startPosition.x < state.endPosition.x)
		{
			rect.left = state.startPosition.x;
			rect.right = state.endPosition.x;
		}
		else
		{
			rect.left = state.endPosition.x;
			rect.right = state.startPosition.x;
		}

		if (state.startPosition.y < state.endPosition.y)
		{
			rect.top = state.startPosition.y;
			rect.bottom = state.endPosition.y;
		}
		else
		{
			rect.top = state.endPosition.y;
			rect.bottom = state.startPosition.y;
		}

		if (settings.shape == Shape::Rectangle)
		{
			geometryGenerator.drawFilledRectWithBorder(rect,
				settings.fillColor, settings.borderColor, settings.borderWidth);
		}
		else if (settings.shape == Shape::Circle)
		{
			settings.borderColor.a = 255;

			float32x2 center((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f);
			float32x2 radius = rect.getSize() * 0.5f;

			geometryGenerator.drawFilledEllipse(center, radius, settings.fillColor);
			geometryGenerator.drawEllipseBorder(center, radius, settings.borderColor, settings.borderWidth);
		}

		geometryGenerator.flush();
	};

	if (state.apply)
	{
		if (state.notEmpty)
		{
			state.notEmpty = false;

			if (state.outOfDate)
			{
				state.outOfDate = false;
				render();
			}

			mergeCurrentLayerWithTemp();

			enableTempLayerRendering = false;
		}

		state.userState = UserState::Standby;
		state.apply = false;
	}

	if (pointerIsActive)
	{
		if (state.userState == UserState::Standby)
		{
			if (state.notEmpty)
			{
				// Check if we are moving previous line anchors or starting new line.

				/*float32x2 viewSpaceStartPosition = state.startPosition * canvasToViewTransform;
				float32x2 viewSpaceEndPosition = state.endPosition * canvasToViewTransform;
				float32x2 pointerPositionF = float32x2(pointerPosition);

				float32 startDistance = VectorMath::Length(viewSpaceStartPosition - pointerPositionF);
				float32 endDistance = VectorMath::Length(viewSpaceEndPosition - pointerPositionF);

				bool potentialAnchorIndex = startDistance > endDistance;
				float32 minDistance = potentialAnchorIndex ? endDistance : startDistance;

				if (minDistance <= ViewSpaceAnchorGrabDistance)
				{
					// Grabbing anchor.
					float32x2 canvasSpacePointerPosition = pointerPositionF * viewToCanvasTransform;
					state.pointerFromAnchorOffset = canvasSpacePointerPosition -
						(potentialAnchorIndex ? state.endPosition : state.startPosition);
					state.prevModifyPointerPosition = pointerPosition;
					state.anchorIndex = potentialAnchorIndex;
					state.userState = UserState::Modify;
				}
				else*/
				{
					// Starting new shape.
					mergeCurrentLayerWithTemp();

					state.startPosition = float32x2(pointerPosition) * viewToCanvasTransform;
					state.endPosition = state.startPosition;
					state.userState = UserState::Draw;
					state.notEmpty = false;

					enableTempLayerRendering = false;
				}
			}
			else
			{
				state.startPosition = float32x2(pointerPosition) * viewToCanvasTransform;
				state.endPosition = state.startPosition;
				state.userState = UserState::Draw;
				state.notEmpty = false;
				state.outOfDate = false;
			}
		}
		else if (state.userState == UserState::Draw)
		{
			float32x2 canvasSpacePointerPosition = float32x2(pointerPosition) * viewToCanvasTransform;
			if (state.endPosition != canvasSpacePointerPosition)
			{
				state.endPosition = canvasSpacePointerPosition;

				state.notEmpty = checkIfIsDrawable();
				if (state.notEmpty)
				{
					state.outOfDate = false;
					render();
				}

				enableTempLayerRendering = state.notEmpty;
			}
		}
		/*else if (state.userState == UserState::Modify)
		{
			if (state.prevModifyPointerPosition != pointerPosition)
			{
				state.prevModifyPointerPosition = pointerPosition;

				float32x2 canvasSpacePointerPosition = float32x2(pointerPosition) * viewToCanvasTransform;
				float32x2 newAnchorPosition = canvasSpacePointerPosition - state.pointerFromAnchorOffset;
				if (state.anchorIndex)
					state.endPosition = newAnchorPosition;
				else
					state.startPosition = newAnchorPosition;

				state.notEmpty = checkIfIsDrawable();
				if (state.notEmpty)
				{
					state.outOfDate = false;
					render();
				}

				enableTempLayerRendering = state.notEmpty;
			}
		}*/
	}
	else
		state.userState = UserState::Standby;

	if (state.outOfDate && state.notEmpty)
	{
		state.outOfDate = false;
		render();
	}
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
		device->copyTexture(layerTextures[currentLayer], tempTexture, selection.leftTop, selection);

		resetInstrument();
	}
}

void CanvasManager::mergeCurrentLayerWithTemp()
{
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

	device->setRenderTarget(layerTextures[currentLayer]);
	device->setViewport(rectu32(0, 0, canvasSize));
	device->setScissorRect(selection);
	device->setTransform2D(Matrix2x3::Identity());
	device->setTexture(tempTexture);

	device->draw2D(PrimitiveType::TriangleList, Effect::TexturedUnorm,
		quadVertexBuffer, 0, sizeof(VertexTexturedUnorm2D), 6);
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

BrushSettings& CanvasManager::setInstrument_brush(Color color, float32 width, bool blendEnabled)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentSettings.brush.color = color;
	instrumentSettings.brush.width = width;
	instrumentSettings.brush.blendEnabled = blendEnabled;
	currentInstrument = Instrument::Brush;

	return instrumentSettings.brush;
}

LineSettings& CanvasManager::setInstrument_line(XLib::Color color, float32 width,
	bool roundedStart, bool roundedEnd)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentSettings.line.color = color;
	instrumentSettings.line.width = width;
	instrumentSettings.line.roundedStart = roundedStart;
	instrumentSettings.line.roundedEnd = roundedEnd;
	instrumentState.line.userState = InstrumentState_Line::UserState::Standby;
	instrumentState.line.notEmpty = false;
	instrumentState.line.outOfDate = false;
	instrumentState.line.apply = false;
	currentInstrument = Instrument::Line;

	return instrumentSettings.line;
}

ShapeSettings& CanvasManager::setInstrument_shape(XLib::Color fillColor,
	XLib::Color borderColor, float32 borderWidth, Shape shape)
{
	disableCurrentLayerRendering = false;
	enableTempLayerRendering = false;

	instrumentSettings.shape.fillColor = fillColor;
	instrumentSettings.shape.borderColor = borderColor;
	instrumentSettings.shape.borderWidth = borderWidth;
	instrumentSettings.shape.shape = shape;
	instrumentState.shape.userState = InstrumentState_Shape::UserState::Standby;
	instrumentState.shape.notEmpty = false;
	instrumentState.shape.outOfDate = false;
	instrumentState.shape.apply = false;
	currentInstrument = Instrument::Shape;

	return instrumentSettings.shape;
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

		case Instrument::Shape:
			instrumentState.shape.outOfDate = true;
			break;

		case Instrument::BrightnessContrastGammaFilter:
			instrumentState.brightnessContrastGammaFilter.outOfDate = true;
			break;
	}
}

void CanvasManager::applyInstrument()
{
	switch (currentInstrument)
	{
		case Instrument::Line:
			instrumentState.line.apply = true;
			break;

		case Instrument::Shape:
			instrumentState.shape.apply = true;
			break;

		case Instrument::BrightnessContrastGammaFilter:
			instrumentState.brightnessContrastGammaFilter.apply = true;
			break;
	}
}