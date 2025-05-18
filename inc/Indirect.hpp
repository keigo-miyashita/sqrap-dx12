#pragma once

#include <common.hpp>

class Device;
class RootSignature;

class Indirect
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Device* pDevice_ = nullptr;
	ComPtr<ID3D12CommandSignature> cmdSig_ = nullptr;
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgDesc_;
	D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc_ = {};

public:
	Indirect();
	~Indirect() = default;
	bool Init(Device* pDevice);
	bool InitializeCommandSignature(const RootSignature& rootSignature_, UINT byteStride_);
	void AddCBV(UINT rootParameterIndex);
	void AddSRV(UINT rootParameterIndex);
	void AddUAV(UINT rootParameterIndex);
	// 頂点バッファを命令ごとに切り替える場合指定
	void AddVertexBufferView(UINT slot);
	// インデックスバッファを命令ごとに切り替える場合指定
	void AddIndexBufferView();
	void AddDrawIndexed();
	void AddDispatch();
	ComPtr<ID3D12CommandSignature> GetCommandSignature() const;

};