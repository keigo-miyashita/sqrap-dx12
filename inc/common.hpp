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
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <AS.hpp>
#include <Application.hpp>
#include <Buffer.hpp>
#include <Camera.hpp>
#include <Command.hpp>
#include <Descriptorheap.hpp>
#include <Descriptormanager.hpp>
#include <Descriptortable.hpp>
#include <Device.hpp>
#include <Dxc.hpp>
#include <Fence.hpp>
#include <Gui.hpp>
#include <Indirect.hpp>
#include <Mesh.hpp>
#include <Pipeline.hpp>
#include <Rootsignature.hpp>
#include <Shader.hpp>
#include <Swapchain.hpp>
#include <Workgraph.hpp>

void DebugOutputFormatString(const char* format, ...);