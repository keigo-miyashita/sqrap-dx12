#pragma once

#include <common.hpp>
#include <Command.hpp>

struct CameraMatrix
{
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
};

struct Light
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT4 color;
};

struct Object
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX invTransModel;
};

struct Color
{
	DirectX::XMFLOAT4 color;
};

class SampleScene
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	Device device_;
	DXC dxc_;
	std::shared_ptr<Command> command_;
	std::shared_ptr<SwapChain> swapChain_;
	std::shared_ptr<Mesh> sphere_;

	// Scene Items
	Camera camera_;
	Light light0_;
	Object sphere0_;

	// Resources
	std::shared_ptr<Buffer> cameraBuffer_;
	std::shared_ptr<Buffer> light0Buffer_;
	std::shared_ptr<Buffer> sphere0Buffer_;

	// Shaders
	std::shared_ptr<Shader> simpleVS_;
	std::shared_ptr<Shader> lambertPS_;

	// Descriptor
	std::shared_ptr<DescriptorManager> sphere0DescManager_;

	// RootSignature
	std::shared_ptr<RootSignature> sphere0RootSignature_;

	// Pipeline
	std::shared_ptr<GraphicsPipeline> lambert_;

	void BeginRender();
	void EndRender();

public:
	SampleScene();
	~SampleScene() = default;
	bool Init(const Application& app);
	bool Init(const Application& app, ComPtr<ID3D12DebugDevice>& debugDevice);
	/*virtual int Input(UINT msg, WPARAM wparam, LPARAM lparam) override;*/
	void Render();
};