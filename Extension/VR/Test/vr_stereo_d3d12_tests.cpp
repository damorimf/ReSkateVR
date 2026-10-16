// Exercise the actual D3D12 renderer and asynchronous timing on WARP.
#include "Extension/VR/vr_stereo.h"
#include <dxgi1_4.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>

using Microsoft::WRL::ComPtr;
using namespace dingosdk::vr::detail;
void require(bool ok) { if (!ok) throw std::runtime_error("D3D12 stereo check failed"); }
void check(HRESULT result) { require(SUCCEEDED(result)); }
int main() {
    try {
        ComPtr<IDXGIFactory4> factory;
        check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter> warp;
        check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
        ComPtr<ID3D12Device> device;
        check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
        ComPtr<ID3D12CommandQueue> queue;
        D3D12_COMMAND_QUEUE_DESC queue_desc{};
        check(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)));
        const auto texture = [&](DXGI_FORMAT format, D3D12_RESOURCE_STATES state) {
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width = desc.Height = 64;
            desc.DepthOrArraySize = desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            desc.Format = format;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
            ComPtr<ID3D12Resource> resource;
            check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&resource)));
            return resource;
        };
        auto color = texture(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto depth = texture(DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto left = texture(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto right = texture(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_RENDER_TARGET);
        ComPtr<ID3D12DescriptorHeap> rtvs;
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2};
        check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtvs)));
        auto rtv = rtvs->GetCPUDescriptorHandleForHeapStart();
        device->CreateRenderTargetView(color.Get(), nullptr, rtv);
        const auto color_rtv = rtv;
        rtv.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(depth.Get(), nullptr, rtv);
        ComPtr<ID3D12CommandAllocator> allocator;
        check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
        ComPtr<ID3D12GraphicsCommandList> list;
        check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        constexpr float yellow[]{0.25f, 0.5f, 0.75f, 1};
        constexpr float near_depth[]{0.02f, 0, 0, 0};
        list->ClearRenderTargetView(color_rtv, yellow, 0, nullptr);
        list->ClearRenderTargetView(rtv, near_depth, 0, nullptr);
        const auto barrier = [&](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
            D3D12_RESOURCE_BARRIER b{};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after};
            list->ResourceBarrier(1, &b);
        };
        barrier(color.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        barrier(depth.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        check(list->Close());
        ID3D12CommandList* commands[]{list.Get()};
        queue->ExecuteCommandLists(1, commands);
        DepthStereo stereo;
        std::string error;
        require(stereo.create(device.Get(), error));
        // Three allocator slots alternate copies, dilation and two-eye draws.
        // Repeated reuse must read the draw's timestamp before overwriting it.
        for (int i = 0; i < 8; ++i) {
            require(stereo.keep(queue.Get(), color.Get(), error));
            require(stereo.keep_depth(queue.Get(), depth.Get(), 1, error));
            require(stereo.render(queue.Get(), {left.Get(), right.Get()}, DXGI_FORMAT_R8G8B8A8_UNORM, {}, true, error));
        }
        require(stereo.wait_idle(10000));
        require(std::isfinite(stereo.gpu_ms()) && stereo.gpu_ms() > 0);
        std::printf("D3D12 stereo: completed GPU draw %.3f ms\n", stereo.gpu_ms());
        stereo.destroy();
        require(stereo.gpu_ms() == 0);
        require(stereo.create(device.Get(), error));
        require(stereo.gpu_ms() == 0);
        require(stereo.keep(queue.Get(), color.Get(), error));
        require(stereo.keep_depth(queue.Get(), depth.Get(), 0, error));
        require(stereo.render(queue.Get(), {left.Get(), right.Get()}, DXGI_FORMAT_R8G8B8A8_UNORM, {}, true, error));
        require(stereo.wait_idle(10000));
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
