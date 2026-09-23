// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <string.h>
#include "r_interpolate.h"
int DG_StreamActive = 1;
bool paused, menuactive;
int leveltime = 10;
fixed_t render_motion_fraction = FRACUNIT / 2;
int main(void)
{
    mobj_t mo;
    memset(&mo, 0, sizeof mo);
    mo.render_oldtic = leveltime;
    mo.x = 20 * FRACUNIT;
    assert(R_MotionFraction(&mo) == FRACUNIT / 2);
    assert(R_MotionLerp(0, mo.x, FRACUNIT / 2) == 10 * FRACUNIT);
    assert(R_MotionLerp(-10, 20, 0) == -10);
    assert(R_MotionLerp(-10, 20, FRACUNIT) == 20);
    assert(R_MotionLerp(-10 * FRACUNIT, 10 * FRACUNIT, FRACUNIT / 2) == 0);
    assert(R_MotionAngle(0xf0000000u, 0x10000000u, FRACUNIT / 2) == 0);
    assert(R_MotionAngle(0x10000000u, 0xf0000000u, FRACUNIT / 2) == 0);
    paused = true; assert(R_MotionFraction(&mo) == FRACUNIT); paused = false;
    menuactive = true; assert(R_MotionFraction(&mo) == FRACUNIT); menuactive = false;
    mo.render_oldtic = 0; assert(R_MotionFraction(&mo) == FRACUNIT);
    mo.render_oldtic = leveltime; mo.x = 256 * FRACUNIT;
    assert(R_MotionFraction(&mo) == FRACUNIT);
    DG_StreamActive = 0; mo.x = 0;
    assert(R_MotionFraction(&mo) == FRACUNIT);
    return 0;
}
