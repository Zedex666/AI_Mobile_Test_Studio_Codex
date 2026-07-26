#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstring>
#include <cstdio>

int main()
{
    Sleep(500);
    char input[64] = {};
    DWORD bytesRead = 0;
    const BOOL read = ReadFile(GetStdHandle(STD_INPUT_HANDLE),
                               input,
                               sizeof(input),
                               &bytesRead,
                               nullptr);
    if (!read || bytesRead == 0) {
        char error[64] = {};
        std::snprintf(error,
                      sizeof(error),
                      "__CONPTY_READ_FAILED_%lu_%lu__\r\n",
                      static_cast<unsigned long>(GetLastError()),
                      static_cast<unsigned long>(bytesRead));
        DWORD ignored = 0;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                  error,
                  static_cast<DWORD>(std::strlen(error)),
                  &ignored,
                  nullptr);
        return 2;
    }

    constexpr char marker[] = "__CONPTY_SMOKE_OK__\r\n";
    DWORD bytesWritten = 0;
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                   marker,
                   static_cast<DWORD>(std::strlen(marker)),
                   &bytesWritten,
                   nullptr)) {
        return 3;
    }
    return 0;
}
