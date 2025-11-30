cbuffer Constants : register(b0)
{
    float4x4 undoCameraMotion;
    float4x4 cameraMotionCorrection;
    float4 textureSize;
}

// Input textures
Texture2D<float2> depth : register(t0);

// Output texture
RWTexture2D<float2> mvec : register(u0);

// correct motion vector is MV = P_Current - P_Previous
[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelId = DTid.xy;
    float2 pixelCenter = float2(pixelId)+0.5;

    if (pixelId.x >= textureSize.x || pixelId.y >= textureSize.y)
    {
        return;
    }

    float2 motionVector = mvec[pixelId].xy;

     {
        float2 uvCurrent = pixelCenter * textureSize.zw;
        float2 ssCurrent = float2(uvCurrent.x * 2.0 - 1.0, 1.0 - uvCurrent.y * 2.0);
        float curDepth = depth[pixelId].x;

        float4 screenSpaceCurrent = float4(ssCurrent, curDepth, 1.0);
        float4 screenSpace2FramesReprojected = mul(cameraMotionCorrection, screenSpaceCurrent);
        float2 ss2FramesReprojected = screenSpace2FramesReprojected.xy / screenSpace2FramesReprojected.w;
        float4 cameraPastFrameReprojected = mul(undoCameraMotion, screenSpaceCurrent);
        float2 ssCameraPastFrameReprojected = cameraPastFrameReprojected.xy / cameraPastFrameReprojected.w;
        float2 twoFramesCameraVector = ssCurrent - ss2FramesReprojected;
        float2 cameraMotionVector = ssCurrent - ssCameraPastFrameReprojected;
        motionVector = 2.0 * (motionVector - cameraMotionVector * 1.0);
        mvec[pixelId] =  motionVector + twoFramesCameraVector * 1.0;
    }
}