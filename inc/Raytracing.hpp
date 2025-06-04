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
	ComPtr<ID3D12StateObjectProperties> soProp_ = nullptr;
	std::shared_ptr<Buffer> sbtBuffer_;

	UINT CreateSBTBuffer(UINT numRecords);


	//bool InitWorkGraphContext(const StateObject& stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0);

public:
	static UINT AlignForSBTRecord(UINT size);
	RayTracing(const Device& device, const StateObject& stateObject, std::wstring name = L"");
	~RayTracing() = default;
	//bool Init(Device* pDevice, const StateObject& stateObject, UINT maxInputRecords = 0, UINT maxNodeInputs = 0);

};