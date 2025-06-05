#include <common.hpp>
#include <comdef.h>

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace {
	void EnableDebugLayer()
	{
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
		debugLayer.Reset();
	}
}

bool Device::EnableFeatures(vector<UUID>& features) const
{
	auto result = D3D12EnableExperimentalFeatures(features.size(), features.data(), nullptr, nullptr);
	if (FAILED(result)) {
		cerr << "Failed to enable features\n";
		_com_error err(result);
		std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
		return false;
	}
	return true;
}

bool Device::CreateDXDevice(wstring gpuVendorName)
{
	UINT flagsDXGI = 0;
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		cerr << "Faild to create dxgi factory" << endl;
		return false;
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
	return isCreatedDevice;
}

bool Device::InitializeStableDevice()
{
	if (FAILED(device_->QueryInterface(IID_PPV_ARGS(stableDevice_.ReleaseAndGetAddressOf())))) {
		cerr << "Failed to get stable device" << endl;
		return false;
	}
	
	return true;
}

bool Device::InitializeLatestDevice()
{
	bool isInitializedLatestDevice = false;
	if (CheckWorkGraphSupport()) {
		if (FAILED(device_->QueryInterface(IID_PPV_ARGS(latestDevice_.ReleaseAndGetAddressOf())))) {
			cerr << "Failed to get latest device" << endl;
			return isInitializedLatestDevice;
		}
		isInitializedLatestDevice = true;
	}
	return isInitializedLatestDevice;
}

bool Device::CheckWorkGraphSupport()
{
	bool hasSupportWorkGraph = false;
	D3D12_FEATURE_DATA_D3D12_OPTIONS21 Options;
	if (FAILED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Options, sizeof(Options)))) {
		cerr << "Failed to check feature support" << endl;
		return false;
	}
	if (Options.WorkGraphsTier == D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED) {
		cerr << "Device does not support for work graph" << endl;
		return hasSupportWorkGraph;
	}
	hasSupportWorkGraph = true;
	if (Options.WorkGraphsTier < D3D12_WORK_GRAPHS_TIER_1_1) {
		cerr << "Device does not support for mesh node of work graph" << endl;
	}
	cout << "Device support for work graph" << endl;
	return hasSupportWorkGraph;
}

bool Device::CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice)
{
	auto result = device_->QueryInterface(debugDevice.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		cerr << "Failed to CreateDebugDevice" << endl;
		return false;
	}
	
	return true;
}

Device::Device()
{

}

bool Device::Init(wstring gpuVenorName)
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
	if (!EnableFeatures(features)) {
		cerr << "Failed to enable features" << endl;
		return false;
	}

	if (!CreateDXDevice(gpuVenorName)) {
		cerr << "Failed to create dx device" << endl;
		return false;
	}

	if (!InitializeStableDevice()) {
		cerr << "Failed to get stable device" << endl;
		return false;
	}

	if (!InitializeLatestDevice()) {
		cerr << "Failed to get latest device" << endl;
		return false;
	}

	return true;
}

bool Device::Init(wstring gpuVenorName, ComPtr<ID3D12DebugDevice>& debugDevice)
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	vector<UUID> features = { D3D12ExperimentalShaderModels, D3D12StateObjectsExperiment };
	if (!EnableFeatures(features)) {
		cerr << "Failed to enable features" << endl;
		return false;
	}

	if (!CreateDXDevice(gpuVenorName)) {
		cerr << "Failed to create dx device" << endl;
		return false;
	}

	if (!InitializeLatestDevice()) {
		cerr << "Failed to get latest device" << endl;
		return false;
	}

#ifdef _DEBUG
	cout << "Try to create DebugDevice" << endl;
	if (!CreateDebugDevice(debugDevice)) {
		cerr << "Failed to CreateDebugDevice" << endl;
		return false;
	}
#endif // DEBUG


	return true;
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

std::shared_ptr<ASMesh> Device::CreateASMesh(std::shared_ptr<Command> command, std::string modelPath) const
{
	return make_shared<ASMesh>(*this, command, modelPath);
}

std::shared_ptr<ASMesh> Device::CreateASMesh(std::shared_ptr<Command> command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const
{
	return make_shared<ASMesh>(*this, command, ASVertices, indices);
}

std::shared_ptr<BLAS> Device::CreateBLAS(std::shared_ptr<Command> command, const ASMesh& mesh, std::wstring name) const
{
	return make_shared<BLAS>(*this, command, mesh, name);
}

std::shared_ptr<Buffer> Device::CreateBuffer(BufferType type, UINT strideSize, UINT numElement, std::wstring name) const
{
	return make_shared<Buffer>(*this, type, strideSize, numElement, name);
}

std::shared_ptr<Command> Device::CreateCommand(D3D12_COMMAND_LIST_TYPE commandType, std::wstring name) const
{
	return make_shared<Command>(*this, commandType, name);
}

std::shared_ptr<ComputePipeline> Device::CreateComputePipeline(const ComputeDesc& desc, std::wstring name) const
{
	return make_shared<ComputePipeline>(*this, desc, name);
}

std::shared_ptr<DescriptorManager> Device::CreateDescriptorManager(HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, std::wstring name) const
{
	return make_shared<DescriptorManager>(*this, heapType, descManagerDesc, name);
}

std::shared_ptr<Fence> Device::CreateFence(std::wstring name) const
{
	return make_shared<Fence>(*this, name);
}

std::shared_ptr<GraphicsPipeline> Device::CreateGraphicsPipeline(const GraphicsDesc& desc, std::wstring name) const
{
	return make_shared<GraphicsPipeline>(*this, desc, name);
}

std::shared_ptr<Indirect> Device::CreateIndirect(std::initializer_list<IndirectDesc> indirectDescs, std::shared_ptr<RootSignature> rootSignature, UINT byteStride, std::wstring name) const
{
	return make_shared<Indirect>(*this, indirectDescs, rootSignature, byteStride, name);
}

std::shared_ptr<Mesh> Device::CreateMesh(std::shared_ptr<Command> command, std::string modelPath) const
{
	return make_shared<Mesh>(*this, command, modelPath);
;}
std::shared_ptr<Mesh> Device::CreateMesh(std::shared_ptr<Command> command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const
{
	return make_shared<Mesh>(*this, command, vertices, indices);
}

std::shared_ptr<RayTracing> Device::CreateRaytracing(const StateObject& stateObject, UINT width, UINT height, UINT depth, std::wstring name) const
{
	return make_shared<RayTracing>(*this, stateObject, width, height, depth, name);
}

std::shared_ptr<RootSignature> Device::CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name) const
{
	return make_shared<RootSignature>(*this, flag, rootParams, name);
}

std::shared_ptr<StateObject> Device::CreateStateObject(const StateObjectDesc soDesc, std::wstring name) const
{
	return make_shared<StateObject>(*this, soDesc, name);
}

std::shared_ptr<SwapChain> Device::CreateSwapChain(std::shared_ptr<Command>& command, const HWND& hwnd, SIZE winSize, std::wstring name) const
{
	return make_shared<SwapChain>(*this, command, hwnd, winSize, name);
}

std::shared_ptr<Texture> Device::CreateTexture(TextureDimention texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, std::wstring name)
{
	return make_shared<Texture>(*this, texDim, type, strideSize, format, width, height, depth, name);
}

std::shared_ptr<TLAS> Device::CreateTLAS(std::shared_ptr<Command> command, const std::vector<TLASDesc>& tlasDescs, std::wstring name) const
{
	return make_shared<TLAS>(*this, command, tlasDescs, name);
}

std::shared_ptr<WorkGraph> Device::CreateWorkGraph(const StateObject& stateObject, UINT maxInputRecords, UINT maxNodeInputs, std::wstring name) const
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