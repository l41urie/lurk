#include "settings.hpp"
#include <cstdlib>
#include <cstring>
#include <string>

namespace lurk {
char const *anyeq(char c, char const *opts) {
  while (char test = *(opts++))
    if (test == c)
      return --opts;

  return nullptr;
}

struct Token {
  char *found;
  char *next;
};

Token extract_token(char *param, char const *wait, char const *skip,
                    char const *delims, char const *delims_end = nullptr) {
  char *found = strstr(param, wait);
  if (!found)
    return {};

  found += strlen(wait) - 1;

  while (anyeq(*(++found), skip))
    ;

  if (!anyeq(*(found++), delims))
    return {};

  Token result;
  result.found = found;

  while (!anyeq(*(++found), delims_end ? delims_end : delims)) {
    // string ends early?
    if (*found == 0)
      return {};
  }

  *found = 0;
  result.next = found + 1;
  return result;
}

void set_settings(char const *args) {
  char *dup = _strdup(args);

  Token dump = extract_token(dup, "dump", " \t", "{", "}");
  if (dump.found) {
    static std::string found = dump.found;
    global_settings.dump_path = found.c_str();
  }

  Token load =
      extract_token(dump.next ? dump.next : dup, "load", " \t", "{", "}");
  if (load.found) {
    static std::string found = load.found;
    global_settings.load_path = found.c_str();
  }

  Token periodic =
      extract_token(load.next ? load.next : dup, "periodic_func", " \t", "{", "}");
  if (periodic.found) {
    static std::string found = periodic.found;
    global_settings.periodic_function = found.c_str();
  }

  free(dup);
}
} // namespace lurk