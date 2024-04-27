#include <iso_file.h>

int check_string_ending(const char *str, const char *end) {
  const char *_str = str;
  const char *_end = end;

  while (*str != 0) {
    str++;
  }
  str--;

  while (*end != 0) {
    end++;
  }
  end--;

  while (1) {
    if (*str != *end) {
      return 0;
    }

    str--;
    end--;

    if (end == _end || (str == _str && end == _end)) {
      return 1;
    }

    if (str == _str) {
      return 0;
    }
  }

  return 0;
}

/*
    Expects the file name to be passed in with the limine module request
    associated with that file
*/
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file) {
  LIMINE_MODULE_RESP *module_response = module_request.response;
  for (size_t i = 0; i < module_response->module_count; i++) {
    LIMINE_FILE *f = module_response->modules[i];
    if (check_string_ending(f->path, name)) {
      *file = f;
      return;
    }
  }
  *file = NULL;
}