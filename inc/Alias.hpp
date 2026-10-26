#pragma once

#include "pch.hpp"

namespace sqrp
{
	class AS;
	class BLAS;
	class Buffer;
	class Command;
	class ComputePipeline;
	class Constants;
	class DescriptorManager;
	class Fence;
	class GraphicsPipeline;
	class Gui;
	class Indirect;
	class Mesh;
	class MeshPipeline;
	class Pipeline;
	class Raytracing;
	class Resource;
	class RootSignature;
	class Shader;
	class StateObject;
	class SwapChain;
	class Texture;
	class TLAS;
	class WorkGraph;

	using ASHandle = std::shared_ptr<AS>;
	using BLASHandle = std::shared_ptr<BLAS>;
	using BufferHandle = std::shared_ptr<Buffer>;
	using CommandHandle = std::shared_ptr<Command>;
	using ComputePipelineHandle = std::shared_ptr<ComputePipeline>;
	using ConstantsHandle = std::shared_ptr<Constants>;
	using DescriptorManagerHandle = std::shared_ptr<DescriptorManager>;
	using FenceHandle = std::shared_ptr<Fence>;
	using GraphicsPipelineHandle = std::shared_ptr<GraphicsPipeline>;
	using GuiHandle = std::shared_ptr<Gui>;
	using IndirectHandle = std::shared_ptr<Indirect>;
	using MeshHandle = std::shared_ptr<Mesh>;
	using MeshPipelineHandle = std::shared_ptr<MeshPipeline>;
	using PipelineHandle = std::shared_ptr<Pipeline>;
	using RaytracingHandle = std::shared_ptr<Raytracing>;
	using ResourceHandle = std::shared_ptr<Resource>;
	using RootSignatureHandle = std::shared_ptr<RootSignature>;
	using ShaderHandle = std::shared_ptr<Shader>;
	using StateObjectHandle = std::shared_ptr<StateObject>;
	using SwapChainHandle = std::shared_ptr<SwapChain>;
	using TextureHandle = std::shared_ptr<Texture>;
	using TLASHandle = std::shared_ptr<TLAS>;
	using WorkGraphHandle = std::shared_ptr<WorkGraph>;
}