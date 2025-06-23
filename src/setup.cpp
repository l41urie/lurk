#include "setup.hpp"
#include "logger.hpp"
#include <tlhelp32.h>
#include <vector>
#include <windows.h>
#include "luahook.hpp"

namespace detail {
struct AutoClose {
  HANDLE h;
  operator HANDLE &() { return h; }
  ~AutoClose() { CloseHandle(h); }
};

struct Mod {
  uintptr_t base;
  char modname[256];

  bool operator==(Mod const &rhs) { return rhs.base == this->base; }
};

std::vector<Mod> loaded_modules() {

  AutoClose hModuleSnap{
      CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId())};

  if (hModuleSnap == INVALID_HANDLE_VALUE)
    return {};

  MODULEENTRY32 me32;
  //  Set the size of the structure before using it.
  me32.dwSize = sizeof(MODULEENTRY32);

  //  Retrieve information about the first module,
  //  and exit if unsuccessful
  if (!Module32First(hModuleSnap, &me32))
    return {};

  std::vector<Mod> modlist;
  do {
    Mod mod;
    mod.base = (uintptr_t)me32.modBaseAddr;
    strcpy_s(mod.modname, me32.szModule);
    modlist.emplace_back(mod);
  } while (Module32Next(hModuleSnap, &me32));
  return modlist;
}
} // namespace detail

namespace lurk {

void on_new_module(char const *name, uintptr_t base) {
  logger::dbg("new module %s @ %llx\n", name, base);

  if (!strcmp(name, "lua51.dll")) {
    logger::info("Found lua51 @ %llx\n", base);
    luahook::setup((HMODULE)base);
  }
}

void on_attach(void *startup_data) {
  logger::spawn_console();
  new_thread();
}

std::vector<detail::Mod> last_modlist;
void new_thread() {
  // find new modules
  auto curmodlist = detail::loaded_modules();
  for (auto &lmod : curmodlist) {
    if (std::find(last_modlist.begin(), last_modlist.end(), lmod) ==
        last_modlist.end()) {
      // new module found
      on_new_module(lmod.modname, lmod.base);
    }
  }

  // tiresome to modify the list in-place, just override all of it..
  last_modlist = curmodlist;
}
} // namespace lurk