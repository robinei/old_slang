#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <libtcc.h>

void fatal_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    exit(1);
}

#include "stdstub.cpp"
#include "murmur3.cpp"
#include "functors.cpp"
#include "hashtable.cpp"
#include "arena.cpp"
#include "types.cpp"


struct SourceLoc {
    uint32_t line;
    uint32_t col;

    SourceLoc() : line(0), col(0) {}
    SourceLoc(uint32_t line, uint32_t col) : line(line), col(col) {}
};

typedef HashTable<const char *, Box<Symbol> *> SymbolTable;
typedef HashTable<Any *, SourceLoc> LocationTable;

class Module;

struct Context {
    Arena *arena;
    SymbolTable symbols;
    LocationTable locations;
    Module *module;
};



template<typename T>
inline Ptr<T> box(Context *ctx, T value) {
    Box<T> *box = ctx->arena->alloc< Box<T> >();
    box->type = get_type(Traits<T>::type);
    box->value = value;
    return box;
}

Ptr<Symbol> symbol(Context *ctx, const char *name) {
    Box<Symbol> *sym;
    if (ctx->symbols.get(name, sym)) {
        return sym;
    }

    size_t name_len = strlen(name);
    char *name_copy = ctx->arena->alloc(name_len + 1);
    memcpy(name_copy, name, name_len + 1);

    sym = box(ctx, Symbol((int)name_len, name_copy));
    ctx->symbols.put(name_copy, sym);
    return sym;
}

Ptr<String> string(Context *ctx, const char *text) {
    size_t text_len = strlen(text);
    char *text_copy = ctx->arena->alloc(text_len + 1);
    memcpy(text_copy, text, text_len + 1);
    return box(ctx, String((int)text_len, text_copy));
}

inline Ptr<Cons> cons(Context *ctx, Any *car, Any *cdr) {
    return box(ctx, Cons(car, cdr));
}
inline Ptr<Cons> list(Context *ctx, Any *a) {
    return cons(ctx, a, &NIL);
}
inline Ptr<Cons> list(Context *ctx, Any *a, Any *b) {
    return cons(ctx, a, cons(ctx, b, &NIL));
}
inline Ptr<Cons> list(Context *ctx, Any *a, Any *b, Any *c) {
    return cons(ctx, a, cons(ctx, b, cons(ctx, c, &NIL)));
}
inline Ptr<Cons> list(Context *ctx, Any *a, Any *b, Any *c, Any *d) {
    return cons(ctx, a, cons(ctx, b, cons(ctx, c, cons(ctx, d, &NIL))));
}

inline bool typep(Any *form) {
    return form->type->type == TYPE_TYPE;
}
inline bool consp(Any *form) {
    return form->type->type == TYPE_CONS;
}
inline bool nilp(Any *form) {
    return form == &NIL;
}
inline bool symbolp(Any *form) {
    return form->type->type == TYPE_SYMBOL;
}
inline Any *car(Any *form) {
    return Ptr<Cons>(form)->car;
}
inline Any *cdr(Any *form) {
    return Ptr<Cons>(form)->cdr;
}
inline Any *cadr(Any *form) {
    return car(cdr(form));
}

#include "reader.cpp"
#include "printer.cpp"


class Module {
public:
};


class Parser {
    Context *ctx;
    Module *module;

public:
    Parser(Context *ctx) : ctx(ctx) {

    }

    void parse_module(Any *list) {
        module = new Module;

        while (list != &NIL) {
            Any *form = car(list);
            if (!consp(form)) {
                parse_error(list, "only list forms are allowed at top-level");
            }

            Any *sym = car(form);
            if (!symbolp(sym)) {
                parse_error(form, "expected a top-level definition");
            }

            if (sym == symbol(ctx, "def")) {
                parse_func(form);
            } else {
                parse_error(form, "expected a top-level definition");
            }

            list = cdr(list);
        }
    }

    void parse_func(Any *form) {
        assert(car(form) == symbol(ctx, "def"));
        form = cdr(form);

        Any *generic_arglist = NULL;
        Any *nameform = car(form); 
        if (consp(nameform)) {
            generic_arglist = cdr(nameform);
            if (!symbolp(car(nameform))) {
                parse_error(nameform, "expected function name");
            }
            nameform = car(nameform);
        } else if (!symbolp(nameform)) {
            parse_error(form, "expected function name");
        }

        form = cdr(form);
        Any *arglist = car(form);
        Any *rettype = NULL;
        if (!consp(arglist)) {
            parse_error(form, "expected argument list");
        }
        if (!nilp(arglist)) {
            if (car(arglist) == symbol(ctx, "ascribe")) {
                Any *ascribeform = cdr(arglist);
                arglist = car(ascribeform);
                if (!consp(arglist)) {
                    parse_error(ascribeform, "expected argument list");
                }
                ascribeform = cdr(ascribeform);
                rettype = car(ascribeform);
            }
        }
    }

    void parse_error(Any *containing_cons, const char *fmt, ...) {
        SourceLoc loc;
        if (ctx->locations.get(containing_cons, loc)) {
            printf("line %d, col %d: ", loc.line + 1, loc.col + 1);
        } else {
            printf("unknown location: ");
        }
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
        exit(1);
    }
};




int main(int argc, char *argv[]) {
    HashTable<const char *, int> hashtable;

    hashtable.put("foo", 99);
    hashtable.put("bar", 435);
    
    int val;
    bool found;
    found = hashtable.get("foo", val);
    assert(found);
    assert(val == 99);
    found = hashtable.get("bar", val);
    assert(found);
    assert(val == 435);
    found = hashtable.get("baz", val);
    assert(!found);
    hashtable.remove("bar");
    found = hashtable.get("bar", val);
    assert(!found);
    found = hashtable.get("foo", val);
    assert(found);

    Arena arena;
    Context ctx;
    ctx.arena = &arena;

    assert(symbol(&ctx, "foo") == symbol(&ctx, "foo"));
    assert(symbol(&ctx, "bar") == symbol(&ctx, "bar"));
    assert(symbol(&ctx, "foo") != symbol(&ctx, "bar"));

    Reader reader(&ctx);
    Any *form = reader.read_file(
        "(def foo () #f)\n"
        "(def bar[T] (x:T):bool\n"
        "    x.is_true)\n"
    );
    assert(form);

    Printer printer(&ctx);
    printer.print(form);
    printf("\n");

    Parser parser(&ctx);
    parser.parse_module(form);





    const char *c_program =
    "int testfunc1(int x, int y) { return x + y; }\n"; 

    TCCState *s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    //tcc_set_lib_path(s, "/usr/lib/tcc");

    if (tcc_compile_string(s, c_program) < 0) {
        fprintf(stderr, "Error compiling C program\n");
        exit(1);
    }

    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0) {
        fprintf(stderr, "Error relocating code\n");
        exit(1);
    }

    typedef int (*TestFunc)(int, int);
    TestFunc func = (TestFunc)tcc_get_symbol(s, "testfunc1");
    if (!func) {
        fprintf(stderr, "Error getting function pointer\n");
        exit(1);
    }

    printf("func result: %d\n", func(100, 78));

    tcc_delete(s);

    return 0;
}
