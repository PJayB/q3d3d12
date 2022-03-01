#pragma once

#include "xbo_common.h"
#include "../gamethread/gt_shared.h"

namespace XBO
{

enum GAME_MSG
{
    _GAME_MSG_XBO_START = GameThread::GAME_MSG_CUSTOM,
    GAME_MSG_XBO_CONSTRAINED,
    GAME_MSG_XBO_UNCONSTRAINED,
    GAME_MSG_XBO_USER_LIST_UPDATED
};

}
