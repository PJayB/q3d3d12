#include "pscommon.h"
#include "pslight.h"

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
    float4 ViewPos : TEXCOORD0;
	float3 Normal : NORMAL;
    float3 WorldPos : TEXCOORD2;
};

float4 main(VS_PS_Data input) : SV_TARGET
{
    ClipToPlane(input.ViewPos);

    return float4(GetAdditiveLighting(
		input.WorldPos,
		input.Normal), 0);
}
