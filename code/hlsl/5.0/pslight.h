
#ifdef _ARM_
#   error This shouldn't be included on ARM builds.
#endif

#define MAX_DLIGHTS  32 // TODO: This won't work on 9.3
#define DLIGHT_SCALE 4

struct Dlight
{
    float3 Origin;
    float  Radius;
    float3 Color;
	float __padding;
};

cbuffer DlightsPS : register(b1)
{
	Dlight c_Dlights[MAX_DLIGHTS];
}

cbuffer DlightInfoPS : register(b2)
{
	int c_ModLightCount;
	int c_AddLightCount;
}

float AttenuateLight(
	float3 fragPos,
	float3 fragNormal,
	float3 lightPos,
	float  lightRadius )
{
    float3 lightVec = lightPos - fragPos;
	float d = length(lightVec);
    float lightAtten = max(0, 1 - d / lightRadius);
	lightVec /= d;
	lightAtten *= max(0, dot(lightVec, fragNormal));
	return lightAtten;
}

float3 GetModulatedLighting(float3 worldPos, float3 normal)
{
	float3 light = 0;
	for (int i = 0; i < c_ModLightCount; ++i)
	{
		float lightAtten = AttenuateLight(
			worldPos,
			normal,
			c_Dlights[i].Origin,
			c_Dlights[i].Radius );
        
		light += lightAtten * DLIGHT_SCALE * c_Dlights[i].Color;
	}
	return light;
}

float3 GetAdditiveLighting(float3 worldPos, float3 normal)
{
	float3 light = 0;
	for (int i = 0; i < c_AddLightCount; ++i)
	{
		float lightAtten = AttenuateLight(
			worldPos,
			normal,
			c_Dlights[i].Origin,
			c_Dlights[i].Radius );

		light += lightAtten * DLIGHT_SCALE * c_Dlights[i].Color;
	}
	return light;
}

