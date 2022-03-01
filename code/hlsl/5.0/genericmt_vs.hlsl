#include "vscommon.h"

struct VS_Data
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 AlbedoTC : TEXCOORD0;
    float2 LightmapTC : TEXCOORD1;
    float4 Color : COLOR;
};

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

VS_PS_Data main(VS_Data input)
{
	VS_PS_Data output;

	float4 viewPos = mul(View, float4(input.Position, 1));
    float4 sPos = mul(Projection, viewPos);
	output.Position = DepthRangeHack(sPos);
    output.AlbedoTC = input.AlbedoTC;
    output.LightmapTC = input.LightmapTC;
	output.Color = input.Color;
    output.ViewPos = viewPos;
	output.WorldPos = input.Position;
	output.Normal = input.Normal;

	return output;
}
