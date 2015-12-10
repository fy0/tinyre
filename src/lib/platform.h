
#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#if defined(_WIN32) && !defined(_WIN32_WCE)
#define PLATFORM_WINDOWS  /* enable goodies for regular Windows */
#endif

#ifdef _MSC_VER
#define _INLINE
#pragma execution_character_set("utf-8")
#else
#define _INLINE inline
#endif

void printf_u8(const char *fmt, ...);

void platform_init();

#endif
