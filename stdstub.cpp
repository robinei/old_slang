
#include <unistd.h>  // for write(), also available on Windows

inline void *emulate_cc_new(size_t len) {
    void *p = malloc(len);
    if (p == 0) {
        // Don't use stdio (e.g. fputs), because that may want to allocate more
        // memory.
        write(2, "out of memory\n", 14);
        abort();
    }
    return p;
}

inline void emulate_cc_delete(void* p) {
    if (p != 0) {
        free(p);
    }
}

void *operator new  (size_t len) { return emulate_cc_new(len); }
void *operator new[](size_t len) { return emulate_cc_new(len); }
void  operator delete  (void *p) { return emulate_cc_delete(p); }
void  operator delete[](void *p) { return emulate_cc_delete(p); }

void *__cxa_pure_virtual = 0;
