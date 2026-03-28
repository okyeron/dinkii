#include "debug.h"

void debug(int level, const char *message) {
  if (level < DEBUG_LEVEL)
    return;
  printf("%s", message);
}

void debugln(int level, const char *message) {
  if (level < DEBUG_LEVEL)
    return;
  printf("%s\n", message);
}

void debugln(int level) {
  if (level < DEBUG_LEVEL)
    return;
  printf("\n");
}

void debugf(int level, const char *message, ...) {
  if (level < DEBUG_LEVEL)
    return;

  va_list ap;
  va_start(ap, message);
  vprintf(message, ap);
  va_end(ap);
}

void debugfln(int level, const char *message, ...) {
  if (level < DEBUG_LEVEL)
    return;

  va_list ap;
  va_start(ap, message);
  char buffer[200] = {0};
  vsnprintf(buffer, 200, message, ap);
  va_end(ap);

  printf("%s\n", buffer);
}
