#pragma once

#ifdef __cplusplus
    #define export exports
    extern "C" {
        #include "qbe/all.h"
    }
    #undef export
#else
    #include <qbe/all.h>
#endif