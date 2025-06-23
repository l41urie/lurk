#include "logger.hpp"
#include "settings.hpp"
#include <MinHook.h>
#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_set>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
}

namespace luahook{
void (*luaL_openlibs_o)();
int (*luaL_loadbufferx_o)(lua_State *L, const char *buff, size_t sz,
                          const char *name, const char *mode);
int (*lua_pcall_o)(lua_State *L, int nargs, int nresults, int msgh);
void (*lua_call_o)(lua_State *L, int nargs, int nresults);
}

extern "C" {

char const *errstr(lua_State *L, int32_t status) {
  switch (status) {
  case LUA_ERRFILE: {
    return "LUA_ERRFILE";
  }
  case LUA_ERRRUN:
  case LUA_ERRSYNTAX: {
    return lua_tostring(L, -1);
  }
  case LUA_ERRMEM: {
    return "LUA_ERRMEM";
  }
  }

  static char buff[256];
  sprintf_s(buff, "Error %x\n", status);
  return buff;
}

int32_t traceback(lua_State *L) {
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) || !luaL_callmeta(L, 1, "__tostring") ||
        !lua_isstring(L, -1))
      return 1;       /* Return non-string error object. */
    lua_remove(L, 1); /* Replace object by result of __tostring metamethod. */
  }
  luaL_traceback(L, L, lua_tostring(L, 1), 1);
  return 1;
}

static int log_info(lua_State *L) {
  int n = lua_gettop(L); /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i = 1; i <= n; i++) {
    const char *s;
    lua_pushvalue(L, -1); /* function to be called */
    lua_pushvalue(L, i);  /* value to print */
    luahook::lua_call_o(L, 1, 1);
    s = lua_tostring(L, -1); /* get result */
    if (s == NULL)
      return luaL_error(
          L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
    if (i > 1)
      logger::info("\t");
    logger::info("%s", s);
    lua_pop(L, 1); /* pop result */
  }
  logger::info("\n");
  return 0;
}

int32_t docall(lua_State *L, int narg, int clear) {
  int32_t status;
  int32_t base = lua_gettop(L) - narg; /* function index */
  lua_pushcfunction(L, traceback);     /* push traceback function */
  lua_insert(L, base);                 /* put it under chunk and args */
  status = luahook::lua_pcall_o(L, narg, (clear ? 0 : LUA_MULTRET), base);
  lua_remove(L, base); /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != LUA_OK)
    lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}
}

namespace luahook {

std::unordered_set<lua_State *> seen_states;

bool boot_mod(lua_State *L) {
  if (seen_states.find(L) != seen_states.end())
  {
    if(lurk::global_settings.periodic_function)
    {
      lua_getglobal(L, lurk::global_settings.periodic_function);
      if (lua_type(L, -1) == LUA_TFUNCTION) {
        docall(L, 0, 1);
      }
    }
    return false;
  }

  seen_states.emplace(L);

  // is this wrong??
  lua_pushcfunction(L, ::log_info);
  lua_setglobal(L, "info");

  // Load custom code upon boot
  if (lurk::global_settings.load_path) {
    auto res = luaL_loadfile(L, lurk::global_settings.load_path);

    if (res) {
      logger::err("failed to load mod\n");
      return false;
    }

    res = docall(L, 0, 0);
    if (res) {
      char const *e = errstr(L, res);
      logger::err("failed to execute mod (%s)\n", e);
      return false;
    }
  }

  return true;
}

void luaL_openlibs_replacement(lua_State *L) {
  logger::enable_dbg_chan = true;
  luaL_openlibs_o();
  logger::info("luaL_openlibs(%p)\n", L);

  if (boot_mod(L)) {
    logger::dbg("booted mod\n");
  }
}

int luaL_loadbufferx_replacement(lua_State *L, const char *buff, size_t sz,
                                 const char *name, const char *mode) {

  auto result = luaL_loadbufferx_o(L, buff, sz, name, mode);

  if (boot_mod(L)) {
    logger::dbg("booted mod (loadbuffer)\n");
  }

  auto hash_str = [](char const *p) {
    const uint64_t prime = 0x00000100000001b3;
    uint64_t hash = 0xcbf29ce484222325;

    while (*p)
      hash = (hash ^ *(p++)) * prime;
    return hash;
  };

  auto alnumonly = [](char const *str, uint32_t maxch = 25) -> std::string {
    std::string result;
    result.reserve(strlen(str));

    while (*str && (--maxch != 0)) {
      char ch = *(str++);
      if (std::isalnum(ch))
        result.push_back(ch);
    }

    return result;
  };

  if (lurk::global_settings.dump_path) {
    auto nhash = hash_str(name);

    auto p = std::filesystem::absolute(lurk::global_settings.dump_path) /
             (alnumonly(name) + '_' + std::to_string(nhash) + ".lua");

    std::ofstream out(p, std::ios::binary);
    if (!out) {
      logger::err("Failed to open file for writing: %s\n", p.string().c_str());
    } else {
      out.write(buff, sz);
      if (!out) {
        logger::err("Failed to write to file: %s\n", p.string().c_str());
      } else {
        logger::info("Dumped buffer to file: %s\n", p.string().c_str());
      }
    }
  }

  return result;
}

int lua_pcall_replacement(lua_State *L, int nargs, int nresults, int msgh) {
  boot_mod(L);
  return lua_pcall_o(L, nargs, nresults, msgh);
}

void lua_call_replacement(lua_State *L, int nargs, int nresults) {
  boot_mod(L);
  return lua_call_o(L, nargs, nresults);
}

struct LuaAPIHook {
  char const *name;
  void *replacement;
  void **orig;
};

#define HOOK(name) {#name, (void *)name##_replacement, (void **)&name##_o}

LuaAPIHook const hooks[] = {
    HOOK(luaL_loadbufferx),
    HOOK(luaL_openlibs),
    HOOK(lua_pcall),
    HOOK(lua_call),
};

void setup(HMODULE luabase) {
  if (MH_Initialize() != MH_OK) {
    logger::err("failed to initialize minhook\n");
    return;
  }

  for (auto const &hk : hooks) {
    auto fn = GetProcAddress(luabase, hk.name);
    if (!fn || MH_CreateHook((void *)fn, hk.replacement, hk.orig) != MH_OK) {
      logger::err("Creating hook on %s failed\n", hk.name);
      continue;
    }

    logger::info("Hooked %s (%p)\n", hk.name, (void *)fn);
  }

  if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
    logger::err("Enabling hooks failed\n");
    return;
  }

  // assume everything went good from here.
  logger::enable_dbg_chan = false;
}
} // namespace luahook