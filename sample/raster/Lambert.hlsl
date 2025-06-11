struct VSInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

struct VStoPS
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

// Vertex shader
VStoPS VSmain(VSInput vsInput)
{
    VStoPS vsToPS;
    vsToPS.svposition = mul(proj, mul(view, mul(model, vsInput.position)));
    vsToPS.position = mul(model, vsInput.position);
    vsToPS.normal = normalize(mul(invTransModel, vsInput.normal));
    vsToPS.tangent = normalize(mul(invTransModel, vsInput.tangent));
    vsToPS.uv = vsInput.uv;

    return vsToPS;
}

// Pixel shader
float4 PSmain(VStoPS vsToPS) : SV_TARGET
{
    float4 lightDir = normalize(lightPos - vsToPS.position);
    //return vsToPS.normal;
    return color * lightColor * max(0.0f, dot(lightDir, vsToPS.normal));
}