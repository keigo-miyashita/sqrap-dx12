#include "SampleApplication.hpp"

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;
using namespace sqrp;

SampleApplication::SampleApplication(std::string windowName, unsigned int window_width, unsigned int window_height) : Application(windowName, window_width, window_height)
{

}

bool SampleApplication::OnStart()
{
	device_.Init(L"NVIDIA");

	dxc_.Init();

	GUI_ = device_.CreateGUI(GetWindowHWND());

	command_ = device_.CreateCommand(L"General");

	SIZE size = { GetWindowWidth(), GetWindowHeight() };
	swapChain_ = device_.CreateSwapChain(L"Main", command_, GetWindowHWND(), size);

	// ASMesh
	string modelPath = string(MODEL_DIR) + "Suzanne.gltf";
	suzanneASMesh_ = device_.CreateASMesh(command_, modelPath);
	suzanneMesh_ = device_.CreateMesh(command_, modelPath);

	// Scene Items
	// Camera
	camera_.Init((float)GetWindowWidth() / (float)GetWindowHeight(), XMFLOAT3(0.0f, 0.0f, -5.0f));
	// Light
	light0_.pos = XMFLOAT4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	suzanneBLAS_ = device_.CreateBLAS(L"suzanne", command_, suzanneASMesh_);
	XMMATRIX modelMat = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixIdentity());
	sceneTLAS_ = device_.CreateTLAS(
		L"scene",
		command_,
		{
			{modelMat, 0, suzanneBLAS_, D3D12_RAYTRACING_INSTANCE_FLAG_NONE}
		}
	);

	// Resources
	cameraBuffer_ = device_.CreateBuffer(L"camera", BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(RayTracigCameraMatrix)), 1);
	cameraBuffer_->Write(RayTracigCameraMatrix{ camera_.GetView(), camera_.GetProj(), camera_.GetInvViewProj(), camera_.GetInvView(), XMFLOAT4(0.0f, 0.0f, -5.0f, 1.0f) });

	light0Buffer_ = device_.CreateBuffer(L"light0", BufferType::Upload, Buffer::AlignForConstantBuffer(sizeof(Light)), 1);
	light0Buffer_->Write(light0_);

	outputTexture_ = device_.CreateTexture(
		L"output",
		TextureDim::Tex2D,
		TextureType::Unordered,
		0, DXGI_FORMAT_R8G8B8A8_UNORM, GetWindowWidth(), GetWindowHeight(), 1
	);

	diffuseColor_ = { {1.0f, 0.0f, 0.0f, 1.0f} };

	// Shaders
	wstring shaderPath = wstring(SHADER_DIR) + L"raytracing.hlsl";
	rayGen_ = dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"rayGeneration");
	closestHit_ = dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"closestHit");
	miss_ = dxc_.CreateShader(ShaderType::RayTracing, shaderPath, L"miss");

	// Descriptor Manager
	suzanneDescManager_ = device_.CreateDescriptorManager(
		L"suzanne",
		HeapType::Resource,
		{
			{ cameraBuffer_,					ViewType::CBV, 0},
			{ light0Buffer_,					ViewType::CBV, 1},
			{ sceneTLAS_->GetASBuffer(),		ViewType::SRV, 0},
			{ suzanneMesh_->GetVertexBuffer(),	ViewType::SRV, 1},
			{ suzanneMesh_->GetIndexBuffer(),	ViewType::SRV, 2},
			// NOTE : Need output UAV
			{ outputTexture_,					ViewType::UAV, 0}
		}
	);
	// RootSignature
	suzanneRootSignature_ = device_.CreateRootSignature(
		L"suzanne",
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
		{
			{RootParamType::DescTable,	suzanneDescManager_},
			{RootParamType::Constant,	DirectRootParamDesc{2, 4, (void*)(&diffuseColor_)}}
		}
	);

	//ColorConstants_ = std::make_shared<Constants>(static_cast<void*>(&diffuseColor_), 4);

	// NOTE : ResourceSet create example
	// This samples does not use local rootsignature, so it is not necessaliry
	suzanneResourceSet_ = std::make_shared<ResourceSet>(
		suzanneRootSignature_,
		std::vector<ResourceSetDesc>{
			{suzanneDescManager_},
			{ ColorConstants_ }
	}
	);

	// StateObject
	rayTracingStates_ = device_.CreateStateObject(
		L"rayTracing",
		{
			StateObjectType::Raytracing,
			StateObjectDesc::RayTracingDesc
			{
				suzanneRootSignature_,
				{
					{rayGen_, ShaderStage::RayGen},
				},
				{
					{miss_, ShaderStage::Miss},
				},
				{
					{
						L"HitGroup",
						{ closestHit_, ShaderStage::ClosestHit },
					}
				},
				{3 * sizeof(float), 2 * sizeof(float), 1}
			}
		}
	);

	rayTracing_ = device_.CreateRaytracing(
		L"rayTracing",
		rayTracingStates_,
		GetWindowWidth(),
		GetWindowHeight(),
		1
	);

	return true;
};

void SampleApplication::OnUpdate()
{
	command_->BeginRender(swapChain_);

	camera_.Update();
	cameraBuffer_->Write(RayTracigCameraMatrix{ camera_.GetView(), camera_.GetProj(), camera_.GetInvViewProj(), camera_.GetInvView(), camera_.GetPos() });	

	command_->SetRayTracingResource(suzanneRootSignature_);
	command_->SetRayTracingState(rayTracingStates_);
	command_->DispatchRays(rayTracing_);

	command_->CopyBuffer(outputTexture_, (swapChain_->GetCurrentBackBuffer()));

	// GUI
	{
		GUI_->BeginCommand();
		// Add GUI 
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::ColorEdit4("##SurfaceColor", (float*)&diffuseColor_);
		GUI_->EndCommand();
		command_->DrawGUI(GUI_);
	}

	command_->EndRender(swapChain_);
}

void SampleApplication::OnTerminate()
{
};