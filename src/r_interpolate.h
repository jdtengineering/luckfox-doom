// SPDX-License-Identifier: GPL-2.0-or-later
// Render-only interpolation. Never change positions used by game simulation.
#ifndef R_INTERPOLATE_H
#define R_INTERPOLATE_H
#include <stdlib.h>
#include "doomgeneric.h"
#include "doomstat.h"
#include "i_timer.h"
#include "p_local.h"
extern fixed_t render_motion_fraction;

static inline fixed_t R_MotionFraction(const mobj_t *mo)
{
    if (!DG_StreamActive || paused || menuactive || mo->render_oldtic != leveltime
        || leveltime == 0 || (mo->player && mo->reactiontime > 0)
        || llabs((long long)mo->x - mo->render_oldx) > 128 * FRACUNIT
        || llabs((long long)mo->y - mo->render_oldy) > 128 * FRACUNIT)
        return FRACUNIT;
    return render_motion_fraction;
}

static inline fixed_t R_MotionLerp(fixed_t from, fixed_t to, fixed_t fraction)
{
    return from + ((int64_t)to - from) * fraction / FRACUNIT;
}

static inline angle_t R_MotionAngle(angle_t from, angle_t to, fixed_t fraction)
{
    return from + (int64_t)(int32_t)(to - from) * fraction / FRACUNIT;
}
#endif
