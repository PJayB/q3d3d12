#pragma once

#include "../gamethread/gt_shared.h"

namespace Win8
{

enum WIN8_GAME_MSG
{
    _GAME_MSG_WIN8_START = GameThread::GAME_MSG_CUSTOM,
    GAME_MSG_WIN8_VIDEO_CHANGE,
    GAME_MSG_WIN8_MOUSE_MOVE,
    GAME_MSG_WIN8_MOUSE_WHEEL,
    GAME_MSG_WIN8_MOUSE_DOWN,
    GAME_MSG_WIN8_MOUSE_UP
};

}
