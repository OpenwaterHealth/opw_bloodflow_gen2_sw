#include "pymodule_lib.h"
#include <stdio.h>

#define IMAGE_BUFFER_SIZE 262144
uint8_t image_buffer[IMAGE_BUFFER_SIZE];

int pymodule_lib_init() {
    // Initialize the Python interpreter
    printf("Initializing Python\n");
    Py_Initialize();    
    return 0;
}

int pymodule_lib_cleanup() {
    // Clean up the Python interpreter
    printf("Cleaning up Python\n");
    Py_Finalize();
    return 0;
}

uint8_t* generate_histogram(const char *module_name, const char *function_name, const char *image_path,  const char *camera_set, size_t* buffer_size) {

    // Import the module containing the histogram function
    printf("Importing module %s\n", module_name);
    PyObject *module = PyImport_ImportModule(module_name);
    if (module == NULL) {
        printf("Failed to import module\n");
        return NULL;
    }

    // Get the function object for the histogram function
    printf("Finding function %s\n", function_name);
    PyObject *function = PyObject_GetAttrString(module, function_name);
    if (function == NULL || !PyCallable_Check(function)) {
        printf("Failed to find function\n");
        return NULL;
    }

    // Create the argument tuple (in this case, a single string)
    printf("Opening image %s\n", image_path);
    PyObject *args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, PyUnicode_FromString(image_path));
    PyTuple_SetItem(args, 1, PyUnicode_FromString(camera_set));

    // Call the histogram function with the argument tuple
    printf("Calling histogram function\n");
    PyObject *result = PyObject_CallObject(function, args);
    if (result == NULL) {
        printf("Failed to call function\n");
        return NULL;
    }

    // Convert the result to a bytes object
    printf("Converting result to bytes object\n");
    PyObject *bytes = PyBytes_FromObject(result);
    if (bytes == NULL) {
        printf("Failed to convert result\n");
        return NULL;
    }

    // Create a buffer from the bytes object
    printf("Creating buffer from bytes object\n");
    Py_buffer buffer;
    int ret = PyObject_GetBuffer(bytes, &buffer, PyBUF_SIMPLE);
    if (ret != 0) {
        printf("Failed to create buffer\n");
        return NULL;
    }

    // Copy the buffer contents to the static array after setting it to zero
    printf("Copying buffer contents to byte array\n");
    if (buffer.len > IMAGE_BUFFER_SIZE) {
        printf("Error: buffer too large buffer size %ld\n", buffer.len);
        PyBuffer_Release(&buffer);
        Py_DECREF(bytes);
        Py_DECREF(result);
        Py_DECREF(args);
        Py_DECREF(function);
        Py_DECREF(module);
        Py_Finalize();
        return NULL;
    }
    printf("Copying buffer contents to byte array\n");
    memset(image_buffer, 0, sizeof(image_buffer));
    memcpy(image_buffer, buffer.buf, buffer.len);

    // Set the buffer size output parameter
    *buffer_size = buffer.len;

    // Release the buffer
    printf("Releasing buffer\n");
    PyBuffer_Release(&buffer);

    // Decrement the reference count for the bytes object
    Py_DECREF(bytes);

    // Decrement the reference count for the result object
    Py_DECREF(result);

    // Decrement the reference count for the argument tuple
    Py_DECREF(args);

    // Decrement the reference count for the function object
    Py_DECREF(function);

    // Decrement the reference count for the module object
    Py_DECREF(module);

    return image_buffer;
}
