cbuffer Constants : register(b0)
{
    float4x4 correctionMatrix;
    float4x4 cameraMotionCorrectionMatrix;
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
        float4 screenSpaceEyeOffsetReprojected = mul(correctionMatrix, screenSpaceCurrent);
        float2 ssEyeOffsetReprojected = screenSpaceEyeOffsetReprojected.xy / screenSpaceEyeOffsetReprojected.w;
        float4 cameraPastFrameReprojected = mul(cameraMotionCorrectionMatrix, screenSpaceCurrent);
        float2 ssCameraPastFrameReprojected = cameraPastFrameReprojected.xy / cameraPastFrameReprojected.w;
        float2 eyeMotionVector = ssCurrent - ssEyeOffsetReprojected;
        float2 cameraMotionVector = ssCurrent - ssCameraPastFrameReprojected;
        mvec[pixelId] =  motionVector + eyeMotionVector * 1.0 + cameraMotionVector * 1.0;
    }
}