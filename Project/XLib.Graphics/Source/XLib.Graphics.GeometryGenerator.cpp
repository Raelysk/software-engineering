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

    device->updateBuffer(gpuVertexBuffer, cpuVertexBuffer, 0, vertexBufferBytesUsed);
    device->draw2D(PrimitiveType::TriangleList, Effect::PerVertexColor, gpuVertexBuffer,
        0, sizeof(VertexColor2D), vertexBufferBytesUsed / sizeof(VertexColor2D));

    vertexBufferBytesUsed = 0;
}

void GeometryGenerator::drawLine(float32x2 start, float32x2 end, float32 width, Color color)
{
    VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

    float32x2 w = VectorMath::NormalLeft(VectorMath::Normalize(end - start)) * (width / 2.0f);
    vertices[0] = { end + w, color };
    vertices[1] = { start + w, color };
    vertices[2] = { start - w, color };
    vertices[3] = vertices[0];
    vertices[4] = vertices[2];
    vertices[5] = { end - w, color };
}

void GeometryGenerator::drawRect(const rectf32& rect, Color color)
{
    VertexColor2D *vertices = allocateVertices<VertexColor2D>(6);

    vertices[0] = { { rect.left,   rect.top    }, color };
    vertices[1] = { { rect.right,  rect.top    }, color };
    vertices[2] = { { rect.right,  rect.bottom }, color };
    vertices[3] = { { rect.left,   rect.top    }, color };
    vertices[4] = { { rect.right,  rect.bottom }, color };
    vertices[5] = { { rect.left,   rect.bottom }, color };
}

void GeometryGenerator::drawRectShadow(const rectf32& rect, float32 width, Color color)
{
    VertexColor2D *vertices = allocateVertices<VertexColor2D>(36);

    const uint32 innerColor = color;
    const uint32 outerColor = 0x00000000_rgba; // TODO: rgb from inner color.

    vertices[ 0] = { { rect.left - width, rect.top    }, outerColor };
    vertices[ 1] = { { rect.left,         rect.top    }, innerColor };
    vertices[ 2] = { { rect.left - width, rect.bottom }, outerColor };
    vertices[ 3] = { { rect.left,         rect.top    }, innerColor };
    vertices[ 4] = { { rect.left,         rect.bottom }, innerColor };
    vertices[ 5] = { { rect.left - width, rect.bottom }, outerColor };

    vertices[ 6] = { { rect.right,         rect.top    }, innerColor };
    vertices[ 7] = { { rect.right + width, rect.top    }, outerColor };
    vertices[ 8] = { { rect.right,         rect.bottom }, innerColor };
    vertices[ 9] = { { rect.right + width, rect.top    }, outerColor };
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

    vertices[0] = { { rect.left,   rect.top    }, topColor    };
    vertices[1] = { { rect.right,  rect.top    }, topColor    };
    vertices[2] = { { rect.right,  rect.bottom }, bottomColor };
    vertices[3] = { { rect.left,   rect.top    }, topColor    };
    vertices[4] = { { rect.right,  rect.bottom }, bottomColor };
    vertices[5] = { { rect.left,   rect.bottom }, bottomColor };
}

inline void* GeometryGenerator::allocateVertices(uint32 size)
{
    if (vertexBufferBytesUsed + size > vertexBufferSize)
        flush();

    void *result = cpuVertexBuffer + vertexBufferBytesUsed;
    vertexBufferBytesUsed += size;

    return result;
}