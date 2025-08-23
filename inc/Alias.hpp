#pragma once

namespace sqrp
{
	class AS;
	class ASMesh;
	class BLAS;
	class Buffer;
	class Command;
	class ComputePipeline;
	class Constants;
	class DescriptorManager;
	class Fence;
	class GraphicsPipeline;
	class GUI;
	class Indirect;
	class Mesh;
	class MeshPipeline;
	class RayTracing;
	class Resource;
	class ResourceSet;
	class RootSignature;
	class Shader;
	class StateObject;
	class SwapChain;
	class Texture;
	class TLAS;
	class WorkGraph;

	using ASHandle = std::shared_ptr<AS>;
	using ASMeshHandle = std::shared_ptr<ASMesh>;
	using BLASHandle = std::shared_ptr<BLAS>;
	using BufferHandle = std::shared_ptr<Buffer>;
	using CommandHandle = std::shared_ptr<Command>;
	using ComputePipelineHandle = std::shared_ptr<ComputePipeline>;
	using ConstantsHandle = std::shared_ptr<Constants>;
	using DescriptorManagerHandle = std::shared_ptr<DescriptorManager>;
	using FenceHandle = std::shared_ptr<Fence>;
	using GraphicsPipelineHandle = std::shared_ptr<GraphicsPipeline>;
	using GUIHandle = std::shared_ptr<GUI>;
	using IndirectHandle = std::shared_ptr<Indirect>;
	using MeshHandle = std::shared_ptr<Mesh>;
	using MeshPipelineHandle = std::shared_ptr<MeshPipeline>;
	using RayTracingHandle = std::shared_ptr<RayTracing>;
	using ResourceHandle = std::shared_ptr<Resource>;
	using ResourceSetHandle = std::shared_ptr<ResourceSet>;
	using RootSignatureHandle = std::shared_ptr<RootSignature>;
	using ShaderHandle = std::shared_ptr<Shader>;
	using StateObjectHandle = std::shared_ptr<StateObject>;
	using SwapChainHandle = std::shared_ptr<SwapChain>;
	using TextureHandle = std::shared_ptr<Texture>;
	using TLASHandle = std::shared_ptr<TLAS>;
	using WorkGraphHandle = std::shared_ptr<WorkGraph>;
}