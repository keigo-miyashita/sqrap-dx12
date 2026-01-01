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

namespace sqrp
{
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

	void Device::CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmdQueueDesc.NodeMask = 0;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		HRESULT result;
		result = device_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(graphicsCommandQueue_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommandQueue : " + to_string(result));
		}
		graphicsCommandQueue_->SetName(L"GraphicsCommandQueue");

		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		cmdQueueDesc.NodeMask = 0;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		result = device_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(computeCommandQueue_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw std::runtime_error("Failed to CreateCommandQueue : " + to_string(result));
		}
		computeCommandQueue_->SetName(L"ComputeCommandQueue");
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

		CreateCommandQueue();
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

	std::shared_ptr<AS> Device::CreateAS(std::wstring name, UINT size) const
	{
		return make_shared<AS>(*this, name, size);
	}

	ASMeshHandle Device::CreateASMesh(CommandHandle command, std::string modelPath) const
	{
		return make_shared<ASMesh>(*this, command, modelPath);
	}

	ASMeshHandle Device::CreateASMesh(std::wstring name, CommandHandle command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const
	{
		return make_shared<ASMesh>(*this, name, command, ASVertices, indices);
	}

	BLASHandle Device::CreateBLAS(std::wstring name, CommandHandle command, ASMeshHandle mesh) const
	{
		return make_shared<BLAS>(*this, name, command, mesh);
	}

	BufferHandle Device::CreateBuffer(std::wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag) const
	{
		return make_shared<Buffer>(*this, name, type, strideSize, numElement, resourceFlag);
	}

	CommandHandle Device::CreateCommand(std::wstring name, D3D12_COMMAND_LIST_TYPE commandType) const
	{
		return make_shared<Command>(*this, name, commandType);
	}

	ComputePipelineHandle Device::CreateComputePipeline(std::wstring name, const ComputeDesc& desc) const
	{
		return make_shared<ComputePipeline>(*this, name, desc);
	}

	DescriptorManagerHandle Device::CreateDescriptorManager(std::wstring name, HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags) const
	{
		return make_shared<DescriptorManager>(*this, name, heapType, descManagerDesc, flags);
	}

	FenceHandle Device::CreateFence(std::wstring name) const
	{
		return make_shared<Fence>(*this, name);
	}

	GraphicsPipelineHandle Device::CreateGraphicsPipeline(std::wstring name, const GraphicsDesc& desc) const
	{
		return make_shared<GraphicsPipeline>(*this, name, desc);
	}

	GUIHandle Device::CreateGUI(const HWND& hwnd) const
	{
		return make_shared<GUI>(*this, hwnd);
	}

	IndirectHandle Device::CreateIndirect(std::wstring name, std::initializer_list<IndirectDesc> indirectDescs, RootSignatureHandle rootSignature, UINT byteStride, UINT maxCommandCount, BufferHandle argumentBuffer, BufferHandle counterBuffer) const
	{
		return make_shared<Indirect>(*this, name, indirectDescs, rootSignature, byteStride, maxCommandCount, argumentBuffer, counterBuffer);
	}

	MeshHandle Device::CreateMesh(CommandHandle command, std::string modelPath) const
	{
		return make_shared<Mesh>(*this, command, modelPath);
	}

	MeshHandle Device::CreateMesh(std::wstring name, CommandHandle command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const
	{
		return make_shared<Mesh>(*this, name, command, vertices, indices);
	}

	MeshHandle Device::CreateMesh(std::wstring name, CommandHandle command, UINT verticesNum, UINT indicesNum) const
	{
		return make_shared<Mesh>(*this, name, command, verticesNum, indicesNum);
	}

	MeshPipelineHandle Device::CreateMeshPipeline(std::wstring name, const MeshDesc& desc) const
	{
		return make_shared<MeshPipeline>(*this, name, desc);
	}

	RayTracingHandle Device::CreateRaytracing(std::wstring name, StateObjectHandle stateObject, UINT width, UINT height, UINT depth) const
	{
		return make_shared<RayTracing>(*this, name, stateObject, width, height, depth);
	}

	RootSignatureHandle Device::CreateRootSignature(std::wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams) const
	{
		return make_shared<RootSignature>(*this, name, flag, rootParams);
	}

	StateObjectHandle Device::CreateStateObject(std::wstring name, const StateObjectDesc soDesc) const
	{
		return make_shared<StateObject>(*this, name, soDesc);
	}

	SwapChainHandle Device::CreateSwapChain(std::wstring name, CommandHandle command, const HWND& hwnd, SIZE winSize) const
	{
		return make_shared<SwapChain>(*this, name, command, hwnd, winSize);
	}

	TextureHandle Device::CreateTexture(std::wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth)
	{
		return make_shared<Texture>(*this, name, texDim, type, strideSize, format, width, height, depth);
	}

	TLASHandle Device::CreateTLAS(std::wstring name, CommandHandle command, const std::vector<TLASDesc>& tlasDescs) const
	{
		return make_shared<TLAS>(*this, name, command, tlasDescs);
	}

	WorkGraphHandle Device::CreateWorkGraph(std::wstring name, StateObjectHandle stateObject, UINT maxInputRecords, UINT maxNodeInputs) const
	{
		return make_shared<WorkGraph>(*this, name, stateObject, maxInputRecords, maxNodeInputs);
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

	ComPtr<ID3D12CommandQueue> Device::GetGraphicsCommandQueue() const
	{
		return graphicsCommandQueue_;
	}

	ComPtr<ID3D12CommandQueue> Device::GetComputeCommandQueue() const
	{
		return computeCommandQueue_;
	}
}