#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Color.h>
#include <XLib.Heap.h>

#include "XLib.Graphics.h"

namespace XLib::Graphics
{
	class GeometryGenerator : public XLib::NonCopyable
	{
	private:
		Device *device = nullptr;
		Buffer gpuVertexBuffer;
		HeapPtr<byte> cpuVertexBuffer;
		uint32 vertexBufferSize = 0;
		uint32 vertexBufferBytesUsed = 0;

		inline void* allocateVertices(uint32 size);

		template <typename VertexType>
		inline VertexType* allocateVertices(uint32 count)
			{ return (VertexType*)allocateVertices(count * sizeof(VertexType)); }

	public:
		GeometryGenerator() = default;
		~GeometryGenerator() = default;

		void initialize(Device& device, uint32 vertexBufferSize = 65536);
		void destroy();

		void discard();
		void flush();

		void drawLine(float32x2 start, float32x2 end, float32 width, Color color,
			bool roundedStart = false, bool roundedEnd = false);
		void drawFilledRect(const rectf32& rect, Color color);
		void drawFilledRectWithBorder(const rectf32& rect, Color fillColor, Color borderColor, float32 borderWidth);
		void drawRectShadow(const rectf32& rect, float32 width, Color color);
		void drawVerticalGradientRect(const rectf32& rect, Color topColor, Color bottomColor);
		void drawLeftHalfEllipseOnDiameter(float32x2 diameterStart, float32x2 diameterEnd, Color color, uint32 segmentCount = 16);
		void drawEllipseBorder(float32x2 center, float32x2 radius, Color color, float32 width, uint32 segmentCount = 64);
		void drawFilledEllipse(float32x2 center, float32x2 radius, Color color, uint32 segmentCount = 64);
	};
}