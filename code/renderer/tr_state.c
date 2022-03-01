/*
===========================================================

@pjb: converts a state bitmask into a string

===========================================================
*/

#include "tr_local.h"
#include "tr_state.h"

typedef struct glStateMaskName_s {
    const char* name;
    unsigned mask;
} glStateMaskName_t;

static void JoinMaskString(char* buf, int bufLen, int* count, const char* maskName) {
    if (*count > 0) {
        Q_strcat(buf, bufLen, "|");
    }
    Q_strcat(buf, bufLen, maskName);
    (*count)++;
}

void R_GetGLStateMaskString(char* buf, int bufLen, int mask) {
    int count = 0, i;
#define STATE_MASK_NAME(x) { #x, x }
    const glStateMaskName_t stateMaskNames[] = {
        STATE_MASK_NAME(GLS_DEPTHMASK_TRUE),
        STATE_MASK_NAME(GLS_POLYMODE_LINE),
        STATE_MASK_NAME(GLS_DEPTHTEST_DISABLE),
        STATE_MASK_NAME(GLS_DEPTHFUNC_EQUAL)
    };
#undef STATE_MASK_NAME

#define STATE_MASK_NAME(x) case x: JoinMaskString(buf, bufLen, &count, #x); break
    if (mask & GLS_SRCBLEND_BITS) {
        switch (mask & GLS_SRCBLEND_BITS) {
            STATE_MASK_NAME(GLS_SRCBLEND_ZERO);
            STATE_MASK_NAME(GLS_SRCBLEND_ONE);
            STATE_MASK_NAME(GLS_SRCBLEND_DST_COLOR);
            STATE_MASK_NAME(GLS_SRCBLEND_ONE_MINUS_DST_COLOR);
            STATE_MASK_NAME(GLS_SRCBLEND_SRC_ALPHA);
            STATE_MASK_NAME(GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA);
            STATE_MASK_NAME(GLS_SRCBLEND_DST_ALPHA);
            STATE_MASK_NAME(GLS_SRCBLEND_ONE_MINUS_DST_ALPHA);
            STATE_MASK_NAME(GLS_SRCBLEND_ALPHA_SATURATE);
        }
        count++;
    }

    if (mask & GLS_DSTBLEND_BITS) {
        switch (mask & GLS_DSTBLEND_BITS) {
            STATE_MASK_NAME(GLS_DSTBLEND_ZERO				);
            STATE_MASK_NAME(GLS_DSTBLEND_ONE				);
            STATE_MASK_NAME(GLS_DSTBLEND_SRC_COLOR			);
            STATE_MASK_NAME(GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
            STATE_MASK_NAME(GLS_DSTBLEND_SRC_ALPHA			);
            STATE_MASK_NAME(GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
            STATE_MASK_NAME(GLS_DSTBLEND_DST_ALPHA			);
            STATE_MASK_NAME(GLS_DSTBLEND_ONE_MINUS_DST_ALPHA);
        }
        count++;
    }

    if (mask & GLS_ATEST_BITS) {
        switch (mask & GLS_ATEST_BITS) {
            STATE_MASK_NAME(GLS_ATEST_GT_0 );
            STATE_MASK_NAME(GLS_ATEST_LT_80);
            STATE_MASK_NAME(GLS_ATEST_GE_80);
        }
        count++;
    }
#undef STATE_MASK_NAME

    for (i = 0; i < _countof(stateMaskNames); ++i) {
        if ((mask & stateMaskNames[i].mask) == stateMaskNames[i].mask) {
            JoinMaskString(buf, bufLen, &count, stateMaskNames[i].name);
        }
    }

    if (count == 0) {
        Q_strcat(buf, bufLen, "0");
    }
}
