#include <common.hpp>

#include "SampleScene.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

void SampleScene::BeginRender()
{
	auto bbIdx = swapChain_.GetSwapChain()->GetCurrentBackBufferIndex();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_.GetCurrentBackBuffer(bbIdx).Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	command_.GetCommandList()->ResourceBarrier(1, &barrier);

	auto rtvHandle = swapChain_.GetRtvHeap()->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIdx * device_.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto dsvHandle = swapChain_.GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
	command_.GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	// Clear depth buffer
	command_.GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Clear display
	float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	command_.GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Set viewport and scissors
	auto viewPort = swapChain_.GetViewPort();
	auto scissorRect = swapChain_.GetRect();
	command_.GetCommandList()->RSSetViewports(1, &viewPort);
	command_.GetCommandList()->RSSetScissorRects(1, &scissorRect);

	//cout << "Begin Render" << endl;
}

void SampleScene::EndRender()
{
	auto bbIdx = swapChain_.GetSwapChain()->GetCurrentBackBufferIndex();

	// Transit render target to present
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_.GetCurrentBackBuffer(bbIdx).Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	command_.GetCommandList()->ResourceBarrier(1, &barrier);

	fence_.WaitCommand(command_);

	//cout << "End Render" << endl;
}

void SampleScene::Render()
{
	BeginRender();

	command_.GetCommandList()->SetPipelineState(lambert_.GetPipelineState().Get());
	command_.GetCommandList()->SetGraphicsRootSignature(sphere0RootSignature_.GetRootSignature().Get());
	command_.GetCommandList()->SetDescriptorHeaps(1, sphere0DescManager_.GetDescriptorHeap().GetAddressOf());
	command_.GetCommandList()->SetGraphicsRootDescriptorTable(0, sphere0DescManager_.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	Color sphere0Color = {XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)};
	command_.GetCommandList()->SetGraphicsRoot32BitConstants(1, 4, reinterpret_cast<void *>(&sphere0Color), 0);
	command_.AddDrawIndexed(sphere_, 1);

	EndRender();

	swapChain_.GetSwapChain()->Present(1, 0);
}

SampleScene::SampleScene()
{
	
}

bool SampleScene::Init(const Application& app)
{
	if (!device_.Init(L"NVIDIA")) {
		cerr << "Failed to init Device" << endl;
		return false;
	}

	if (!dxc_.Init()) {
		return false;
	}

	if (!command_.Init(&device_)) {
		cerr << "Failed to init commandmanager" << endl;
		return false;
	}

	SIZE size = { app.GetWindowWidth(), app.GetWindowHeight()};
	if (!swapChain_.Init(device_, app.GetWindowHWND(), size, command_)) {
		cerr << "Failed to init swapchain" << endl;
		return false;
	}

	if (!fence_.Init(&device_)) {
		return false;
		cerr << "Failed to init fence" << endl;
	}

	// Objects data
	string modelPath = string(modelPath) + "\\sphere.gltf";
	if (!sphere_.Init(&device_, modelPath)) {
		cerr << "Failed to init sphere" << endl;
		return false;
	}

	// Scene Items
	// Camera
	XMFLOAT3 cameraPos = XMFLOAT3(0.0f, 0.0f, -5.0f);
	XMFLOAT3 targetPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 upDir = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&cameraPos), XMLoadFloat3(&targetPos), XMLoadFloat3(&upDir));

	float aspectRatio = (float)app.GetWindowWidth() / (float)app.GetWindowHeight();
	float fovYAngle = XMConvertToRadians(60.0f);
	float nearZ = 0.1f;
	float farZ = 100.0f;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(fovYAngle, aspectRatio, nearZ, farZ);
	camera_.view = view;
	camera_.proj = proj;
	
	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	XMFLOAT4 sphere0Pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	sphere0_.model = XMMatrixTranslation(sphere0Pos.x, sphere0Pos.y, sphere0Pos.z);
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));
	
	// Resources
	cameraBuffer_.InitAsUpload(&device_,  Buffer::AlignForConstantBuffer(sizeof(Camera)), 1);
	void* rawPtr = cameraBuffer_.Map();
	if (rawPtr) {
		Camera* pCamera = static_cast<Camera*>(rawPtr);
		*pCamera = camera_;
		cameraBuffer_.Unmap();
	}

	light0Buffer_.InitAsUpload(&device_, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	rawPtr = light0Buffer_.Map();
	if (rawPtr) {
		Light* pLight = static_cast<Light*>(rawPtr);
		*pLight = light0_;
		light0Buffer_.Unmap();
	}

	sphere0Buffer_.InitAsUpload(&device_, Buffer::AlignForConstantBuffer(sizeof(Object)), 1);
	rawPtr = sphere0Buffer_.Map();
	if (rawPtr) {
		Object* pObject = static_cast<Object*>(rawPtr);
		*pObject = sphere0_;
		sphere0Buffer_.Unmap();
	}

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"\\lambert.hlsl";
	if (!simpleVS_.Init(dxc_, shaderPath, ShaderType::Vertex, L"vs_6_6", L"VSmain")) {
		cerr << "Failed to create simpleVS" << endl;
		return false;
	}
	
	if (!lambertPS_.Init(dxc_, shaderPath, ShaderType::Pixel, L"ps_6_6", L"PSmain")) {
		cerr << "Failed to create lambertPS_" << endl;
		return false;
	}

	//// Descriptor Heap
	//sphere0DescHeap_.Init(&device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3);
	//sphere0DescHeap_.CreateCBV(cameraBuffer_, 0);
	//sphere0DescHeap_.CreateCBV(light0Buffer_, 1);
	//sphere0DescHeap_.CreateCBV(sphere0Buffer_, 2);
	//// Descriptor Table
	//sphere0DescTable_.InitAsBuffer(0, 3, 0, 0, 0, 0);
	// Descriptor Manager
	sphere0DescManager_.InitAsBuffer(&device_, 0, 3, 0, 0, 0, 0);
	sphere0DescManager_.CreateCBV(cameraBuffer_, 0);
	sphere0DescManager_.CreateCBV(light0Buffer_, 1);
	sphere0DescManager_.CreateCBV(sphere0Buffer_, 2);
	// RootSignature
	sphere0RootSignature_.Init(&device_);
	sphere0RootSignature_.AddDescriptorTable(sphere0DescManager_, D3D12_SHADER_VISIBILITY_ALL);
	sphere0RootSignature_.AddConstant(3, 4);
	sphere0RootSignature_.InitializeRootSignature(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Graphics pipeline
	vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	GraphicsDesc lambertDesc(inputLayouts);
	lambertDesc.rootSignature_ = &sphere0RootSignature_;
	lambertDesc.VS_ = &simpleVS_;
	lambertDesc.PS_ = &lambertPS_;
	/*lambertDesc.blendState_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lambertDesc.sampleMask_ = D3D12_DEFAULT_SAMPLE_MASK;
	lambertDesc.rasterizerDesc_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lambertDesc.depthStencilDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lambertDesc.dsvFormat_ = DXGI_FORMAT_D32_FLOAT;
	lambertDesc.IBStripCutValue_ = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	lambertDesc.primitiveType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	vector<DXGI_FORMAT> RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM};
	lambertDesc.RTVFormats_ = RTVFormats;
	lambertDesc.sampleDesc_.Count = 1;
	lambertDesc.sampleDesc_.Quality = 0;*/
	if (!lambert_.Init(&device_, lambertDesc)) {
		cerr << "Failed to create lambert pipeline state" << endl;
		return false;
	}

	
	return true;
}

bool SampleScene::Init(const Application& app, ComPtr<ID3D12DebugDevice>& debugDevice)
{
	if (!device_.Init(L"NVIDIA", debugDevice)) {
		cerr << "Failed to init Device" << endl;
		return false;
	}

	if (!dxc_.Init()) {
		return false;
	}

	if (!command_.Init(&device_)) {
		cerr << "Failed to init commandmanager" << endl;
		return false;
	}

	SIZE size = { app.GetWindowWidth(), app.GetWindowHeight() };
	if (!swapChain_.Init(device_, app.GetWindowHWND(), size, command_)) {
		cerr << "Failed to init swapchain" << endl;
		return false;
	}

	if (!fence_.Init(&device_)) {
		return false;
		cerr << "Failed to init fence" << endl;
	}

	// Objects data
	string modelPath = string(MODEL_DIR) + "\\sphere.gltf";
	if (!sphere_.Init(&device_, modelPath)) {
		cerr << "Failed to init sphere" << endl;
		return false;
	}

	// Scene Items
	// Camera
	XMFLOAT3 cameraPos = XMFLOAT3(0.0f, 0.0f, -5.0f);
	XMFLOAT3 targetPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 upDir = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&cameraPos), XMLoadFloat3(&targetPos), XMLoadFloat3(&upDir));

	float aspectRatio = (float)app.GetWindowWidth() / (float)app.GetWindowHeight();
	float fovYAngle = XMConvertToRadians(60.0f);
	float nearZ = 0.1f;
	float farZ = 100.0f;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(fovYAngle, aspectRatio, nearZ, farZ);
	camera_.view = view;
	camera_.proj = proj;

	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	XMFLOAT4 sphere0Pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	sphere0_.model = XMMatrixTranslation(sphere0Pos.x, sphere0Pos.y, sphere0Pos.z);
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));

	// Resources
	cameraBuffer_.InitAsUpload(&device_, Buffer::AlignForConstantBuffer(sizeof(Camera)), 1);
	void* rawPtr = cameraBuffer_.Map();
	if (rawPtr) {
		Camera* pCamera = static_cast<Camera*>(rawPtr);
		*pCamera = camera_;
		cameraBuffer_.Unmap();
	}

	light0Buffer_.InitAsUpload(&device_, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	rawPtr = light0Buffer_.Map();
	if (rawPtr) {
		Light* pLight = static_cast<Light*>(rawPtr);
		*pLight = light0_;
		light0Buffer_.Unmap();
	}

	sphere0Buffer_.InitAsUpload(&device_, Buffer::AlignForConstantBuffer(sizeof(Object)), 1);
	rawPtr = sphere0Buffer_.Map();
	if (rawPtr) {
		Object* pObject = static_cast<Object*>(rawPtr);
		*pObject = sphere0_;
		sphere0Buffer_.Unmap();
	}

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"\\lambert.hlsl";
	if (!simpleVS_.Init(dxc_, shaderPath, ShaderType::Vertex, L"vs_6_6", L"VSmain")) {
		cerr << "Failed to create simpleVS" << endl;
		return false;
	}

	if (!lambertPS_.Init(dxc_, shaderPath, ShaderType::Pixel, L"ps_6_6", L"PSmain")) {
		cerr << "Failed to create lambertPS_" << endl;
		return false;
	}

	//// Descriptor Heap
	//sphere0DescHeap_.Init(&device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3);
	//sphere0DescHeap_.CreateCBV(cameraBuffer_, 0);
	//sphere0DescHeap_.CreateCBV(light0Buffer_, 1);
	//sphere0DescHeap_.CreateCBV(sphere0Buffer_, 2);
	//// Descriptor Table
	//sphere0DescTable_.InitAsBuffer(0, 3, 0, 0, 0, 0);
	// Descriptor Manager
	sphere0DescManager_.InitAsBuffer(&device_, 0, 3, 0, 0, 0, 0);
	sphere0DescManager_.CreateCBV(cameraBuffer_, 0);
	sphere0DescManager_.CreateCBV(light0Buffer_, 1);
	sphere0DescManager_.CreateCBV(sphere0Buffer_, 2);
	// RootSignature
	sphere0RootSignature_.Init(&device_);
	sphere0RootSignature_.AddDescriptorTable(sphere0DescManager_, D3D12_SHADER_VISIBILITY_ALL);
	sphere0RootSignature_.AddConstant(3, 4);
	sphere0RootSignature_.InitializeRootSignature(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Graphics pipeline
	vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	GraphicsDesc lambertDesc(inputLayouts);
	lambertDesc.rootSignature_ = &sphere0RootSignature_;
	lambertDesc.VS_ = &simpleVS_;
	lambertDesc.PS_ = &lambertPS_;
	/*lambertDesc.blendState_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lambertDesc.sampleMask_ = D3D12_DEFAULT_SAMPLE_MASK;
	lambertDesc.rasterizerDesc_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lambertDesc.depthStencilDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lambertDesc.dsvFormat_ = DXGI_FORMAT_D32_FLOAT;
	lambertDesc.IBStripCutValue_ = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	lambertDesc.primitiveType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	vector<DXGI_FORMAT> RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM};
	lambertDesc.RTVFormats_ = RTVFormats;
	lambertDesc.sampleDesc_.Count = 1;
	lambertDesc.sampleDesc_.Quality = 0;*/
	if (!lambert_.Init(&device_, lambertDesc)) {
		cerr << "Failed to create lambert pipeline state" << endl;
		return false;
	}


	return true;
}