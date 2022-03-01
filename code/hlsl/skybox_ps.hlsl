#include "pscommon.h"

cbuffer SkyBoxDataPS : register(b0)
{
    float4 ColorTint;
};

Texture2D	 Diffuse : register(t0);
SamplerState Sampler : register(s0);

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 AlbedoTC : TEXCOORD0;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
    return GammaCorrect(ColorTint * Diffuse.Sample(Sampler, input.AlbedoTC));
}
