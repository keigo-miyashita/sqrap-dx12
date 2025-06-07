#pragma once

#include "pch.hpp"

#include "Alias.hpp"

class Buffer;
class Device;
class Shader;
class StateObject;

struct Constants;

class RayTracing
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const Device* pDevice_ = nullptr;
	std::wstring name_;
	ComPtr<ID3D12StateObjectProperties> soProp_ = nullptr;
	BufferHandle sbtBuffer_;

	D3D12_DISPATCH_RAYS_DESC raysDesc_ = {};

	UINT CreateSBTBuffer(UINT numRecords);

public:
	static UINT AlignForSBTRecord(UINT size);
	static UINT AlignForSBT(UINT size);
	static UINT CopyMem(void* dest, const void* data, UINT size);
	RayTracing(const Device& device, const StateObject& stateObject, UINT width, UINT height, UINT depth, std::wstring name = L"");
	~RayTracing() = default;

	D3D12_DISPATCH_RAYS_DESC GetDispatchRayDesc() const;
};