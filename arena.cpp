
class Arena {
public:
    Arena(size_t chunk_size = 65536)
    : chunk_size(chunk_size), curr(NULL), freelist(NULL) {}

    ~Arena() {
        while (curr) {
            Chunk *next = curr->next;
            free(curr);
            curr = next;
        }
        while (freelist) {
            Chunk *next = freelist->next;
            free(freelist);
            freelist = next;
        }
    }

    void reset() {
        while (curr) {
            Chunk *next = curr->next;
            curr->next = freelist;
            curr->pos = 0;
            freelist = curr;
            curr = next;
        }
    }

    char *alloc(size_t size) {
        if (!curr) {
            new_chunk();
        }
        const size_t alignment = sizeof(void *);
        size += (alignment - (size & (alignment - 1))) & (alignment - 1);
        assert(size <= chunk_size);
        if (curr->pos + size > chunk_size) {
            new_chunk();
        }
        char *result = curr->data + curr->pos;
        curr->pos += size;
        return result;
    }

    template<typename T>
    T *alloc() {
        return (T *)alloc(sizeof(T));
    }

private:
    Arena(const Arena &); // disallow
    Arena &operator=(const Arena &); // disallow

    struct Chunk {
        Chunk *next;
        size_t pos;
        char data[1];
    };

    const size_t chunk_size;
    Chunk *curr;
    Chunk *freelist;

    void new_chunk() {
        Chunk *chunk = freelist;
        if (chunk) {
            freelist = chunk->next;
        } else {
            chunk = (Chunk *)malloc(sizeof(Chunk) + chunk_size);
            chunk->pos = 0;
        }
        chunk->next = curr;
        curr = chunk;
    }
};
