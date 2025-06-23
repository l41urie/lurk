#include "settings.hpp"
#include "setup.hpp"
#include <Windows.h>
#include <cstdio>

extern "C" {
__declspec(dllexport) unsigned int set_parameters(char const *param)  {
  lurk::set_settings(param);
  printf("set parameters\n\n");
  return 1;
}
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_THREAD_ATTACH)
    lurk::new_thread();

  if (dwReason == DLL_PROCESS_ATTACH)
    lurk::on_attach(lpReserved);
  return TRUE;
}