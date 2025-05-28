#pragma once

#include <common.hpp>

#define StableDevice ID3D12Device13
#define LatestDevice ID3D12Device14

class ASMesh;
class Buffer;
class BLAS;
class Command;
class ComputePipeline;
class DescriptorManager;
class GraphicsPipeline;
class Mesh;
class RootSignature;
class SwapChain;
class TLAS;

struct ASMesh;
struct ASVertex;
struct ComputeDesc;
struct GraphicsDesc;
struct IndirectDesc;
struct Mesh;
struct RootParameter;
struct Vertex;

class Device
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	D3D_FEATURE_LEVEL featureLevel_ = D3D_FEATURE_LEVEL_12_1;
	ComPtr<ID3D12Device> device_ = nullptr;
	ComPtr<IDXGIAdapter4> adapter_ = nullptr;
	ComPtr<StableDevice> stableDevice_ = nullptr;
	ComPtr<LatestDevice> latestDevice_ = nullptr;

	bool EnableFeatures(std::vector<UUID>& features) const;
	bool CreateDXDevice(std::wstring gpuVendorName);
	bool InitializeStableDevice();
	bool InitializeLatestDevice();
	bool CheckWorkGraphSupport();
	bool CreateDebugDevice(ComPtr<ID3D12DebugDevice>& debugDevice);

public:
	Device();
	~Device() = default;
	bool Init(std::wstring gpuVenorName);
	bool Init(std::wstring gpuVenorName, ComPtr<ID3D12DebugDevice>& debugDevice);
	void ShowUsedVramSize();
	std::shared_ptr<ASMesh> CreateASMesh(std::shared_ptr<Command> command, std::string modelPath) const;
	std::shared_ptr<ASMesh> CreateASMesh(std::shared_ptr<Command> command, const std::vector<ASVertex>& ASVertices, const std::vector<uint32_t>& indices) const;
	std::shared_ptr<BLAS> CreateBLAS(std::shared_ptr<Command> command, const ASMesh& mesh, std::wstring name = L"") const;
	std::shared_ptr<Buffer> CreateBuffer(BufferType type, UINT strideSize, UINT numElement, std::wstring name = L"") const;
	std::shared_ptr<Command> CreateCommand(D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring name = L"") const;
	std::shared_ptr<ComputePipeline> CreateComputePipeline(const ComputeDesc& desc, std::wstring name = L"") const;
	std::shared_ptr<DescriptorManager> CreateDescriptorManager(HeapType heapType, std::initializer_list<DescriptorManagerDesc> descManagerDesc, std::wstring name = L"") const;
	std::shared_ptr<Fence> CreateFence(std::wstring name = L"") const;
	std::shared_ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsDesc& desc, std::wstring name = L"") const;
	std::shared_ptr<Indirect> CreateIndirect(std::initializer_list<IndirectDesc> indirectDescs, std::shared_ptr<RootSignature> rootSignature, UINT byteStride, std::wstring name = L"") const;
	std::shared_ptr<Mesh> CreateMesh(std::shared_ptr<Command> command, std::string modelPath) const;
	std::shared_ptr<Mesh> CreateMesh(std::shared_ptr<Command> command, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) const;
	std::shared_ptr<RootSignature> CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flag, std::initializer_list<RootParameter> rootParams, std::wstring name = L"") const;
	std::shared_ptr<SwapChain> CreateSwapChain(std::shared_ptr<Command>& command, const HWND& hwnd, SIZE winSize, std::wstring name = L"") const;
	std::shared_ptr<TLAS> CreateTLAS(std::shared_ptr<Command> command, const std::vector<TLASDesc>& tlasDescs, std::wstring name = L"") const;
	ComPtr<IDXGIFactory7> GetDXGIFactory() const;
	ComPtr<ID3D12Device> GetDevice() const;
	ComPtr<StableDevice> GetStableDevice() const;
	ComPtr<LatestDevice> GetLatestDevice() const;
};