struct MeshNodeRecord
{
    uint3 DGid : SV_DispatchGrid;
};

struct Vertex
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

struct MeshToPS
{
    float4 svposition : SV_POSITION;
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

cbuffer Camera : register(b0)
{
    float4x4 view;
    float4x4 proj;
}

cbuffer Light : register(b1)
{
    float4 lightPos;
    float4 lightColor;
}

cbuffer Object : register(b2)
{
    float4x4 model;
    float4x4 invTransModel;
}

cbuffer Color : register(b3)
{
    float4 color;
}

StructuredBuffer<Vertex> VertexBuffer_ : register(t0);
StructuredBuffer<uint> IndexBuffer_ : register(t1);

[Shader("mesh")]
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void MSmain(
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    out vertices MeshToPS verts[192],
    out indices uint3 idx[64]
)
{
    uint id = DTid.x;
    
    //if (WaveIsFirstLane() == 0)
    SetMeshOutputCounts(192, 64);
    
    idx[GTid.x] = uint3(0 + 3 * GTid.x, 1 + 3 * GTid.x, 2 + 3 * GTid.x);
    for (int i = 0; i < 3; i++)
    {
        verts[i + 3 * GTid.x].svposition = mul(proj, mul(view, mul(model, VertexBuffer_[IndexBuffer_[i + 3 * id]].position)));
        verts[i + 3 * GTid.x].position = mul(model, VertexBuffer_[IndexBuffer_[i + 3 * id]].position);
        verts[i + 3 * GTid.x].normal = normalize(mul(invTransModel, VertexBuffer_[IndexBuffer_[i + 3 * id]].normal));
        verts[i + 3 * GTid.x].tangent = normalize(mul(invTransModel, VertexBuffer_[IndexBuffer_[i + 3 * id]].tangent));
        verts[i + 3 * GTid.x].uv = VertexBuffer_[IndexBuffer_[i + 3 * id]].uv;
    }
}

// Pixel shader
float4 PSmain(MeshToPS meshToPS) : SV_TARGET
{
    float4 lightDir = normalize(lightPos - meshToPS.position);
    //return vsToPS.normal;
    return color * lightColor * max(0.0f, dot(lightDir, meshToPS.normal));
}