#include "common.h"

static StatusCode __internal_error_codes = SUCCESS;

void SetStatusCode(StatusCode code) { __internal_error_codes = code; }

const char *GetStatusCode() {
  switch (__internal_error_codes) {
  case INVALID_FUNCTION_ARGUMENTS:
    return "Invalid function arguments provided.";
  case MEMORY_ALLOCATION_FAILURE:
    return "Memory alloc/realloc failure.";
  case ATTEMPTED_INVALID_ACCESS:
    return "Attempted invalid/out-of-bounds access.";
  case CANNOT_EXECUTE:
    return "Can not execute function due to internal conditions.";
  default:
    return "Unknown error code.";
  }
}
