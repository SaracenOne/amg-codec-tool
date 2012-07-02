/*
  This file is a part of the
  Amagami SS PS2 Translation Project

  You may feel free to use this code if you want so.
  I provide no guarantee that this code is working,
  useful or anything else. Use it at your own risk.
*/

#include "module.h"
#include <stdio.h>
#include <stdlib.h>

#include <QString>

Module::Module()
{
    qstrncpy(moduleName, "Undefined", qstrlen("Undefined"));
}

Module::Module(const char * _moduleName)
{
    size_t lengh = strlen(_moduleName);
    qstrncpy(moduleName,
             _moduleName,
             (lengh > MAX_MODULE_NAME_LENGTH) ? (MAX_MODULE_NAME_LENGTH) : (lengh));
}

void Module::fatalExit(const char *reason)
{
    fprintf(stderr, "\nModule [%s] operation abort! Reason: [%s]\n", moduleName, reason);
    exit(1);
}