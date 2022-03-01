#include "pscommon.h"
#include "pslight.h"

Texture2D	 Diffuse : register(t0);
SamplerState Sampler : register(s0);

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 AlbedoTC : TEXCOORD0;
    float4 Color : COLOR;
    float4 ViewPos : TEXCOORD2;
	float3 WorldPos : TEXCOORD3;
	float3 Normal : NORMAL;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
    ClipToPlane(input.ViewPos);
	
	// Lighting is stored in the vertex color
	float3 light = 
		input.Color.xyz +
		GetModulatedLighting(input.WorldPos, input.Normal);

    return FinalColor(
        float4(light, input.Color.a) *
        GammaCorrect(Diffuse.Sample(Sampler, input.AlbedoTC)));
}
