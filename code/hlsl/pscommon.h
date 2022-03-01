
cbuffer ViewDataPS : register(b0)
{
    float4 c_ClipPlane;
    float2 c_AlphaClip;
};

void ClipToPlane(float4 vpos)
{
    clip(dot(c_ClipPlane, vpos));
}

float4 FinalColor(float4 c)
{
    clip((c.a - c_AlphaClip[1]) * c_AlphaClip[0]);
    return c;
}

float4 GammaCorrect(float4 c)
{
    return c; // passthrough for now
}
