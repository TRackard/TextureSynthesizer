//contains the basic libs that all the files will depend on
//also some generally helpful macros to keep things consistent

#ifndef DEFINES_H
#define DEFINES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define bool uint8_t
#define FALSE (0 != 0)
#define TRUE !FALSE

#endif //DEFINES_H
