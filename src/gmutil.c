/*
 *  gmutil.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include "gmutil.h"
#include "core.h"

/**
 * Used on platforms where strlcpy() is missing.
 */
size_t gmu_strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';                /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return (s - src - 1);        /* count does not include NUL */
}

/**
 * Used on platforms where strlcat() is missing.
 */
size_t gmu_strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));        /* count does not include NUL */
}

/**
 * Not interrupted by signals.
 */
void gmu_sleep_ms(int ms)
{
    if (ms <= 0)
        return;
    int target = gmu_add_s32(tick, ms);
    while (gmu_sub_s32(tick, target) < 0) {
        // spin
    }
}
