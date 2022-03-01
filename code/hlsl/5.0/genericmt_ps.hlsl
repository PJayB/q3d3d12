#include "pscommon.h"
#include "pslight.h"

Texture2D Diffuse	 : register(t0);
Texture2D Lightmap   : register(t1);

SamplerState Sampler : register(s0);

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 AlbedoTC : TEXCOORD0;
    float2 LightmapTC : TEXCOORD1;
    float4 Color : COLOR;
    float4 ViewPos : TEXCOORD2;
	float3 WorldPos : TEXCOORD3;
	float3 Normal : NORMAL;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
    ClipToPlane(input.ViewPos);

	// Get the initial lighting from the lightmap
	float3 light = 
		Lightmap.Sample(Sampler, input.LightmapTC).xyz +
		GetModulatedLighting(input.WorldPos, input.Normal);
    
    return FinalColor(
        input.Color *
        GammaCorrect(Diffuse.Sample(Sampler, input.AlbedoTC)) *
        float4(light, 1));
}
