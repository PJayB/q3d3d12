#include "pscommon.h"

Texture2D Diffuse		: register(t0);
SamplerState Sampler	: register(s0);

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Color : COLOR;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
	return FinalColor(float4(input.Color, 1) * GammaCorrect(Diffuse.Sample(Sampler, input.TexCoord)));
}
