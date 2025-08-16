#include "common.hlsli"

Texture2D<float2> mvec : register(t0);
// Texture2D<float> depthBuffer : register(t1);
// Texture2D<float2> mvecProcessed : register(t2);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

float4 ps_main(CommonVertex inp) : SV_TARGET {
    float2 modifiedUV = inp.texcoord;
    modifiedUV.x = modifiedUV.x * 0.5;
//     if(modifiedUV.y > 0.5) {
//       float depth = depthBuffer.Sample(LinearSampler, modifiedUV);
//       return float4(depth, depth, depth, 1);
//     }
    
    // Sample the processed motion vectors texture
    // Since the compute shader already processed the motion vectors calculations,
    // we can just directly use the results
    float2 result = mvec.Sample(PointSampler, modifiedUV);
    float4 outputColor = float4(0, 0, 0, 1);
    result = result * 40.0;
    if(result.x > 0) {
        outputColor.x = result.x;
    } else {
        outputColor.z = abs(result.x);
    }
    outputColor.y = abs(result.y);

    return outputColor;
}
