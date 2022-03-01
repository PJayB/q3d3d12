
cbuffer Globals : register(b0)
{
    int2 SrcSize;
};

Texture2D<float4>    SourceBuf : register(t0);
RWTexture2D<float4>  TargetBuf : register(u0);

// Yes I know this is horrible but I just wanted to test whether my shaders were working

[numthreads(1, 1, 1)]
void main( int3 DTid : SV_DispatchThreadID )
{
	int2 TL = int2(0,0);
	int2 minXY = clamp(DTid.xy-3, TL, SrcSize);
	int2 maxXY = clamp(DTid.xy+3, TL, SrcSize);

	float4 s = float4(0, 0, 0, 0);
	int c = 0;
	for (int y = minXY.y; y <= maxXY.y; ++y) 
	{
		for (int x = minXY.x; x <= maxXY.x; ++x)
		{
		    s += SourceBuf.Load(int3(x, y, 0));
			++c;
		}
	}
	if (c > 0)
	{
		TargetBuf[DTid.xy] = s / (float)c;
	}
}
