#pragma once

#include <common.hpp>

class Buffer;
class Device;
class Shader;

class RayTracing
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::wstring name_;
	ComPtr<IID_ID3D12StateObjectProperties> soProp_ = nullptr;
	ComPtr<ID3D12Resource> rayGenBuffer = nullptr;
	ComPtr<ID3D12Resource> missBuffer = nullptr;
	ComPtr<ID3D12Resource> hitGroupBuffer = nullptr;

	bool CreateSBTBuffer(UINT numRecords):


	//bool InitWorkGraphContext(const StateObject& stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0);

public:
	static UINT AlignForSBT(UINT size);
	RayTracing(const Device& device, const StateObject& stateObject, std::wstring name = L"");
	~RayTracing() = default;
	//bool Init(Device* pDevice, const StateObject& stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0);

};