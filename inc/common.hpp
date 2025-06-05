#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <wrl.h>
#include <tchar.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <DirectXTex.h>
#include <directx/d3dx12.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <AS.hpp>
#include <Application.hpp>
#include <Camera.hpp>
#include <Command.hpp>
#include <Descriptormanager.hpp>
#include <Device.hpp>
#include <Dxc.hpp>
#include <Fence.hpp>
#include <Gui.hpp>
#include <Indirect.hpp>
#include <Mesh.hpp>
#include <Pipeline.hpp>
#include <Raytracing.hpp>
#include <Resource.hpp>
#include <Rootsignature.hpp>
#include <Shader.hpp>
#include <Swapchain.hpp>
#include <Workgraph.hpp>

using ASMeshHandle = std::shared_ptr<ASMesh>;
using BLASHandle = std::shared_ptr<BLAS>;
using BufferHandle = std::shared_ptr<Buffer>;
using CommandHandle = std::shared_ptr<Command>;
using ComputePipelineHandle = std::shared_ptr<ComputePipeline>;
using DescriptorManagerHandle = std::shared_ptr<DescriptorManager>;
using FenceHandle = std::shared_ptr<Fence>;
using GraphicsPipelineHandle = std::shared_ptr<GraphicsPipeline>;
using IndirectHandle = std::shared_ptr<Indirect>;
using MeshHandle = std::shared_ptr<Mesh>;
using RayTracingHandle = std::shared_ptr<RayTracing>;
using ResourceHandle = std::shared_ptr<Resource>;
using RootSignatureHandle = std::shared_ptr<RootSignature>;
using ShaderHandle = std::shared_ptr<Shader>;
using StateObjectHandle = std::shared_ptr<StateObject>;
using SwapChainHandle = std::shared_ptr<SwapChain>;
using TLASHandle = std::shared_ptr<TLAS>;
using TextureHandle = std::shared_ptr<Texture>;
using WorkGraphHandle = std::shared_ptr<WorkGraph>;


void DebugOutputFormatString(const char* format, ...);