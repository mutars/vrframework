#include "common.hlsli"

cbuffer Constants : register(b0)
{
    float  scale;         // Scale factor for visualization
    uint   channel_mask;  // Bitmask: bit0=R, bit1=G, bit2=B, bit3=A
    uint   show_abs;      // Show absolute values
    uint   padding;
}


Texture2D<float2> mvec : register(t0);
// Texture2D<float> depthBuffer : register(t1);
// Texture2D<float2> mvecProcessed : register(t2);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

float4 ps_main(CommonVertex inp) : SV_TARGET {
    float2 modifiedUV = inp.texcoord;
//     modifiedUV.x = modifiedUV.x * 0.5;
//     if(modifiedUV.y > 0.5) {
//       float depth = depthBuffer.Sample(LinearSampler, modifiedUV);
//       return float4(depth, depth, depth, 1);
//     }
    
    // Sample the processed motion vectors texture
    // Since the compute shader already processed the motion vectors calculations,
    // we can just directly use the results
    float2 result = mvec.Sample(PointSampler, modifiedUV);
    float4 outputColor = float4(0, 0, 0, 1);
    result = result * scale;
    
    // Apply absolute value if enabled
    float2 displayValue = show_abs ? abs(result) : result;
    
    // Map channels based on mask
    // For 2-channel texture (motion vectors): X -> R/B (positive/negative), Y -> G
    if (channel_mask & 0x1) { // R channel
        outputColor.r = show_abs ? displayValue.x : max(0, result.x);
    }
    if (channel_mask & 0x2) { // G channel
        outputColor.g = displayValue.y;
    }
    if (channel_mask & 0x4) { // B channel
        outputColor.b = show_abs ? 0 : max(0, -result.x);
    }
    if (channel_mask & 0x8) { // A channel (for 4-channel textures)
        outputColor.a = 1.0;
    }

    return outputColor;
}
