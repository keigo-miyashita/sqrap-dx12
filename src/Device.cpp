#include "Device.hpp"

#include "Command.hpp"
#include "Fence.hpp"
#include "Gui.hpp"
#include "Shader.hpp"
#include "Swapchain.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION_MACRO; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = D3D12_SDK_PATH_MACRO; }

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

namespace {
	void EnableDebugLayer()
	{
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer.Reset();
	}
}

void Device::EnableFeatures(vector<UUID>& features) const
{
	auto result = D3D12EnableExperimentalFeatures(features.size(), features.data(), nullptr, nullptr);
	if (FAILED(result)) {
		throw std::runtime_error("Failed to D3D12EnableExperimentalFeatures" + to_string(result));
	}
}

void Device::CreateDXDevice(wstring gpuVendorName)
{
	UINT flagsDXGI = 0;
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to CreateDXGIFactory2" + to_string(result));
	}
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	vector<ComPtr<IDXGIAdapter>> allAdapters;
	ComPtr<IDXGIAdapter> optionalAdapter = nullptr;
	for (int i = 0; dxgiFactory_->EnumAdapters(i, optionalAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
		allAdapters.push_back(optionalAdapter);
	}
	for (auto adapter : allAdapters) {
		DXGI_ADAPTER_DESC adpDesc = {};
		adapter->GetDesc(&adpDesc);
		wstring strDesc = adpDesc.Description;
		if (strDesc.find(gpuVendorName.c_str()) != string::npos) {
			wcout << strDesc << endl;
			optionalAdapter = adapter;
			adapter.As(&adapter_);
			break;
		}
	}
	bool isCreatedDevice = false;
	for (auto level : levels) {
		result = D3D12CreateDevice(optionalAdapter.Get(), level, IID_PPV_ARGS(device_.ReleaseAndGetAddressOf()));
		if (SUCCEEDED(result)) {
			featureLevel_ = level;
			device_->SetName(L"Device");
			isCreatedDevice = true;
			break;
		}
	}
	if (!isCreatedDevice) {
		throw std::runtime_error("Failed to D3D12CreateDevice" + to_string(result));
	}
}

void Device::InitializeStableDevice()
{
	HRESULT result = device_->QueryInterface(IID_PPV_ARGS(stableDevice_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for Stabledevice" + to_string(result));
	}
}

void Device::InitializeLatestDevice()
{
	bool isInitializedLatestDevice = false;
	if (CheckWorkGraphSupport()) {
		HRESULT result = device_->QueryInterface(IID_PPV_ARGS(latestDevice_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to QueryInterface for LatestDevice" + to_string(result));
		}
	}
}

bool Device::CheckWorkGraphSupport()
{
	bool hasSupportWorkGraph = false;
	bool hasSupportMeshNode = false;
	D3D12_FEATURE_DATA_D3D12_OPTIONS21 Options;
	HRESULT result = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Options, sizeof(Options));
	if (FAILED(result)) {
		cerr << "Failed to check feature support" + to_string(result) << endl;
		return hasSupportWorkGraph;
	}
	if (Options.WorkGraphsTier == D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED) {
		cerr << "Device does not support for work graph" << endl;
		return hasSupportWorkGraph;
	}
	hasSupportWorkGraph = true;
	if (Options.WorkGraphsTier < D3D12_WORK_GRAPHS_TIER_1_1) {
		cerr << "Device does not support for mesh node of work graph" << endl;
		return hasSupportMeshNode;
	}
	hasSupportMeshNode = true;
	cout << "Device support for work graph" << endl;
	return hasSupportWorkGraph;
}

void Device::CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice)
{
	auto result = device_->QueryInterface(debugDevice.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		throw std::runtime_error("Failed to QueryInterface for DebugDevice" + to_string(result));
	}
}

Device::Device()
{

}

void Device::Init(wstring gpuVenorName)
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
	EnableFeatures(features);

	CreateDXDevice(gpuVenorName);

	InitializeStableDevice();

	InitializeLatestDevice();
}

void Device::Init(wstring gpuVenorName, ComPtr<ID3D12DebugDevice>& debugDevice)
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
	EnableFeatures(features);

	CreateDXDevice(gpuVenorName);

	InitializeLatestDevice();

#ifdef _DEBUG
	cout << "Try to create DebugDevice" << endl;
	CreateDebugDevice(debugDevice);
#endif // _DEBUG
}

void Device::ShowUsedVramSize()
{
	cout << "Try to get Vram info" << endl;
	DXGI_QUERY_VIDEO_MEMORY_INFO memInfo = {};
	if (SUCCEEDED(adapter_->QueryVideoMemoryInfo(
		0, 
		DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
		&memInfo)))
	{
		cout << "Scceeded to get memory info" << endl;
		/*std::wcout << L"Adapter: " << desc.Description << std::endl;*/
		std::cout << (memInfo.CurrentUsage / (1024.0 * 1024.0)) << " MB" << std::endl;
		std::cout << (memInfo.Budget / (1024.0 * 1024.0)) << " MB" << std::endl;
		return;
	}
	else {
		cerr << "Failed to get memory info" << endl;
	}
}

std::shared_ptr<AS> Device::CreateAS(UINT size, std::wstring name) const
{
	return make_shared<AS>(*this, size, name);
}

ASMeshHandle Device::CreateASMesh(CommandHandle command, std::string modelPath) const
{
	return make_shared<ASMesh>(*this, command, modelPath);
}

ASMeshHandle Device::CreateASMesh(CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const
{
	return make_shared<ASMesh>(*this, command, ASVertices, indices);
}

BLASHandle Device::CreateBLAS(CommandHandle command, ASMeshHandle mesh, std::wstring name) const
{
	return make_shared<BLAS>(*this, command, mesh, name);
}

BufferHandle Device::CreateBuffer(BufferType type, UINT strideSize, UINT numElement, std::wstring name) const
{
	return make_shared<Buffer>(*this, type, strideSize, numElement, name);
}

CommandHandle Device::CreateCommand(D3D12_COMMAND_LIST_TYPE commandType, std::wstring name) const
{
	return make_shared<Command>(*this, commandType, name);
}

ComputePipelineHandle Device::CreateComputePipeline(const ComputeDesc& desc, std::wstring name) const
{
	return make_shared<ComputePipeline>(*this, desc, name);
}

DescriptorManagerHandle Device::CreateDescriptorManager(HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, std::wstring name) const
{
	return make_shared<DescriptorManager>(*this, heapType, descManagerDesc, name);
}

FenceHandle Device::CreateFence(std::wstring name) const
{
	return make_shared<Fence>(*this, name);
}

GraphicsPipelineHandle Device::CreateGraphicsPipeline(const GraphicsDesc& desc, std::wstring name) const
{
	return make_shared<GraphicsPipeline>(*this, desc, name);
}

GUIHandle Device::CreateGUI(const HWND& hwnd) const
{
	return make_shared<GUI>(*this, hwnd);
}

IndirectHandle Device::CreateIndirect(std::initializer_list<IndirectDesc> indirectDescs, RootSignatureHandle rootSignature, UINT byteStride, std::wstring name) const
{
	return make_shared<Indirect>(*this, indirectDescs, rootSignature, byteStride, name);
}

MeshHandle Device::CreateMesh(CommandHandle command, std::string modelPath) const
{
	return make_shared<Mesh>(*this, command, modelPath);
}

MeshHandle Device::CreateMesh(CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const
{
	return make_shared<Mesh>(*this, command, vertices, indices);
}

RayTracingHandle Device::CreateRaytracing(StateObjectHandle stateObject, UINT width, UINT height, UINT depth, std::wstring name) const
{
	return make_shared<RayTracing>(*this, stateObject, width, height, depth, name);
}

RootSignatureHandle Device::CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name) const
{
	return make_shared<RootSignature>(*this, flag, rootParams, name);
}

StateObjectHandle Device::CreateStateObject(const StateObjectDesc soDesc, std::wstring name) const
{
	return make_shared<StateObject>(*this, soDesc, name);
}

SwapChainHandle Device::CreateSwapChain(CommandHandle command, const HWND& hwnd, SIZE winSize, std::wstring name) const
{
	return make_shared<SwapChain>(*this, command, hwnd, winSize, name);
}

TextureHandle Device::CreateTexture(TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name)
{
	return make_shared<Texture>(*this, texDim, type, strideSize, format, width, height, depth, name);
}

TLASHandle Device::CreateTLAS(CommandHandle command, const std::vector<TLASDesc>& tlasDescs, std::wstring name) const
{
	return make_shared<TLAS>(*this, command, tlasDescs, name);
}

WorkGraphHandle Device::CreateWorkGraph(StateObjectHandle stateObject, UINT maxInputRecords, UINT maxNodeInputs, std::wstring name) const
{
	return make_shared<WorkGraph>(*this, stateObject, maxInputRecords, maxNodeInputs, name);
}

ComPtr<IDXGIFactory7> Device::GetDXGIFactory() const
{
	return dxgiFactory_;
}

ComPtr<ID3D12Device> Device::GetDevice() const
{
	return device_;
}

ComPtr<StableDevice> Device::GetStableDevice() const
{
	return stableDevice_;
}

ComPtr<LatestDevice> Device::GetLatestDevice() const
{
	return latestDevice_;
}