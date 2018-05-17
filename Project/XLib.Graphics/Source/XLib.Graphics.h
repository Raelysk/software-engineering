#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Color.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>
#include <XLib.Math.Matrix2x3.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct ID3D11BlendState;
struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;

struct IDXGIFactory3;
struct IDXGISwapChain1;

namespace XLib::Graphics
{
	enum class PrimitiveType : uint8
	{
		None = 0,
		Points = 1,
		LineList = 2,
		LineStrip = 3,
		TriangleList = 4,
		TriangleStrip = 5,
	};

	enum class Effect : uint8
	{
		None = 0,
		PerVertexColor = 1,
		GlobalColor = 2,
        TexturedUnorm = 3,
	};

	struct VertexBase2D
	{
		float32x2 position;
	};

	struct VertexBase3D
	{
		float32x2 position;
	};

	struct VertexColor2D
	{
		float32x2 position;
		uint32 color;
	};

    struct VertexTexturedUnorm2D
    {
        float32x2 position;
        uint16x2 texcoordUnorm;
    };

	class Buffer : public XLib::NonCopyable
	{
		friend class Device;

	private:
		XLib::Platform::COMPtr<ID3D11Buffer> d3dBuffer;

		bool initialize(ID3D11Device* d3dDevice, uint32 size, const void* initialData);

	public:
		
	};

    class Texture : public XLib::NonCopyable
    {
        friend class Device;

    private:
        XLib::Platform::COMPtr<ID3D11Texture2D> d3dTexture;
        XLib::Platform::COMPtr<ID3D11ShaderResourceView> d3dSRV;

    protected:
        bool initialize(ID3D11Device* d3dDevice, uint32 width, uint32 height,
            const void* initialData, uint32 initialDataStride, bool enableRenderTarget);

        inline ID3D11Texture2D* getD3D11Texture2D() { return d3dTexture; }

    public:

    };

    class RenderTarget abstract : public XLib::NonCopyable
    {
        friend class Device;

    private:
        XLib::Platform::COMPtr<ID3D11RenderTargetView> d3dRTV;

    protected:
        bool initialize(ID3D11Device* d3dDevice, ID3D11Texture2D* d3dTexture);

    public:

    };

    class TextureRenderTarget : public Texture, public RenderTarget
    {
        friend class Device;

    private:
        bool initialize(ID3D11Device* d3dDevice, uint32 width, uint32 height,
            const void* initialData, uint32 initialDataStride);

    public:

    };

	class WindowRenderTarget : public RenderTarget
	{
		friend class Device;

	private:
        XLib::Platform::COMPtr<IDXGISwapChain1> dxgiSwapChain;

		bool initialize(ID3D11Device* d3dDevice, IDXGIFactory3* dxgiFactory,
			void* hWnd, uint32 width, uint32 height);
		bool resize(ID3D11Device* d3dDevice, uint32 width, uint32 height);

	public:
		WindowRenderTarget() = default;
		~WindowRenderTarget() = default;

		void present(bool sync = true);
	};

	class Device : public XLib::NonCopyable
	{
	private:
		static XLib::Platform::COMPtr<IDXGIFactory3> dxgiFactory;

		XLib::Platform::COMPtr<ID3D11Device> d3dDevice;
		XLib::Platform::COMPtr<ID3D11DeviceContext> d3dContext;

		XLib::Platform::COMPtr<ID3D11InputLayout> d3dColor2DIL;
		XLib::Platform::COMPtr<ID3D11VertexShader> d3dColor2DVS;
		XLib::Platform::COMPtr<ID3D11PixelShader> d3dColorPS;

        XLib::Platform::COMPtr<ID3D11InputLayout> d3dTexturedUnorm2DIL;
        XLib::Platform::COMPtr<ID3D11VertexShader> d3dTextured2DVS;
        XLib::Platform::COMPtr<ID3D11PixelShader> d3dTexturedPS;

        XLib::Platform::COMPtr<ID3D11SamplerState> d3dDefaultSamplerState;
        XLib::Platform::COMPtr<ID3D11BlendState> d3dDefaultBlendState;

		XLib::Platform::COMPtr<ID3D11Buffer> d3dConstantBuffer;

        rectu32 viewport = {};
        XLib::Matrix2x3 transform = {};
        bool transformUpToDate = false;

	public:
		bool initialize();

		void clear(RenderTarget& renderTarget, Color color);
		void setRenderTarget(RenderTarget& renderTarget);
        void setViewport(const rectu32& viewport);
		void setTransform2D(const Matrix2x3& transform);
        void setTexture(Texture& texture, uint32 slot = 0);

        void updateBuffer(Buffer& buffer, const void* srcData, uint32 baseOffset, uint32 size);
        void updateTexture(Texture& texture, const rectu32& region, const void* srcData, uint32 srcDataStride);

		void draw2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
			uint32 baseOffset, uint32 vertexStride, uint32 vertexCount);
		void drawIndexed2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
			uint32 baseOffset, uint32 vertexStride, uint32 vertexCount);

		inline bool createBuffer(Buffer& buffer, uint32 size, const void* initialData = nullptr)
			{ return buffer.initialize(d3dDevice, size, initialData); }
        inline bool createTexture(Texture& texture, uint32 width, uint32 height, const void* initialData = nullptr, uint32 initialDataStride = 0)
            { return texture.initialize(d3dDevice, width, height, initialData, initialDataStride, false); }
        inline bool createTextureRenderTarget(TextureRenderTarget& textureRenderTarget, uint32 width, uint32 height, const void* initialData = nullptr, uint32 initialDataStride = 0)
            { return textureRenderTarget.initialize(d3dDevice, width, height, initialData, initialDataStride); }
        inline bool createWindowRenderTarget(WindowRenderTarget& renderTarget, void* hWnd, uint32 width, uint32 height)
			{ return renderTarget.initialize(d3dDevice, dxgiFactory, hWnd, width, height); }
        inline bool resizeWindowRenderTarget(WindowRenderTarget& renderTarget, uint32 width, uint32 height)
            { return renderTarget.resize(d3dDevice, width, height); }
	};
}