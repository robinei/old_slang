
template<typename T>
struct Equal {
    bool operator()(T a, T b) {
        return a == b;
    }
};

template<>
struct Equal<const char *> {
    bool operator()(const char *a, const char *b) {
        return strcmp(a, b) == 0;
    }
};

template<typename T>
struct Hash {

};

template<>
struct Hash<const char *> {
    uint32_t operator()(const char *key) {
        uint32_t hash;
        MurmurHash3_x86_32(key, strlen(key), 0, &hash);
        return hash;
    }
};

template<typename T>
struct Hash<T *> {
    uint32_t operator()(const T *ptr) const {
        uint32_t val = (uint32_t)(intptr_t)ptr;
        val = ~val + (val << 15);
        val = val ^ (val >> 12);
        val = val + (val << 2);
        val = val ^ (val >> 4);
        val = val * 2057;
        val = val ^ (val >> 16);
        return val;
    }
};
