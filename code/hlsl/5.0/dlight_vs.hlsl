#include "vscommon.h"

struct VS_Data
{
	float3 Position : POSITION;
    float4 Normal : NORMAL;
};

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
    float4 ViewPos : TEXCOORD0;
	float3 Normal : NORMAL;
    float3 WorldPos : TEXCOORD2;
};

VS_PS_Data main(VS_Data input)
{
	VS_PS_Data output;

	float4 viewPos = mul(View, float4(input.Position, 1));
    float4 sPos = mul(Projection, viewPos);
	output.Position = DepthRangeHack(sPos);
    output.ViewPos = viewPos;
	output.Normal = input.Normal.xyz;
    output.WorldPos = input.Position;

   	return output;
}
