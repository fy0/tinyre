
#include "platform.h"
#include <stdio.h>
#include <locale.h>


#ifdef PLATFORM_WINDOWS
#include <windows.h>

wchar_t* _utf8_to_16(const char* str) {
    int nwLen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);

    wchar_t *pwBuf = malloc(sizeof(wchar_t) * (nwLen + 1));
    memset(pwBuf, 0, nwLen * 2 + 2);

    MultiByteToWideChar(CP_UTF8, 0, str, strlen(str), pwBuf, nwLen);

    return pwBuf;
}
#endif


void printf_u8(const char *fmt, ...) {
#if defined(PLATFORM_WINDOWS)
    // 在 windows 控制台输出一个 utf8 字符串，代价巨大
    // 同时我并不理解 MSVC 的思路，不知道如何同时使用 UTF-8 no bom 编码文件同时正常输出utf8
    // 只有妥协
    int size;

    va_list args;
    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *buf = malloc(sizeof(char) * (size+1));
    
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    wchar_t* final_str = _utf8_to_16(buf);
    wprintf(final_str);

    free(buf);
    free(final_str);
#else
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);    
#endif
}

void platform_init() {
#ifdef PLATFORM_WINDOWS
    setlocale(LC_CTYPE, "");
#endif
}
