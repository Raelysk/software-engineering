#include <d3d11.h>
#include <dxgi1_3.h>

#include <XLib.Util.h>
#include <XLib.Memory.h>
#include <XLib.Platform.D3D11.Helpers.h>
#include <XLib.Platform.DXGI.Helpers.h>

#include "XLib.Graphics.h"
#include "XLib.Graphics.Internal.Shaders.h"

using namespace XLib;
using namespace XLib::Platform;
using namespace XLib::Graphics;
using namespace XLib::Graphics::Internal;

static_assert(
	uint32(PrimitiveType::Points) == D3D11_PRIMITIVE_TOPOLOGY_POINTLIST &&
	uint32(PrimitiveType::LineList) == D3D11_PRIMITIVE_TOPOLOGY_LINELIST &&
	uint32(PrimitiveType::LineStrip) == D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP &&
	uint32(PrimitiveType::TriangleList) == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST &&
	uint32(PrimitiveType::TriangleStrip) == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	"invalid PrimitiveType value");

COMPtr<IDXGIFactory3> Device::dxgiFactory;

struct TransformConstants
{
    float32x4 tranfromRow0;
    float32x4 tranfromRow1;

    inline void set(const Matrix2x3& transform)
    {
        tranfromRow0 = { transform[0][0], transform[0][1], transform[0][2], 0.0f };
        tranfromRow1 = { transform[1][0], transform[1][1], transform[1][2], 0.0f };
    }
};

bool Device::initialize()
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL(0);
	D3D_FEATURE_LEVEL requestedFeatureLevel = D3D_FEATURE_LEVEL_10_0;

	uint32 deviceCreationFlags = 0;
#ifdef _DEBUG
	deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		deviceCreationFlags, &requestedFeatureLevel, 1, D3D11_SDK_VERSION,
		d3dDevice.initRef(), &featureLevel, d3dContext.initRef());

	if (!dxgiFactory.isInitialized())
		CreateDXGIFactory1(dxgiFactory.uuid(), dxgiFactory.voidInitRef());

	D3D11_INPUT_ELEMENT_DESC d3dVertexColor2DILDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    D3D11_INPUT_ELEMENT_DESC d3dVertexTexturedUnorm2DILDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	d3dDevice->CreateInputLayout(d3dVertexColor2DILDesc, countof(d3dVertexColor2DILDesc),
		Shaders::Color2DVS.data, Shaders::Color2DVS.size, d3dColor2DIL.initRef());
	d3dDevice->CreateVertexShader(Shaders::Color2DVS.data, Shaders::Color2DVS.size, nullptr, d3dColor2DVS.initRef());
	d3dDevice->CreatePixelShader(Shaders::ColorPS.data, Shaders::ColorPS.size, nullptr, d3dColorPS.initRef());

    d3dDevice->CreateInputLayout(d3dVertexTexturedUnorm2DILDesc, countof(d3dVertexTexturedUnorm2DILDesc),
        Shaders::Textured2DVS.data, Shaders::Textured2DVS.size, d3dTexturedUnorm2DIL.initRef());
    d3dDevice->CreateVertexShader(Shaders::Textured2DVS.data, Shaders::Textured2DVS.size, nullptr, d3dTextured2DVS.initRef());
    d3dDevice->CreatePixelShader(Shaders::TexturedPS.data, Shaders::TexturedPS.size, nullptr, d3dTexturedPS.initRef());

	d3dDevice->CreateRasterizerState(
		&D3D11RasterizerDesc(D3D11_FILL_SOLID, D3D11_CULL_BACK, false, 0, 0.0f, 0.0f, false, true),
		d3dDefaultRasterizerState.initRef());

    d3dDevice->CreateSamplerState(
		&D3D11SamplerDesc(D3D11_FILTER_MIN_MAG_MIP_LINEAR),
		d3dDefaultSamplerState.initRef());

    d3dDevice->CreateBlendState(
        &D3D11BlendDesc(
            D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_INV_SRC_ALPHA,
            D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE),
        d3dDefaultBlendState.initRef());

	d3dDevice->CreateBuffer(
		&D3D11BufferDesc(sizeof(TransformConstants), D3D11_BIND_CONSTANT_BUFFER),
		nullptr, d3dTransformConstantBuffer.initRef());
	d3dDevice->CreateBuffer(
		&D3D11BufferDesc(customEffectConstantsSizeLimit, D3D11_BIND_CONSTANT_BUFFER),
		nullptr, d3dCustomEffectConstantBuffer.initRef());

    transform = Matrix2x3::Identity();
    transformUpToDate = false;

	return true;
}

void Device::clear(RenderTarget& renderTarget, Color color)
{
	d3dContext->ClearRenderTargetView(renderTarget.d3dRTV, to<float32*>(&color.toF32x4()));
}

void Device::setRenderTarget(RenderTarget& renderTarget)
{
	ID3D11RenderTargetView *d3dRTVs = renderTarget.d3dRTV;
	d3dContext->OMSetRenderTargets(1, &d3dRTVs, nullptr);
}

void Device::setViewport(const rectu32& viewport)
{
    this->viewport = viewport;
    transformUpToDate = false;
}

void Device::setScissorRect(const rectu32& rect)
{
	d3dContext->RSSetScissorRects(1, &D3D11Rect(rect.left, rect.top, rect.right, rect.bottom));
}

void Device::setTransform2D(const Matrix2x3& transform)
{
    this->transform = transform;
    transformUpToDate = false;
}

void Device::setTexture(Texture& texture, uint32 slot)
{
    ID3D11ShaderResourceView *d3dSRVs[] = { texture.d3dSRV };
    d3dContext->PSSetShaderResources(0, 1, d3dSRVs);
}

void Device::setCustomEffectConstants(const void* data, uint32 size)
{
	Memory::Copy(customEffectConstantsBuffer, data, size);
	d3dContext->UpdateSubresource(d3dCustomEffectConstantBuffer,
		0, nullptr, customEffectConstantsBuffer, 0, 0);
}

void Device::uploadBuffer(Buffer& buffer, const void* srcData, uint32 baseOffset, uint32 size)
{
    d3dContext->UpdateSubresource(buffer.d3dBuffer, 0,
        &D3D11Box(baseOffset, baseOffset + size), srcData, 0, 0);
}

void Device::uploadTexture(Texture& texture, const rectu32& region,
    const void* srcData, uint32 srcDataStride)
{
	if (!srcDataStride)
		srcDataStride = (region.right - region.left) * 4;

    d3dContext->UpdateSubresource(texture.d3dTexture, 0,
        &D3D11Box(region.left, region.right, region.top, region.bottom), srcData, srcDataStride, 0);
}

void Device::copyTexture(Texture& dstTexture, Texture& srcTexture, uint32x2 dstLocation, const rectu32& srcRegion)
{
	d3dContext->CopySubresourceRegion(dstTexture.d3dTexture, 0, dstLocation.x, dstLocation.y, 0,
		srcTexture.d3dTexture, 0, &D3D11Box(srcRegion.left, srcRegion.right, srcRegion.top, srcRegion.bottom));
}

void Device::draw2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
	uint32 baseOffset, uint32 vertexStride, uint32 vertexCount)
{
	ID3D11InputLayout *d3dIL = nullptr;
	ID3D11VertexShader *d3dVS = nullptr;
	ID3D11PixelShader *d3dPS = nullptr;
    ID3D11SamplerState *d3dSS = nullptr;

	switch (effect)
	{
		case Effect::PerVertexColor:
			d3dIL = d3dColor2DIL;
			d3dVS = d3dColor2DVS;
			d3dPS = d3dColorPS;
			break;

		//case Effect::GlobalColor:
			//break;

        case Effect::TexturedUnorm:
            d3dIL = d3dTexturedUnorm2DIL;
            d3dVS = d3dTextured2DVS;
            d3dPS = d3dTexturedPS;
            d3dSS = d3dDefaultSamplerState;
            break;

		default:
			return;
	}

	d3dContext->IASetInputLayout(d3dIL);
	d3dContext->VSSetShader(d3dVS, nullptr, 0);
	d3dContext->PSSetShader(d3dPS, nullptr, 0);
    if (d3dSS)
        d3dContext->PSSetSamplers(0, 1, &d3dSS);

	ID3D11Buffer *d3dVSCB = d3dTransformConstantBuffer;
	d3dContext->VSSetConstantBuffers(0, 1, &d3dVSCB);

    if (!transformUpToDate)
    {
        Matrix2x3 ndcToScreenSpaceTransform =
            Matrix2x3::VerticalReflection() *
            Matrix2x3::Translation(-1.0f, -1.0f) *
            Matrix2x3::Scale(2.0f / viewport.getWidth(), 2.0f / viewport.getHeight()) *
            Matrix2x3::Translation(float32(viewport.left), float32(viewport.top));

		TransformConstants transformConstants;
		transformConstants.set(ndcToScreenSpaceTransform * transform);
        d3dContext->UpdateSubresource(d3dTransformConstantBuffer,
			0, nullptr, &transformConstants, 0, 0);

        transformUpToDate = true;
    }

    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY(primitiveType));
	d3dContext->RSSetState(d3dDefaultRasterizerState);
    d3dContext->OMSetBlendState(d3dDefaultBlendState, nullptr, 0xFFFFFFFF);

    d3dContext->RSSetViewports(1, &D3D11ViewPort(float32(viewport.left), float32(viewport.top),
        float32(viewport.right - viewport.left), float32(viewport.bottom - viewport.top)));

    {
        ID3D11Buffer *d3dBuffer = vertexBuffer.d3dBuffer;
        UINT stride = vertexStride;
        UINT offset = baseOffset;
        d3dContext->IASetVertexBuffers(0, 1, &d3dBuffer, &stride, &offset);
    }

	d3dContext->Draw(vertexCount, 0);
}

void Device::draw2D(PrimitiveType primitiveType, CustomEffect& effect, Buffer& vertexBuffer,
	uint32 baseOffset, uint32 vertexStride, uint32 vertexCount)
{
	d3dContext->IASetInputLayout(effect.d3dIL);
	d3dContext->VSSetShader(effect.d3dVS, nullptr, 0);
	d3dContext->PSSetShader(effect.d3dPS, nullptr, 0);

	ID3D11SamplerState *d3dSS = d3dDefaultSamplerState;
	d3dContext->PSSetSamplers(0, 1, &d3dSS);

	ID3D11Buffer *d3dVSCB = d3dTransformConstantBuffer;
	d3dContext->VSSetConstantBuffers(0, 1, &d3dVSCB);

	ID3D11Buffer *d3dPSCB = d3dCustomEffectConstantBuffer;
	d3dContext->PSSetConstantBuffers(0, 1, &d3dPSCB);

	if (!transformUpToDate)
	{
		Matrix2x3 ndcToScreenSpaceTransform =
			Matrix2x3::VerticalReflection() *
			Matrix2x3::Translation(-1.0f, -1.0f) *
			Matrix2x3::Scale(2.0f / viewport.getWidth(), 2.0f / viewport.getHeight()) *
			Matrix2x3::Translation(float32(viewport.left), float32(viewport.top));

		TransformConstants transformConstants;
		transformConstants.set(ndcToScreenSpaceTransform * transform);
		d3dContext->UpdateSubresource(d3dTransformConstantBuffer,
			0, nullptr, &transformConstants, 0, 0);

		transformUpToDate = true;
	}

	d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY(primitiveType));
	d3dContext->RSSetState(d3dDefaultRasterizerState);
	d3dContext->OMSetBlendState(d3dDefaultBlendState, nullptr, 0xFFFFFFFF);

	d3dContext->RSSetViewports(1, &D3D11ViewPort(float32(viewport.left), float32(viewport.top),
		float32(viewport.right - viewport.left), float32(viewport.bottom - viewport.top)));

	{
		ID3D11Buffer *d3dBuffer = vertexBuffer.d3dBuffer;
		UINT stride = vertexStride;
		UINT offset = baseOffset;
		d3dContext->IASetVertexBuffers(0, 1, &d3dBuffer, &stride, &offset);
	}

	d3dContext->Draw(vertexCount, 0);
}

bool Device::createCustomEffect(CustomEffect& effect, Effect defaultInputLayoutEffect,
	const void* psBytecode, uint32 psBytecodeSize)
{
	ID3D11InputLayout *d3dIL = nullptr;
	ID3D11VertexShader *d3dVS = nullptr;

	switch (defaultInputLayoutEffect)
	{
	case Effect::PerVertexColor:
		d3dIL = d3dColor2DIL;
		d3dVS = d3dColor2DVS;
		break;

	case Effect::TexturedUnorm:
		d3dIL = d3dTexturedUnorm2DIL;
		d3dVS = d3dTextured2DVS;
		break;

	default:
		return false;
	}

	return effect.inititalize(d3dDevice, d3dIL, d3dVS, psBytecode, psBytecodeSize);
}

// Buffer ===================================================================================//

bool Buffer::initialize(ID3D11Device* d3dDevice, uint32 size, const void* initialData)
{
	d3dDevice->CreateBuffer(
		&D3D11BufferDesc(size, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER),
		initialData ? &D3D11SubresourceData(initialData, size, 0) : nullptr,
		d3dBuffer.initRef());

	return true;
}

// Texture ==================================================================================//

bool Texture::initialize(ID3D11Device* d3dDevice, uint32 width, uint32 height,
    const void* initialData, uint32 initialDataStride, bool enableRenderTarget)
{
    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (enableRenderTarget)
        bindFlags |= D3D11_BIND_RENDER_TARGET;

    d3dDevice->CreateTexture2D(
        &D3D11Texture2DDesc(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, bindFlags),
        initialData ? &D3D11SubresourceData(initialData, initialDataStride, 0) : nullptr,
        d3dTexture.initRef());
    d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, d3dSRV.initRef());

    return true;
}

// RenderTarget =============================================================================//

bool RenderTarget::initialize(ID3D11Device* d3dDevice, ID3D11Texture2D* d3dTexture)
{
	d3dDevice->CreateRenderTargetView(d3dTexture, nullptr, d3dRTV.initRef());

	return true;
}

// TextureRenderTarget ======================================================================//

bool TextureRenderTarget::initialize(ID3D11Device* d3dDevice, uint32 width, uint32 height,
    const void* initialData, uint32 initialDataStride)
{
    Texture::initialize(d3dDevice, width, height, initialData, initialDataStride, true);
    RenderTarget::initialize(d3dDevice, Texture::getD3D11Texture2D());

    return true;
}

// WindowRenderTarget =======================================================================//

bool WindowRenderTarget::initialize(ID3D11Device* d3dDevice,
	IDXGIFactory3* dxgiFactory, void* hWnd, uint32 width, uint32 height)
{
	dxgiFactory->CreateSwapChainForHwnd(d3dDevice, HWND(hWnd),
		&DXGISwapChainDesc1(width, height), nullptr, nullptr, dxgiSwapChain.initRef());

	COMPtr<ID3D11Texture2D> d3dBackTexture;
	dxgiSwapChain->GetBuffer(0, d3dBackTexture.uuid(), d3dBackTexture.voidInitRef());

	return RenderTarget::initialize(d3dDevice, d3dBackTexture);
}

bool WindowRenderTarget::resize(ID3D11Device* d3dDevice, uint32 width, uint32 height)
{
	RenderTarget::~RenderTarget();

	dxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

	COMPtr<ID3D11Texture2D> d3dBackTexture;
	dxgiSwapChain->GetBuffer(0, d3dBackTexture.uuid(), d3dBackTexture.voidInitRef());

	return RenderTarget::initialize(d3dDevice, d3dBackTexture);
}

void WindowRenderTarget::present(bool sync)
{
	dxgiSwapChain->Present(sync ? 1 : 0, 0);
}

// CustomEffect =============================================================================//

bool CustomEffect::inititalize(ID3D11Device* d3dDevice, uint32 ilElementCount,
	const CustomEffectInputLayoutElement* ilElements,
	const void* vsBytecode, uint32 vsBytecodeSize,
	const void* psBytecode, uint32 psBytecodeSize)
{
	return false;
}

bool CustomEffect::inititalize(ID3D11Device* d3dDevice, ID3D11InputLayout* d3dIL,
	ID3D11VertexShader* d3dVS, const void* psBytecode, uint32 psBytecodeSize)
{
	d3dDevice->CreatePixelShader(psBytecode, psBytecodeSize, nullptr, d3dPS.initRef());

	this->d3dIL = d3dIL;
	this->d3dVS = d3dVS;

	return true;
}