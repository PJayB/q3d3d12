#include "pscommon.h"

Texture2D	 Diffuse : register(t0);
SamplerState Sampler : register(s0);

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 AlbedoTC : TEXCOORD0;
  float4 ViewPos : TEXCOORD2;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
    ClipToPlane(input.ViewPos);
    return Diffuse.Sample(Sampler, input.AlbedoTC);
}
