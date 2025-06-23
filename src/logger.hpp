#pragma once
#include <cstdint>
#include <cstdio>
#include <windows.h>

namespace logger {
static auto constexpr GREY = 8;
static auto constexpr WHITE = 15;
static auto constexpr RED = 4;
static auto constexpr BLUE = 11;
static auto constexpr GREEN = 10;

template <uint32_t Color, typename... Args> inline void log(Args... args) {
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);
  printf(args...);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
}

inline bool enable_dbg_chan = true;
template <typename... Args> inline void dbg(Args... args) {
    if(!enable_dbg_chan)return;
  return log<GREY>(args...);
}

inline bool enable_info_chan = true;
template <typename... Args> inline void info(Args... args) {
  if(!enable_info_chan) return;
  return log<BLUE>(args...);
}

template <typename... Args> inline void err(Args... args) {
  return log<RED>(args...);
}

inline void spawn_console()
{
  AllocConsole();
  FILE *a;
  freopen_s(&a, "CONIN$", "r", stdin);
  freopen_s(&a, "CONOUT$", "w", stdout);
  freopen_s(&a, "CONOUT$", "w", stderr);
}
} // namespace logger