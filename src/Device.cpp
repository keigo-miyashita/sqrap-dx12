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

ComPtr<IDXGIFactory7> Device::GetDXGIFactory() const
{
	return dxgiFactory_;
}

ComPtr<ID3D12Device> Device::GetDevice() const
{
	return device_;
}

ComPtr<LatestDevice> Device::GetLatestDevice() const
{
	return latestDevice_;
}