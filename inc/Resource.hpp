#pragma once

#include "pch.hpp"

namespace sqrp
{
	class DescriptorManager;
	class Device;

	enum class ResourceType
	{
		Buffer, Texture, AS
	};

	class Resource
	{
	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		const Device& device_;
		ComPtr<ID3D12Resource> resource_ = nullptr;
		std::wstring name_;
		ResourceType rscType_ = ResourceType::Buffer;
		D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_FLAGS rscFlag_ = D3D12_RESOURCE_FLAG_NONE;
		D3D12_RESOURCE_STATES initialState_ = D3D12_RESOURCE_STATE_COMMON;

		// 基底としてだけ使う（Buffer / Texture / AS から呼ぶ）
		Resource(const Device& device, ResourceType rscType, std::wstring name = L"");

	public:
		virtual ~Resource() = default;
		Resource(const Resource&) = delete;
		Resource& operator=(const Resource&) = delete;

		virtual D3D12_CONSTANT_BUFFER_VIEW_DESC MakeCBV() const;
		virtual D3D12_SHADER_RESOURCE_VIEW_DESC MakeSRV() const;
		virtual D3D12_UNORDERED_ACCESS_VIEW_DESC MakeUAV(bool isCounter, UINT mipSlice) const;

		const ComPtr<ID3D12Resource>& GetResource() const;
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
		ResourceType GetResourceType() const;
		D3D12_HEAP_TYPE GetHeapType() const;
		D3D12_RESOURCE_FLAGS GetResourceFlag() const;
		D3D12_RESOURCE_STATES GetInitialState() const;

		//void SetResourceState(D3D12_RESOURCE_STATES rscState);
	};

	enum class BufferType
	{
		Default, Upload, Read, Unordered, AS, Counter
	};

	class Buffer : public Resource
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		BufferType type_ = BufferType::Default;
		UINT strideSize_ = 0;
		UINT numElement_ = 0;
		UINT64 offsetCounter_ = 0;

		void CreateBuffer();
		void CreateCounterBuffer();

	public:
		static UINT64 AlignForUAVCounter(UINT64 size);
		static UINT AlignForConstantBuffer(UINT size);

		Buffer(const Device& device, std::wstring name, BufferType type, UINT strideSize, UINT numElement, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE);

		void* Map(UINT subresource = 0, D3D12_RANGE* range = nullptr);
		void Unmap();
		void Reset();
		void Upload(void* pData, UINT strideSize, UINT numElement);
		template<typename T>
		void Write(const T& data)
		{
			if (sizeof(T) > static_cast<std::size_t>(GetStrideSize()) * GetNumElement()) {
				throw std::runtime_error("[Buffer::Write] the size of src is larger than the buffer size");
			}
			void* rawPtr = Map();
			if (!rawPtr) {
				throw std::runtime_error("[Buffer::Write] Map failed");
			}
			*static_cast<T*>(rawPtr) = data;

			Unmap();
		}
		template<typename T>
		void Write(const std::vector<T>& src)
		{
			if (src.size() * sizeof(T) > static_cast<std::size_t>(GetStrideSize()) * GetNumElement()) {
				throw std::runtime_error("[Buffer::Write] the size of src is larger than the buffer size");
			}
			void* rawPtr = Map();
			if (!rawPtr) {
				throw std::runtime_error("[Buffer::Write] Map failed");
			}
			memcpy(rawPtr, src.data(), sizeof(T) * src.size());

			Unmap();
		}
		void Write(const void* src, UINT size);
		template<typename T, std::size_t N>
		void Read(std::array<T, N>& dest)
		{
			if (sizeof(T) * N > static_cast<std::size_t>(GetStrideSize()) * GetNumElement()) {
				throw std::runtime_error("[Buffer::Read] the size of dest is larger than the buffer size");
			}
			void* rawPtr = Map();
			if (!rawPtr) {
				throw std::runtime_error("[Buffer::Read] Map failed");
			}
			std::memcpy(dest.data(), rawPtr, dest.size() * sizeof(T));

			Unmap();
		}
		template<typename T>
		void Read(std::vector<T>& dest)
		{
			if (dest.size() * sizeof(T) > static_cast<std::size_t>(GetStrideSize()) * GetNumElement()) {
				throw std::runtime_error("[Buffer::Read] the size of dest is larger than the buffer size");
			}
			void* rawPtr = Map();
			if (!rawPtr) {
				throw std::runtime_error("[Buffer::Read] Map failed");
			}
			std::memcpy(dest.data(), rawPtr, dest.size() * sizeof(T));

			Unmap();
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC MakeCBV() const override;
		D3D12_SHADER_RESOURCE_VIEW_DESC MakeSRV() const override;
		D3D12_UNORDERED_ACCESS_VIEW_DESC MakeUAV(bool isCounter, UINT mipSlice) const override;

		UINT GetStrideSize() const;
		UINT GetNumElement() const;
		UINT64 GetOffsetCounter() const;
	};

	enum class TextureType
	{
		Default, Upload, Read, Unordered
	};

	enum class TextureDim
	{
		Tex1D, Tex2D, Tex3D, TexCube,
	};

	class Texture : public Resource
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		TextureType type_ = TextureType::Default;
		TextureDim texDim_ = TextureDim::Tex2D;
		UINT strideSize_ = 0;
		UINT width_ = 1;
		UINT height_ = 1;
		UINT depth_ = 1;
		UINT mipLevels_ = 1;
		DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;

	public:
		Texture(const Device& device, std::wstring name, TextureDim texDim, TextureType type, UINT strideSize, DXGI_FORMAT format, UINT width, UINT height, UINT depth, UINT mipLevels = 1);
		// 既にあるリソース（スワップチェーンのバックバッファなど）を包む。幅・高さ・形式などは resource の desc から読む
		Texture(const Device& device, std::wstring name, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES initialState);
		void* Map();
		void Unmap();
		void Reset();

		D3D12_SHADER_RESOURCE_VIEW_DESC MakeSRV() const override;
		D3D12_UNORDERED_ACCESS_VIEW_DESC MakeUAV(bool isCounter, UINT mipSlice) const override;

		UINT GetStrideSize() const;
		UINT GetWidth() const;
		UINT GetHeight() const;
		UINT GetDepth() const;
		UINT GetMipLevels() const;

	};

	class AS : public Resource
	{
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		UINT64 size_ = 0;

	public:
		AS(const Device& device, std::wstring name, UINT64 size);

		D3D12_SHADER_RESOURCE_VIEW_DESC MakeSRV() const override;
	};

	class Constants
	{
	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		void* constants_ = nullptr;
		UINT numConstants_ = 0;

	public:
		Constants(void* constants, UINT numConstants);

		void* GetConstants() const;
		UINT GetNumConstants() const;
	};

	struct HDRImage
	{
		std::vector<float> data;
		int width    = 0;
		int height   = 0;
		int channels = 0;
	};

	HDRImage LoadHDR(const std::string& path);
}