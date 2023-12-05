#ifndef PYMODULE_LIB_H
#define PYMODULE_LIB_H

#include "python3.8/Python.h"

int pymodule_lib_init();
int pymodule_lib_cleanup();
uint8_t* generate_histogram(const char *module_name, const char *function_name, const char *image_path,  const char *camera_set, size_t* buffer_size);

#endif // PYMODULE_LIB_H