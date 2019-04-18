#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <Python.h> // https://www.linuxjournal.com/article/8497

const char *file_extension[] = {"jpg","png","gif"};

void error(char *msg) {
    perror(msg);
    exit(1);
}

bool is_image(char *header); 
// ee
bool is_safe_image(char *url); 
// bool is_safe_image(char *filename);

void exec_pycode(const char* code) {
    Py_Initialize();
    PyRun_SimpleString(code);
    Py_Finalize();
}



