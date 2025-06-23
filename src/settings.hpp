#pragma once
namespace lurk {
struct Settings {
  char const *dump_path{};
  char const *load_path{};
  char const *periodic_function{};
};

inline Settings global_settings;
void set_settings(char const *args);
} // namespace lurk