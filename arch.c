
#if defined(__aarch64__)
    #if defined(__APPLE__)
        #error cmake_ARCH arm_apple
    #else
        #error cmake_ARCH arm
    #endif
#elif defined(__x86_64__)
    #error cmake_ARCH x86_64
#endif
#error cmake_ARCH unknown
