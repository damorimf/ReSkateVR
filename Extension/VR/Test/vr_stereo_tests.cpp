// Runs the real stereo shader on Microsoft's software D3D11 device. No headset
// or game is required; synthetic depth edges exercise both eye directions.
#include "Extension/VR/vr_stereo_shader.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
void require(HRESULT result) {
    if (FAILED(result)) throw std::runtime_error("D3D call failed");
}
ComPtr<ID3DBlob> compile(const char* entry, const char* profile) {
    ComPtr<ID3DBlob> blob, errors;
    const auto result = D3DCompile(dingosdk::vr::detail::shader_source,
        sizeof(dingosdk::vr::detail::shader_source) - 1, "vr_stereo", nullptr, nullptr,
        entry, profile, D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &blob, &errors);
    if (errors) std::printf("%s\n", static_cast<const char*>(errors->GetBufferPointer()));
    require(result);
    return blob;
}
constexpr UINT width = 192, height = 8;
using Pixel = std::array<float, 4>;
struct Constants {
    UINT mode;
    float side, scale, max_shift, vignette;
    float padding[3]{};
};
}
int main() {
    try {
        const auto vertex = compile("vs_main", "vs_5_0");
        const auto pixel = compile("ps_main", "ps_5_0");
        (void)compile("ps_dilate", "ps_5_0");
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        require(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &device, nullptr, &context));
        ComPtr<ID3D11VertexShader> vs;
        ComPtr<ID3D11PixelShader> ps;
        require(device->CreateVertexShader(vertex->GetBufferPointer(), vertex->GetBufferSize(), nullptr, &vs));
        require(device->CreatePixelShader(pixel->GetBufferPointer(), pixel->GetBufferSize(), nullptr, &ps));
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> target, readback;
        require(device->CreateTexture2D(&desc, nullptr, &target));
        ComPtr<ID3D11RenderTargetView> rtv;
        require(device->CreateRenderTargetView(target.Get(), nullptr, &rtv));
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        require(device->CreateTexture2D(&desc, nullptr, &readback));
        D3D11_BUFFER_DESC buffer{};
        buffer.ByteWidth = sizeof(Constants); buffer.Usage = D3D11_USAGE_DEFAULT;
        buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ComPtr<ID3D11Buffer> constants;
        require(device->CreateBuffer(&buffer, nullptr, &constants));
        D3D11_SAMPLER_DESC sampler{};
        sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler.MaxLOD = D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> linear;
        require(device->CreateSamplerState(&sampler, &linear));
        const D3D11_VIEWPORT viewport{0, 0, float(width), float(height), 0, 1};
        context->RSSetViewports(1, &viewport);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs.Get(), nullptr, 0);
        context->PSSetShader(ps.Get(), nullptr, 0);
        auto* constant_ptr = constants.Get();
        context->PSSetConstantBuffers(0, 1, &constant_ptr);
        auto* sampler_ptr = linear.Get();
        context->PSSetSamplers(0, 1, &sampler_ptr);
        auto* target_ptr = rtv.Get();
        context->OMSetRenderTargets(1, &target_ptr, nullptr);
        for (const float side : {-1.0f, 1.0f}) {
            std::vector<Pixel> colors(width * height);
            std::vector<float> depths(width * height);
            // A red foreground, blue adjacent background, and unrelated green
            // scenery farther along the search. Mirror for the other eye.
            for (UINT y = 0; y < height; ++y) for (UINT x = 0; x < width; ++x) {
                const UINT sx = side > 0 ? x : width - 1 - x;
                const bool foreground = sx >= 92 && sx < 112;
                const bool unrelated = sx < 82;
                colors[y * width + x] = foreground ? Pixel{1, 0, 0, 1}
                    : unrelated ? Pixel{0, 1, 0, 1} : Pixel{0, 0, 1, 1};
                depths[y * width + x] = foreground ? 0.4f : unrelated ? 0.01f : 0.08f;
            }
            ComPtr<ID3D11Texture2D> color, depth;
            ComPtr<ID3D11ShaderResourceView> color_view, depth_view;
            desc.Usage = D3D11_USAGE_IMMUTABLE; desc.CPUAccessFlags = 0;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            D3D11_SUBRESOURCE_DATA data{colors.data(), width * UINT(sizeof(Pixel)), 0};
            require(device->CreateTexture2D(&desc, &data, &color));
            require(device->CreateShaderResourceView(color.Get(), nullptr, &color_view));
            desc.Format = DXGI_FORMAT_R32_FLOAT;
            data = {depths.data(), width * UINT(sizeof(float)), 0};
            require(device->CreateTexture2D(&desc, &data, &depth));
            require(device->CreateShaderResourceView(depth.Get(), nullptr, &depth_view));
            ID3D11ShaderResourceView* views[]{color_view.Get(), depth_view.Get()};
            context->PSSetShaderResources(0, 2, views);
            const Constants values{0x101, side, 0.3f, 0.12f, 0};
            context->UpdateSubresource(constants.Get(), 0, nullptr, &values, 0, 0);
            context->Draw(3, 0);
            context->CopyResource(readback.Get(), target.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            require(context->Map(readback.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            const auto* row = reinterpret_cast<const Pixel*>(
                static_cast<const unsigned char*>(mapped.pData) + (height / 2) * mapped.RowPitch);
            const auto output = row[side > 0 ? 100 : width - 1 - 100];
            context->Unmap(readback.Get(), 0);
            if (output[2] < 0.95f || output[0] > 0.05f || output[1] > 0.05f) {
                std::printf("Eye %.0f: gap RGB %.3f %.3f %.3f; expected adjacent blue background\n",
                    side, output[0], output[1], output[2]);
                return 1;
            }
        }
        std::puts("vr_stereo: real shader compiles; both eyes use adjacent background");
        return 0;
    } catch (const std::exception& error) {
        std::printf("vr_stereo: %s\n", error.what());
        return 1;
    }
}
