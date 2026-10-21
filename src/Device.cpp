#include "Device.hpp"

#include "Command.hpp"
#include "Fence.hpp"
#include "Gui.hpp"
#include "Shader.hpp"
#include "SwapChain.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION_MACRO; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = D3D12_SDK_PATH_MACRO; }

namespace {
	void EnableDebugLayer()
	{
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		if (FAILED(result))
		{
			throw runtime_error("[EnableDebugLayer] D3D12GetDebugInterface failed\nHRESULT = " + to_string(result));
		}
		debugLayer->EnableDebugLayer();
		debugLayer.Reset();
	}
}

namespace sqrp
{
	void DebugOutputFormatString(const char* format, ...)
	{
#ifdef _DEBUG
		va_list valist;
		va_start(valist, format);
		vprintf(format, valist);
		va_end(valist);
#endif
	}

	void Device::EnableFeatures(vector<UUID>& features) const
	{
		auto result = D3D12EnableExperimentalFeatures(features.size(), features.data(), nullptr, nullptr);
		if (FAILED(result)) {
			throw runtime_error("[Device::EnableFeatures] D3D12EnableExperimentalFeatures failed\nHRESULT = " + to_string(result));
		}
	}

	void Device::CreateDXDevice(wstring gpuVendorName)
	{
		UINT flagsDXGI = 0;
#ifdef _DEBUG
		flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Device::CreateDXDevice] CreateDXGIFactory2 failed\nHRESULT = " + to_string(result));
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
			throw runtime_error("[Device::CreateDXDevice] D3D12CreateDevice failed\nHRESULT = " + to_string(result));
		}
	}

	void Device::InitializeStableDevice()
	{
		HRESULT result = device_->QueryInterface(IID_PPV_ARGS(stableDevice_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Device::InitializeStableDevice] QueryInterface for StableDevice failed\nHRESULT = " + to_string(result));
		}
	}

	void Device::InitializeLatestDevice()
	{
		if (CheckWorkGraphSupport()) {
			HRESULT result = device_->QueryInterface(IID_PPV_ARGS(latestDevice_.ReleaseAndGetAddressOf()));
			if (FAILED(result)) {
				throw runtime_error("[Device::InitializeLatestDevice] QueryInterface for LatestDevice failed\nHRESULT = " + to_string(result));
			}
			CheckWorkGraphMeshNodeSupport();
		}
	}

	bool Device::CheckWorkGraphSupport()
	{
		bool hasSupportWorkGraph = false;
		D3D12_FEATURE_DATA_D3D12_OPTIONS21 Options;
		HRESULT result = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Options, sizeof(Options));
		if (FAILED(result)) {
			cerr << "[Device::CheckWorkGraphSupport] CheckFeatureSupport failed\nHRESULT = " + to_string(result) << endl;
			return false;
		}
		if (Options.WorkGraphsTier == D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED) {
			cerr << "[Device::CheckWorkGraphSupport] device does not support work graphs" << endl;
			return hasSupportWorkGraph;
		}
		hasSupportWorkGraph = true;
		return hasSupportWorkGraph;
	}

	bool Device::CheckWorkGraphMeshNodeSupport()
	{
		bool hasSupportMeshNode = false;
		D3D12_FEATURE_DATA_D3D12_OPTIONS21 Options;
		HRESULT result = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Options, sizeof(Options));
		if (FAILED(result)) {
			cerr << "[Device::CheckWorkGraphMeshNodeSupport] CheckFeatureSupport failed\nHRESULT = " + to_string(result) << endl;
			return false;
		}
		if (Options.WorkGraphsTier < D3D12_WORK_GRAPHS_TIER_1_1) {
			cerr << "[Device::CheckWorkGraphMeshNodeSupport] device does not support mesh nodes of work graphs" << endl;
			return hasSupportMeshNode;
		}
		hasSupportMeshNode = true;
		return hasSupportMeshNode;
	}

	void Device::CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice)
	{
		auto result = device_->QueryInterface(debugDevice.ReleaseAndGetAddressOf());
		if (FAILED(result)) {
			throw runtime_error("[Device::CreateDebugDevice] QueryInterface for DebugDevice failed\nHRESULT = " + to_string(result));
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
			throw runtime_error("[Device::CreateCommandQueue] CreateCommandQueue for graphics queue failed\nHRESULT = " + to_string(result));
		}
		graphicsCommandQueue_->SetName(L"GraphicsCommandQueue");

		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		cmdQueueDesc.NodeMask = 0;
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		result = device_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(computeCommandQueue_.ReleaseAndGetAddressOf()));
		if (FAILED(result)) {
			throw runtime_error("[Device::CreateCommandQueue] CreateCommandQueue for compute queue failed\nHRESULT = " + to_string(result));
		}
		computeCommandQueue_->SetName(L"ComputeCommandQueue");
	}

	template<typename T>
	void Device::DeferredReleaser(T* p) const
	{
		const UINT64 graphicsFenceVal = graphicsFence_->GetFenceVal() + 1;
		const UINT64 computeFenceVal = computeFence_->GetFenceVal() + (hasComputeCommand_ ? 1 : 0);
		deferredRelease_->Push(graphicsFenceVal, computeFenceVal, p);
	}

	Device::~Device()
	{
		if (graphicsFence_) {
			graphicsFence_->Signal();
			graphicsFence_->WaitSignal();
		}

		if (computeFence_) {
			computeFence_->Signal();
			computeFence_->WaitSignal();
		}

		if (deferredRelease_) {
			deferredRelease_->ReleaseAll();
		}
	}

	void Device::Init(wstring gpuVendorName)
	{
#ifdef _DEBUG
		EnableDebugLayer();
#endif

		vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
		EnableFeatures(features);

		CreateDXDevice(gpuVendorName);

		InitializeStableDevice();

		InitializeLatestDevice();

		CreateCommandQueue();

		graphicsFence_ = CreateFence(QueueType::Graphics, L"GraphicsFence");
		computeFence_ = CreateFence(QueueType::Compute, L"ComputeFence");

		deferredRelease_ = make_unique<DeferredRelease>();
	}

	void Device::Init(wstring gpuVendorName, ComPtr<ID3D12DebugDevice>& debugDevice)
	{
#ifdef _DEBUG
		EnableDebugLayer();
#endif

		vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
		EnableFeatures(features);

		CreateDXDevice(gpuVendorName);

		InitializeStableDevice();

		InitializeLatestDevice();

#ifdef _DEBUG
		cout << "Try to create DebugDevice" << endl;
		CreateDebugDevice(debugDevice);
#endif // _DEBUG

		CreateCommandQueue();

		graphicsFence_ = CreateFence(QueueType::Graphics, L"GraphicsFence");
		computeFence_ = CreateFence(QueueType::Compute, L"ComputeFence");

		deferredRelease_ = make_unique<DeferredRelease>();
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
			/*wcout << L"Adapter: " << desc.Description << endl;*/
			cout << (memInfo.CurrentUsage / (1024.0 * 1024.0)) << " MB" << endl;
			cout << (memInfo.Budget / (1024.0 * 1024.0)) << " MB" << endl;
			return;
		}
		else {
			cerr << "[Device::ShowUsedVramSize] QueryVideoMemoryInfo failed" << endl;
		}
	}

	// GPU のオブジェクトを持つものは、参照が 0 になってもすぐには消さず、GPU が使い終わるまで deferredRelease_ に預ける。
	// Fence（Device 自身のフェンスもこれで作る）と Gui（ImGui のグローバルな後始末をする）は対象外
	shared_ptr<AS> Device::CreateAS(wstring name, UINT64 size) const
	{
		return shared_ptr<AS>(new AS(*this, name, size),
			[this](AS* p) { DeferredReleaser(p); });
	}

	ASMeshHandle Device::CreateASMesh(const CommandHandle& command, string modelPath) const
	{
		return shared_ptr<ASMesh>(new ASMesh(*this, command, modelPath),
			[this](ASMesh* p) { DeferredReleaser(p); });
	}

	ASMeshHandle Device::CreateASMesh(wstring name, const CommandHandle& command, const vector<ASVertex>& ASVertices, const vector<uint32_t>& indices) const
	{
		return shared_ptr<ASMesh>(new ASMesh(*this, name, command, ASVertices, indices),
			[this](ASMesh* p) { DeferredReleaser(p); });
	}

	BLASHandle Device::CreateBLAS(wstring name, const CommandHandle& command, const ASMeshHandle& mesh) const
	{
		return shared_ptr<BLAS>(new BLAS(*this, name, command, mesh),
			[this](BLAS* p) { DeferredReleaser(p); });
	}

	BufferHandle Device::CreateBuffer(wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag) const
	{
		return BufferHandle(new Buffer(*this, name, type, strideSize, numElement, resourceFlag),
			[this](Buffer* p) { DeferredReleaser(p); });
	}

	CommandHandle Device::CreateCommand(wstring name, D3D12_COMMAND_LIST_TYPE commandType, UINT frameCount) const
	{
		if (commandType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
			hasComputeCommand_.store(true);
		}
		return shared_ptr<Command>(new Command(*this, name, commandType, frameCount),
			[this](Command* p) { DeferredReleaser(p); });
	}

	ComputePipelineHandle Device::CreateComputePipeline(wstring name, const ComputeDesc& desc) const
	{
		return shared_ptr<ComputePipeline>(new ComputePipeline(*this, name, desc),
			[this](ComputePipeline* p) { DeferredReleaser(p); });
	}

	DescriptorManagerHandle Device::CreateDescriptorManager(wstring name, HeapType heapType, initializer_list<DescriptorManagerDesc> descManagerDesc, D3D12_DESCRIPTOR_RANGE_FLAGS flags) const
	{
		return shared_ptr<DescriptorManager>(new DescriptorManager(*this, name, heapType, descManagerDesc, flags),
			[this](DescriptorManager* p) { DeferredReleaser(p); });
	}
	
	FenceHandle Device::CreateFence(QueueType queueType, wstring name) const
	{
		return make_shared<Fence>(*this, queueType, name);
	}

	GraphicsPipelineHandle Device::CreateGraphicsPipeline(wstring name, const GraphicsDesc& desc) const
	{
		return shared_ptr<GraphicsPipeline>(new GraphicsPipeline(*this, name, desc),
			[this](GraphicsPipeline* p) { DeferredReleaser(p); });
	}

	GuiHandle Device::CreateGui(const HWND& hwnd) const
	{
		return make_shared<Gui>(*this, hwnd);
	}

	IndirectHandle Device::CreateIndirect(wstring name, initializer_list<IndirectDesc> indirectDescs, const RootSignatureHandle& rootSignature, UINT byteStride, UINT maxCommandCount, const BufferHandle& argumentBuffer, const BufferHandle& counterBuffer) const
	{
		return shared_ptr<Indirect>(new Indirect(*this, name, indirectDescs, rootSignature, byteStride, maxCommandCount, argumentBuffer, counterBuffer),
			[this](Indirect* p) { DeferredReleaser(p); });
	}

	MeshHandle Device::CreateMesh(const CommandHandle& command, string modelPath) const
	{
		return shared_ptr<Mesh>(new Mesh(*this, command, modelPath),
			[this](Mesh* p) { DeferredReleaser(p); });
	}

	GLTFMeshHandle Device::CreateGLTFMesh(const CommandHandle& command, string modelPath) const
	{
		return shared_ptr<GLTFMesh>(new GLTFMesh(*this, command, modelPath),
			[this](GLTFMesh* p) { DeferredReleaser(p); });
	}

	MeshHandle Device::CreateMesh(wstring name, const CommandHandle& command, const vector<Vertex>& vertices, const vector<uint32_t>& indices) const
	{
		return shared_ptr<Mesh>(new Mesh(*this, name, command, vertices, indices),
			[this](Mesh* p) { DeferredReleaser(p); });
	}

	MeshHandle Device::CreateMesh(wstring name, const CommandHandle& command, UINT verticesNum, UINT indicesNum) const
	{
		return shared_ptr<Mesh>(new Mesh(*this, name, command, verticesNum, indicesNum),
			[this](Mesh* p) { DeferredReleaser(p); });
	}

	MeshPipelineHandle Device::CreateMeshPipeline(wstring name, const MeshDesc& desc) const
	{
		return shared_ptr<MeshPipeline>(new MeshPipeline(*this, name, desc),
			[this](MeshPipeline* p) { DeferredReleaser(p); });
	}

	RaytracingHandle Device::CreateRaytracing(wstring name, const StateObjectHandle& stateObject, UINT width, UINT height, UINT depth) const
	{
		return shared_ptr<Raytracing>(new Raytracing(*this, name, stateObject, width, height, depth),
			[this](Raytracing* p) { DeferredReleaser(p); });
	}

	RootSignatureHandle Device::CreateRootSignature(wstring name, D3D12_ROOT_SIGNATURE_FLAGS flag, initializer_list<RootParameter> rootParams) const
	{
		return shared_ptr<RootSignature>(new RootSignature(*this, name, flag, rootParams),
			[this](RootSignature* p) { DeferredReleaser(p); });
	}

	StateObjectHandle Device::CreateStateObject(wstring name, const StateObjectDesc soDesc) const
	{
		return shared_ptr<StateObject>(new StateObject(*this, name, soDesc),
			[this](StateObject* p) { DeferredReleaser(p); });
	}

	SwapChainHandle Device::CreateSwapChain(wstring name, const HWND& hwnd, SIZE winSize, UINT bufferCount) const
	{
		return shared_ptr<SwapChain>(new SwapChain(*this, name, hwnd, winSize, bufferCount),
			[this](SwapChain* p) { DeferredReleaser(p); });
	}

	TextureHandle Device::CreateTexture(wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, UINT mipLevels) const
	{
		return shared_ptr<Texture>(new Texture(*this, name, texDim, type, strideSize, format, width, height, depth, mipLevels),
			[this](Texture* p) { DeferredReleaser(p); });
	}

	TLASHandle Device::CreateTLAS(wstring name, const CommandHandle& command, const vector<TLASDesc>& tlasDescs) const
	{
		return shared_ptr<TLAS>(new TLAS(*this, name, command, tlasDescs),
			[this](TLAS* p) { DeferredReleaser(p); });
	}

	WorkGraphHandle Device::CreateWorkGraph(wstring name, const StateObjectHandle& stateObject, UINT maxInputRecords, UINT maxNodeInputs) const
	{
		return shared_ptr<WorkGraph>(new WorkGraph(*this, name, stateObject, maxInputRecords, maxNodeInputs),
			[this](WorkGraph* p) { DeferredReleaser(p); });
	}

	void Device::ReleaseCompleted() const
	{
		deferredRelease_->ReleaseCompleted(
			graphicsFence_->GetFence()->GetCompletedValue(),
			computeFence_->GetFence()->GetCompletedValue()
		);
	}

	void Device::WaitOnCPU(const SyncPoint& syncPoint) const
	{
		GetFence(syncPoint.queueType)->WaitSignal(syncPoint.fenceVal);
		ReleaseCompleted();
	}

	const ComPtr<IDXGIFactory7>& Device::GetDXGIFactory() const
	{
		return dxgiFactory_;
	}

	const ComPtr<ID3D12Device>& Device::GetDevice() const
	{
		return device_;
	}

	const ComPtr<StableDevice>& Device::GetStableDevice() const
	{
		return stableDevice_;
	}

	const ComPtr<LatestDevice>& Device::GetLatestDevice() const
	{
		return latestDevice_;
	}

	const ComPtr<ID3D12CommandQueue>& Device::GetCommandQueue(QueueType queueType) const
	{
		if (queueType == QueueType::Graphics) {
			return graphicsCommandQueue_;
		}
		else if (queueType == QueueType::Compute) {
			return computeCommandQueue_;
		}
		else {
			throw runtime_error("[Device::GetCommandQueue] invalid QueueType");
		}
	}

	FenceHandle Device::GetFence(QueueType queueType) const
	{
		if (queueType == QueueType::Graphics) {
			return graphicsFence_;
		}
		else if (queueType == QueueType::Compute) {
			return computeFence_;
		}
		else {
			throw runtime_error("[Device::GetFence] invalid QueueType");
		}
	}
}