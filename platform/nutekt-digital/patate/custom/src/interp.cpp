#include <custom/interp.h>

#include "float_math.h"

float custom::linearstep(const float left, const float right, const float xx)
{
    return clip01f((xx - left) / (right - left));
}

float custom::smoothstep(const float left, const float right, const float xx)
{
    const auto aa = clip01f((xx - left) / (right - left));
    return 3 * aa * aa - 2 * aa * aa * aa;
}
