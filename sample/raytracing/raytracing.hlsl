cbuffer Camera : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 invViewProj;
    float4x4 invView;
    float4 cameraPosition;
}

cbuffer Light : register(b1)
{
    float4 lightPos;
    float4 lightColor;
}

// Payloadは自分で定義
struct Payload {
    float3 color;
};

// Shader root signature
RaytracingAccelerationStructure SceneBVH : register(t0);  // TLAS

RWTexture2D<float4> Output : register(u0);  // 出力バッファ

[shader("raygeneration")]
void rayGeneration()
{
    // ピクセル座標
    uint2 dispatchID = DispatchRaysIndex().xy;
    // 全ピクセル数
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    // ピクセルの真ん中に補正, 正規化 (-1.0f - 1.0f)
    //float2 uv = (dispatchID + 0.5f) / dispatchDim * 2.0f - 1.0f;
    float2 frameCoord = float2(dispatchID.x, dispatchDim.y - dispatchID.y - 1) + float2(0.5f, 0.5f);
    float2 ndcCoord = (frameCoord) / float2(dispatchDim.x - 1, dispatchDim.y - 1);
    ndcCoord = ndcCoord * 2 - float2(1, 1);
    ndcCoord = ndcCoord / proj[1][1];
    float aspectRatio = (float)dispatchDim.x / (float)dispatchDim.y;
    float3 viewDirection = normalize(float3(ndcCoord.x * aspectRatio, ndcCoord.y, 1.0f));
    float3 rayDirection = normalize(mul((float3x3)invView, viewDirection));
    //// スクリーン上のピクセル位置をカメラ座標系からワールド座標系に変換
    //// x, y座標はまだわかるけどz座標は工夫の余地ないか？
    //// NDC空間のz座標は0 - 1 ここで ndcの一番奥までを対象とすることで
    //// 視錐台に対してレイを飛ばせる
    //float4 target = mul(invViewProj, float4(uv, 1.0f, 1.0f));
    //// カメラからスクリーン方向のレイ
    //float3 direction = normalize(target.xyz - cameraPosition.xyz);

    // RayDescは組み込み型
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDirection;
    ray.TMin = 0.0001f;
    ray.TMax = 10000.0f;

    Payload payload;
    payload.color = float3(0.0f, 0.0f, 0.0f);

    // tlas, rayflag, mask, hit id, geometry , miss id ray, payload
    // geometryはいったん1
    // hit groupが増えたときに気にする
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    Output[dispatchID] = float4(payload.color, 1.0f);
    //Output[dispatchID] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

[shader("closesthit")]
void closestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 beryCentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

    float3 normal = float3(1.0f, 0.0f, 0.0f);

    float3 hitColor = abs(normal);

    payload.color = hitColor;
    //payload.color = float3(1.0f, 1.0f, 1.0f);
}

[shader("miss")]
void miss(inout Payload payload)
{
    payload.color = float3(0.5f, 0.5f, 0.5f);
}