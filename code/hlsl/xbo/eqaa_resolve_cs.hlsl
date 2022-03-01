//--------------------------------------------------------------------------------------
// iFMask.hlsl
//
// Routines for interpreting iFMask
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define ENABLE_EQAA

#define MAX_NUM_FRAGMENTS 16

// This code supports either a single ubershader which handles all MSAA settings, 
// or else specializations for each Count and Quality.  
// To create an ubershader, set a constant buffer matching cbFMask below.
// To create specialized shaders, compile with LOG_NUM_FRAGMENTS and LOG_NUM_SAMPLES 
// and COLOR_EXPANDED #defined.  
#if defined(LOG_NUM_FRAGMENTS) && defined(LOG_NUM_SAMPLES) && defined(COLOR_EXPANDED)

// A sample is a sub-pixel location.  
// A fragment is a sub-pixel color. 
// We can have more than one sample assigned to a fragment.
#define LOG_FRAGMENTS LOG_NUM_FRAGMENTS
#if LOG_NUM_SAMPLES < LOG_FRAGMENTS
#define LOG_SAMPLES LOG_FRAGMENTS
#else 
#define LOG_SAMPLES LOG_NUM_SAMPLES
#endif

#define NUM_FRAGMENTS ( 1 << LOG_FRAGMENTS )
#define NUM_SAMPLES ( 1 << LOG_SAMPLES )

// An iFMask index must be able to encode any fragment.
// Also, if there are more samples than fragments, we require a code for "unknown" color.
// In practice, the hardware uses power-of-two index sizes.
#if LOG_SAMPLES > LOG_FRAGMENTS
#define MIN_INDEX_BITS (LOG_FRAGMENTS + 1)
#else
#define MIN_INDEX_BITS LOG_FRAGMENTS
#endif

#if MIN_INDEX_BITS <= 2 && !defined( NATIVE_FMASK )
#define INDEX_BITS MIN_INDEX_BITS
#else
#define INDEX_BITS 4
#endif

// Anything which hits outside the VALID_INDEX_MASK is "unknown".  Our choice how
// to handle these.  We simply omit them, and scale the remaining colors.
#define INDEX_MASK ( ( 1 << INDEX_BITS ) - 1 )
#define VALID_INDEX_MASK ( NUM_FRAGMENTS - 1 )
#define INVALID_INDEX_MASK ~VALID_INDEX_MASK

#else

#define UBERSHADER

cbuffer cbFMask : register(b0)
{
    bool COLOR_EXPANDED;
    uint LOG_NUM_FRAGMENTS;
    uint LOG_NUM_SAMPLES;
    uint NUM_FRAGMENTS;
    uint NUM_SAMPLES;
    uint INDEX_BITS;
    uint INDEX_MASK;
    uint VALID_INDEX_MASK;
    uint INVALID_INDEX_MASK;
};

#endif

//-------------------------------------------------------------------------------------------------------------
// Name: ManualLoad()
// Desc: Load the nth sample color, by looking up the index in the FMask, and then looking up that fragment
// in the color buffer.  
//
// Set bUnknown if the sample has the index which codes "unknown".  In this case, return some valid color 
// for the pixel.
//
// Note this is not the most efficient way to iterate through all samples, as you may end up fetching the
// same fragment color more than once.
//-------------------------------------------------------------------------------------------------------------
float4 ManualLoad(uniform Texture2DMS<float4> TexColorMS, uniform Texture2D<uint2> TexFMask, uint2 viTexcoord, uniform uint iSample, out bool bUnknown)
{
    uint2 viFMaskXY = TexFMask.Load(uint3(viTexcoord, 0));
    uint iShift = iSample * INDEX_BITS;
    uint iFrag = (iShift < 32)
        ? ((viFMaskXY.x >> iShift) & INDEX_MASK)
        : ((viFMaskXY.y >> (iShift - 32)) & INDEX_MASK);

    bUnknown = (iFrag & INVALID_INDEX_MASK);
    return TexColorMS.Load(viTexcoord, iFrag & VALID_INDEX_MASK);
}

//-------------------------------------------------------------------------------------------------------------
// Name: ManualResolve()
// Desc: Returns the average of the fragment colors, weighted by multiplicity.  Ignore unknown samples.
// Take care not to fetch the same sample repeatedly.
//-------------------------------------------------------------------------------------------------------------
float4 ManualResolve(uniform Texture2DMS<float4> TexColorMS, uniform Texture2D<uint2> TexFMask, uint2 viTexcoord)
{
    // FragmentWeight records the number of samples which hit each fragment (if any).
    uint FragmentWeight[MAX_NUM_FRAGMENTS];
    [unroll]
    for (uint i = 0; i < NUM_FRAGMENTS; ++i)
    {
        FragmentWeight[i] = 0;
    }

    // Read the iFMask, and decode the indices for each sample.
    uint2 viFMaskXY = TexFMask.Load(uint3(viTexcoord, 0));
    uint iFMask = viFMaskXY.x;
    uint iTotalWeight = 0;
#ifndef UBERSHADER
    [unroll] // this breaks the ubershader
#endif
    for (uint iSample = 0; iSample < NUM_SAMPLES; ++iSample)
    {
        uint iFrag = iFMask & INDEX_MASK;
        iFMask >>= INDEX_BITS;

        [flatten]
        if ((iSample + 1) * INDEX_BITS == 32)    // used 32 bits ... move to the next component
        {
            iFMask = viFMaskXY.y;
        }

        // If "unknown" code, skip the sample.  An alternative would be to use a nearby 
        // known sample.  GPU guarantees that iSample & ( NUM_FRAGMENTS - 1 ) is such a sample.
        [unroll]
        for (uint i = 0; i < NUM_FRAGMENTS; ++i)
        {
            [flatten]
            if (iFrag == i)
            {
                ++FragmentWeight[i];
                ++iTotalWeight;
            }
        }
    }

    // Fetch any fragment colors which have non-zero weight, and compute the weighted average.
    float4 vResolvedColor = 0;
    [unroll]
    for (uint iFrag = 0; iFrag < NUM_FRAGMENTS; ++iFrag)
    {
        if (FragmentWeight[iFrag] > 0)
        {
            vResolvedColor += FragmentWeight[iFrag] * TexColorMS.Load(viTexcoord, iFrag);
        }
    }
    vResolvedColor /= iTotalWeight;

    return vResolvedColor;
}

//-------------------------------------------------------------------------------------------------------------
// Name: CSManualResolve()
// Desc: Manually resolve the MS texture by reading FMask and Color and writing to output texture.
//-------------------------------------------------------------------------------------------------------------
Texture2DMS<float4> TexColorMS : register(t0);
Texture2D<uint2> TexFMask : register(t1);
RWTexture2D<float4> TexColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float4 ResolvedColor = ManualResolve(TexColorMS, TexFMask, id.xy);
    TexColor[id.xy] = ResolvedColor;
}
