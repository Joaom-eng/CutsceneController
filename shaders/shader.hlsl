sampler2D gSampler : register(s0);

// Constants
float2 gImageSize : register(c0);    // Resolution
float2 gPixelOffset : register(c1);  // Blur intensity (Ex: 1.0, 0.0 for horizontal)

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 color = tex2D(gSampler, texCoord) * 0.1633;
    
    float offset[4] = { 1.0, 2.0, 3.0, 4.0 };
    float weight[4] = { 0.1531, 0.1224, 0.0918, 0.0510 };

    for(int i = 0; i < 4; i++) {
        float2 res = (offset[i] * gPixelOffset) / gImageSize;
        color += tex2D(gSampler, texCoord + res) * weight[i];
        color += tex2D(gSampler, texCoord - res) * weight[i];
    }

    float noise = frac(sin(dot(texCoord, float2(12.9898, 78.233))) * 43758.5453);
    color.rgb += (noise - 0.5) * (0.004); // 1.0/255.0 aprox.

    return color;
}