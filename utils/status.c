#include "status.h"
#include <pthread.h>
#include <stdarg.h>

static const char *StatusToStr(StatusCode code);

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *StatusToStr(StatusCode code) {
  switch (code) {
  case SUCCESS:
    return "SUCCESS";
  case FAILURE:
    return "FAILURE";
  case NULL_EXCEPTION:
    return "NULL EXCEPTION";
  case CREATION_FAILURE:
    return "CREATION FAILURE";
  case OUT_OF_BOUNDS_ACCESS:
    return "OUT OF BOUNDS ACCESS";
  case USE_AFTER_FREE:
    return "USE AFTER FREE";
  default:
    return "UNKNOWN STATUS CODE";
  }
}

void status_Log(StatusCode code, const char *file_name, const char *func_name,
                i32 line_num, const char *fmt, ...) {

  IF_NULL(file_name) { file_name = "UnknownFile"; }
  IF_NULL(func_name) { func_name = "UnknownFunc"; }

  va_list args;
  va_start(args, fmt);
  char buffer[512];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  char log[1536]; // 1536 + 512 = 2048
  snprintf(log, sizeof(log), "[%s] %s:%d (%s): %s\n", StatusToStr(code),
           file_name, line_num, func_name, buffer);

  pthread_mutex_lock(&log_mutex);
  fputs(log, stdout);
  fflush(stdout);
  pthread_mutex_unlock(&log_mutex);
}
