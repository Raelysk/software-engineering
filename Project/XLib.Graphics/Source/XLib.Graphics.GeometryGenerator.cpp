#include <XLib.Vectors.Math.h>

#include "XLib.Graphics.GeometryGenerator.h"

using namespace XLib;
using namespace XLib::Graphics;

void GeometryGenerator::initialize(Device& device, uint32 vertexBufferSize)
{
	this->device = &device;
	this->vertexBufferSize = vertexBufferSize;

	device.createBuffer(gpuVertexBuffer, vertexBufferSize);
	cpuVertexBuffer.resize(vertexBufferSize);
}

void GeometryGenerator::destroy()
{

}

void GeometryGenerator::discard()
{
	vertexBufferBytesUsed = 0;
}

void GeometryGenerator::flush()
{
	if (!vertexBufferBytesUsed)
		return;

	device->uploadBuffer(gpuVertexBuffer, cpuVertexBuffer, 0, vertexBufferBytesUsed);
	device->draw2D(PrimitiveType::TriangleList, Effect::PerVertexColor, gpuVertexBuffer,
		0, sizeof(VertexColor2D), vertexBufferBytesUsed / sizeof(VertexColor2D));

	vertexBufferBytesUsed = 0;
}

void GeometryGenerator::drawLine(float32x2 start, float32x2 end, float32 width,
	Color color, bool roundedStart, bool roundedEnd)
{
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

	float32x2 w = VectorMath::NormalLeft(VectorMath::Normalize(end - start)) * (width / 2.0f);
	vertices[0] = { end + w, color };
	vertices[1] = { start + w, color };
	vertices[2] = { start - w, color };
	vertices[3] = vertices[0];
	vertices[4] = vertices[2];
	vertices[5] = { end - w, color };

	if (roundedStart)
		drawLeftHalfEllipseOnDiameter(start - w, start + w, color);

	if (roundedEnd)
		drawLeftHalfEllipseOnDiameter(end + w, end - w, color);
}

void GeometryGenerator::drawFilledRect(const rectf32& rect, Color color)
{
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

	vertices[0] = { { rect.left,   rect.top    }, color };
	vertices[1] = { { rect.right,  rect.top    }, color };
	vertices[2] = { { rect.right,  rect.bottom }, color };
	vertices[3] = { { rect.left,   rect.top    }, color };
	vertices[4] = { { rect.right,  rect.bottom }, color };
	vertices[5] = { { rect.left,   rect.bottom }, color };
}

void GeometryGenerator::drawFilledRectWithBorder(const rectf32& rect,
	Color fillColor, Color borderColor, float32 borderWidth)
{
	float32 w = borderWidth * 0.5f;

	rectf32 outer = rect;
	outer.left -= w;
	outer.top -= w;
	outer.right += w;
	outer.bottom += w;

	if (outer.getWidth()  > borderWidth * 2.0f &&
		outer.getHeight() > borderWidth * 2.0f)
	{
		rectf32 inner = rect;
		inner.left += w;
		inner.top += w;
		inner.right -= w;
		inner.bottom -= w;

		VertexColor2D *vertices = allocateVertices<VertexColor2D>(30);

		// fill
		vertices[ 0] = { { inner.left,   inner.top    }, fillColor };
		vertices[ 1] = { { inner.right,  inner.top    }, fillColor };
		vertices[ 2] = { { inner.right,  inner.bottom }, fillColor };
		vertices[ 3] = { { inner.left,   inner.top    }, fillColor };
		vertices[ 4] = { { inner.right,  inner.bottom }, fillColor };
		vertices[ 5] = { { inner.left,   inner.bottom }, fillColor };

		// border left
		vertices[ 6] = { { outer.left,   outer.top    }, borderColor };
		vertices[ 7] = { { inner.left,   inner.top    }, borderColor };
		vertices[ 8] = { { inner.left,   inner.bottom }, borderColor };
		vertices[ 9] = { { outer.left,   outer.top    }, borderColor };
		vertices[10] = { { inner.left,   inner.bottom }, borderColor };
		vertices[11] = { { outer.left,   outer.bottom }, borderColor };

		// border top
		vertices[12] = { { outer.left,   outer.top    }, borderColor };
		vertices[13] = { { outer.right,  outer.top    }, borderColor };
		vertices[14] = { { inner.right,  inner.top    }, borderColor };
		vertices[15] = { { outer.left,   outer.top    }, borderColor };
		vertices[16] = { { inner.right,  inner.top    }, borderColor };
		vertices[17] = { { inner.left,   inner.top    }, borderColor };

		// border right
		vertices[18] = { { inner.right,  inner.top    }, borderColor };
		vertices[19] = { { outer.right,  outer.top    }, borderColor };
		vertices[20] = { { outer.right,  outer.bottom }, borderColor };
		vertices[21] = { { inner.right,  inner.top    }, borderColor };
		vertices[22] = { { outer.right,  outer.bottom }, borderColor };
		vertices[23] = { { inner.right,  inner.bottom }, borderColor };

		// border bottom
		vertices[24] = { { inner.left,   inner.bottom }, borderColor };
		vertices[25] = { { inner.right,  inner.bottom }, borderColor };
		vertices[26] = { { outer.right,  outer.bottom }, borderColor };
		vertices[27] = { { inner.left,   inner.bottom }, borderColor };
		vertices[28] = { { outer.right,  outer.bottom }, borderColor };
		vertices[29] = { { outer.left,   outer.bottom }, borderColor };
	}
	else
	{
		VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

		// fill with borders
		vertices[0] = { { outer.left,   outer.top    }, borderColor };
		vertices[1] = { { outer.right,  outer.top    }, borderColor };
		vertices[2] = { { outer.right,  outer.bottom }, borderColor };
		vertices[3] = { { outer.left,   outer.top    }, borderColor };
		vertices[4] = { { outer.right,  outer.bottom }, borderColor };
		vertices[5] = { { outer.left,   outer.bottom }, borderColor };
	}
}

void GeometryGenerator::drawRectShadow(const rectf32& rect, float32 width, Color color)
{
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(36);

	const uint32 innerColor = color;
	const uint32 outerColor = 0x00000000_rgba; // TODO: rgb from inner color.

	vertices[0] = { { rect.left - width, rect.top    }, outerColor };
	vertices[1] = { { rect.left,         rect.top    }, innerColor };
	vertices[2] = { { rect.left - width, rect.bottom }, outerColor };
	vertices[3] = { { rect.left,         rect.top    }, innerColor };
	vertices[4] = { { rect.left,         rect.bottom }, innerColor };
	vertices[5] = { { rect.left - width, rect.bottom }, outerColor };

	vertices[6] = { { rect.right,         rect.top    }, innerColor };
	vertices[7] = { { rect.right + width, rect.top    }, outerColor };
	vertices[8] = { { rect.right,         rect.bottom }, innerColor };
	vertices[9] = { { rect.right + width, rect.top    }, outerColor };
	vertices[10] = { { rect.right + width, rect.bottom }, outerColor };
	vertices[11] = { { rect.right,         rect.bottom }, innerColor };

	vertices[12] = { { rect.left,  rect.top },         innerColor };
	vertices[13] = { { rect.left,  rect.top - width }, outerColor };
	vertices[14] = { { rect.right, rect.top },         innerColor };
	vertices[15] = { { rect.left,  rect.top - width }, outerColor };
	vertices[16] = { { rect.right, rect.top - width }, outerColor };
	vertices[17] = { { rect.right, rect.top },         innerColor };

	vertices[18] = { { rect.left,  rect.bottom + width }, outerColor };
	vertices[19] = { { rect.left,  rect.bottom },         innerColor };
	vertices[20] = { { rect.right, rect.bottom + width }, outerColor };
	vertices[21] = { { rect.left,  rect.bottom },         innerColor };
	vertices[22] = { { rect.right, rect.bottom },         innerColor };
	vertices[23] = { { rect.right, rect.bottom + width }, outerColor };

	vertices[24] = { { rect.left,         rect.top         }, innerColor };
	vertices[25] = { { rect.left - width, rect.top         }, outerColor };
	vertices[26] = { { rect.left,         rect.top - width }, outerColor };

	vertices[27] = { { rect.left,         rect.bottom         }, innerColor };
	vertices[28] = { { rect.left,         rect.bottom + width }, outerColor };
	vertices[29] = { { rect.left - width, rect.bottom         }, outerColor };

	vertices[30] = { { rect.right,         rect.top         }, innerColor };
	vertices[31] = { { rect.right,         rect.top - width }, outerColor };
	vertices[32] = { { rect.right + width, rect.top         }, outerColor };

	vertices[33] = { { rect.right,         rect.bottom         }, innerColor };
	vertices[34] = { { rect.right + width, rect.bottom         }, outerColor };
	vertices[35] = { { rect.right,         rect.bottom + width }, outerColor };
}

void GeometryGenerator::drawVerticalGradientRect(const rectf32& rect, Color topColor, Color bottomColor)
{
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

	vertices[0] = { { rect.left,   rect.top    }, topColor };
	vertices[1] = { { rect.right,  rect.top    }, topColor };
	vertices[2] = { { rect.right,  rect.bottom }, bottomColor };
	vertices[3] = { { rect.left,   rect.top    }, topColor };
	vertices[4] = { { rect.right,  rect.bottom }, bottomColor };
	vertices[5] = { { rect.left,   rect.bottom }, bottomColor };
}

void GeometryGenerator::drawLeftHalfEllipseOnDiameter(float32x2 diameterStart, float32x2 diameterEnd,
	Color color, uint32 segmentCount)
{
	float32 angleStep = Math::PiF32 / float32(segmentCount);
	float32 angle = angleStep;
	float32x2 prevVertex = diameterEnd;

	float32x2 diameter = (diameterEnd - diameterStart) / 2.0f;
	float32x2 side = VectorMath::NormalLeft(diameter);

	VertexColor2D *vertices = allocateVertices<VertexColor2D>(segmentCount * 3);

	for (uint32 i = 0; i < segmentCount; i++)
	{
		float32 sin = Math::Sin(angle);
		float32 cos = Math::Cos(angle);

		float32x2 newVertex = diameterStart + diameter * (1.0f + cos) + side * sin;

		vertices[i * 3 + 0] = { diameterStart, color };
		vertices[i * 3 + 1] = { prevVertex, color };
		vertices[i * 3 + 2] = { newVertex, color };

		prevVertex = newVertex;
		angle += angleStep;
	}
}

void GeometryGenerator::drawEllipseBorder(float32x2 center, float32x2 radius,
	Color color, float32 width, uint32 segmentCount)
{
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(segmentCount * 6);

	float32 aspect = radius.y / radius.x;

	float32 w = width * 0.5f;
	float32 angleStep = Math::PiF32 * 2.0f / float32(segmentCount);
	float32x2 prevInner = float32x2(center.x + radius.x, center.y);
	float32x2 prevOuter = prevInner;
	prevInner.x -= w;
	prevOuter.x += w;

	float32x2 prevVertex = center + float32x2(Math::Cos(angleStep), Math::Sin(angleStep)) * radius;

	for (uint32 i = 0; i < segmentCount; i++)
	{
		float32 angle = float32(i + 1) * angleStep;
		float32x2 cosSin(Math::Cos(angle), Math::Sin(angle));

		float32x2 base = center + cosSin * radius;
		float32x2 normal = VectorMath::Normalize({ cosSin.x * aspect, cosSin.y });
		float32x2 normalOffset = normal * w;

		float32x2 newInner = base - normalOffset;
		float32x2 newOuter = base + normalOffset;

		vertices[i * 6 + 0] = { prevInner, color };
		vertices[i * 6 + 1] = { prevOuter, color };
		vertices[i * 6 + 2] = { newOuter,  color };
		vertices[i * 6 + 3] = { prevInner, color };
		vertices[i * 6 + 4] = { newOuter,  color };
		vertices[i * 6 + 5] = { newInner,  color };

		prevInner = newInner;
		prevOuter = newOuter;
	}
}

void GeometryGenerator::drawFilledEllipse(float32x2 center, float32x2 radius,
	Color color, uint32 segmentCount)
{
	uint32 fillTriangleCount = segmentCount - 2;
	VertexColor2D *vertices = allocateVertices<VertexColor2D>(fillTriangleCount * 3);

	float32x2 startVertex = center;
	startVertex.x += radius.x;

	float32 angleStep = Math::PiF32 * 2.0f / float32(segmentCount);
	float32x2 prevVertex = center + float32x2(Math::Cos(angleStep), Math::Sin(angleStep)) * radius;

	for (uint32 i = 0; i < fillTriangleCount; i++)
	{
		float32 angle = float32(i + 2) * angleStep;
		float32x2 newVertex = center + float32x2(Math::Cos(angle), Math::Sin(angle)) * radius;

		vertices[i * 3 + 0] = { startVertex, color };
		vertices[i * 3 + 1] = { prevVertex,  color };
		vertices[i * 3 + 2] = { newVertex,   color };

		prevVertex = newVertex;
	}
}

inline void* GeometryGenerator::allocateVertices(uint32 size)
{
	if (vertexBufferBytesUsed + size > vertexBufferSize)
		flush();

	void *result = cpuVertexBuffer + vertexBufferBytesUsed;
	vertexBufferBytesUsed += size;

	return result;
}