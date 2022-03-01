#pragma once

#include "../gamethread/gt_shared.h"

namespace UWP
{

enum UWP_GAME_MSG
{
    _GAME_MSG_UWP_START = GameThread::GAME_MSG_CUSTOM,
    GAME_MSG_UWP_VIDEO_CHANGE,
    GAME_MSG_UWP_MOUSE_MOVE,
    GAME_MSG_UWP_MOUSE_WHEEL,
    GAME_MSG_UWP_MOUSE_DOWN,
    GAME_MSG_UWP_MOUSE_UP
};

}
