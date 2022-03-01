#include "vscommon.h"

struct VS_Data
{
	float3 Position : POSITION;
	float2 AlbedoTC : TEXCOORD0;
};

struct VS_PS_Data
{
	float4 Position : SV_POSITION;
	float2 AlbedoTC : TEXCOORD0;
  float4 ViewPos : TEXCOORD2;
};

VS_PS_Data main(VS_Data input)
{
    VS_PS_Data output;

    float4 viewPos = mul(View, float4(input.Position, 1));
    float4 sPos = mul(Projection, viewPos);
    output.Position = DepthRangeHack(sPos);
    output.AlbedoTC = input.AlbedoTC;
    output.ViewPos = viewPos;

   	return output;
}
