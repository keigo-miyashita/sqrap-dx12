cbuffer Camera : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 invViewProj;
}

cbuffer Light : register(b1)
{
    float4 lightPos;
    float4 lightColor;
}

[shader("raygeneration")]
void rayGeneration()
{
    // ピクセル座標
    uint2 dispatchID = DispatchRaysIndex().xy;
    // 全ピクセル数
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    // ピクセルの真ん中に補正, 正規化 (-1.0f - 1.0f)
    float2 uv = (dispatchID + 0.5f) / dispatchDim * 2.0f - 1.0f;
    // スクリーン上のピクセル位置をカメラ座標系からワールド座標系に変換
    // x, y座標はまだわかるけどz座標は工夫の余地ないか？
    float4 target = mul(invViewProj, float4(uv, 1.0f, 1.0f));
    float3 direction = normalize(target.xyz - cameraPosition);
}