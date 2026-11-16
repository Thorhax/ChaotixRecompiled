// Threads on the Nintendo Switch get a stack of 128 KiB unless they ask for
// more (libnx), where a desktop gives them a megabyte or more. The frontend's
// threads run a whole machine -- the menu's zone pictures, a time attack
// ghost -- and its reset alone puts more than that on the stack. std::thread
// has no way to ask, so every thread made without a size of its own is given
// a desktop-sized stack here: the linker sends pthread_create this way
// (-Wl,--wrap=pthread_create in CMakeLists.txt).
#include <pthread.h>

#include <cstddef>

extern "C" {

int __real_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*entry)(void*), void* arg);

int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*entry)(void*), void* arg) {
    constexpr size_t kStack = size_t(1) << 20;
    size_t asked = 0;
    if (attr && pthread_attr_getstacksize(attr, &asked) == 0 && asked >= kStack)
        return __real_pthread_create(thread, attr, entry, arg);
    pthread_attr_t roomy;
    if (attr) roomy = *attr;
    else if (pthread_attr_init(&roomy) != 0) return __real_pthread_create(thread, attr, entry, arg);
    pthread_attr_setstacksize(&roomy, kStack);
    const int result = __real_pthread_create(thread, &roomy, entry, arg);
    if (!attr) pthread_attr_destroy(&roomy);
    return result;
}

} // extern "C"
