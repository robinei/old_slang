
enum {
    TYPE_UNKNOWN,

    TYPE_TYPE,

    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,

    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,

    TYPE_F32,
    TYPE_F64,

    TYPE_BOOL,
    TYPE_SYMBOL,
    TYPE_STRING,
    TYPE_CONS,
    TYPE_STRUCT,

    NUM_TYPES
};

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

struct Type;

struct Any {
    Type *type;
    Any(Type *type) : type(type) {}
};

template<typename T>
struct Box : public Any {
    T value;
    Box(Type *type, T value) : Any(type), value(value) {}
};



struct Unknown { };

struct Symbol {
    int length;
    char *data;
    Symbol(int length, char *data) : length(length), data(data) {}
};

struct String {
    int length;
    int capacity;
    char *data;
    String(int length, char *data) : length(length), capacity(length), data(data) {}
};

struct Cons {
    Any *car;
    Any *cdr;
    Cons(Any *car, Any *cdr) : car(car), cdr(cdr) {}
};



struct Type {
    int type;
    int size;
    Box<Symbol> *name;
    
    Type(int type, int size) : type(type), size(size) {}
};

template<class T> struct Traits {
    static const int type = TYPE_UNKNOWN; 
};

#define DEF_TYPE(TYPE, CODE) \
Box<Type> type_##TYPE(&type_Type.value, Type(CODE, sizeof(TYPE))); \
template <> struct Traits<TYPE> { \
    static const int type = CODE; \
}

DEF_TYPE(Type, TYPE_TYPE);
DEF_TYPE(Unknown, TYPE_UNKNOWN);

DEF_TYPE(i8, TYPE_I8);
DEF_TYPE(i16, TYPE_I16);
DEF_TYPE(i32, TYPE_I32);
DEF_TYPE(i64, TYPE_I64);

DEF_TYPE(u8, TYPE_U8);
DEF_TYPE(u16, TYPE_U16);
DEF_TYPE(u32, TYPE_U32);
DEF_TYPE(u64, TYPE_U64);

DEF_TYPE(f32, TYPE_F32);
DEF_TYPE(f64, TYPE_F64);

DEF_TYPE(bool, TYPE_BOOL);
DEF_TYPE(Symbol, TYPE_SYMBOL);
DEF_TYPE(String, TYPE_STRING);
DEF_TYPE(Cons, TYPE_CONS);


Box<Cons> NIL(&type_Cons.value, Cons(NULL, NULL));
Box<bool> TRUE(&type_bool.value, true);
Box<bool> FALSE(&type_bool.value, false);


template<typename T>
class Ptr {
    Box<T> *box;
public:
    Ptr() : box(NULL) {}

    Ptr(Box<T> *box) : box(box) {}

    Ptr(Any *any) {
        assert(Traits<T>::type == any->type->type);
        box = (Box<T>*)any;
    }

    T &operator*() { return box->value; }
    T *operator->() { return &box->value; }

    operator T() const { return box->value; }
    operator T*() { return &box->value; }
    operator Box<T>*() { return box; }

    bool operator==(Ptr other) const { return box == other.box; }
    bool operator!=(Ptr other) const { return box != other.box; }
};


Ptr<Type> get_type(int id) {
    switch (id) {
        case TYPE_TYPE: return &type_Type;
        case TYPE_I8: return &type_i8;
        case TYPE_I16: return &type_i16;
        case TYPE_I32: return &type_i32;
        case TYPE_I64: return &type_i64;
        case TYPE_U8: return &type_u8;
        case TYPE_U16: return &type_u16;
        case TYPE_U32: return &type_u32;
        case TYPE_U64: return &type_u64;
        case TYPE_F32: return &type_f32;
        case TYPE_F64: return &type_f64;
        case TYPE_BOOL: return &type_bool;
        case TYPE_SYMBOL: return &type_Symbol;
        case TYPE_STRING: return &type_String;
        case TYPE_CONS: return &type_Cons;
        default: return &type_Unknown;
    }
}

