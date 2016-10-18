
class Printer {
public:
    Printer(Context *ctx) : ctx(ctx), module(ctx->module) {}

    void print(Any *form) {
        last_line = 0;
        print_form(form);
    }

private:
    Context *ctx;
    Module *module;
    int last_line;
    
    void print_form(Any *form) {
        switch (form->type->type) {
        case TYPE_SYMBOL: {
            Ptr<Symbol> sym(form);
            printf("%s", sym->data);
            break;
        }
        case TYPE_STRING: {
            Ptr<String> str(form);
            // TODO: escape the output
            printf("\"%s\"", str->data);
            break;
        }
        case TYPE_CONS: {
            printf("(");
            while (form != &NIL) {
                Ptr<Cons> cons(form);

                // try to preserve whitespace when printing forms that were read
                // (for which we remember the locations of the car forms of all conses)
                SourceLoc loc;
                if (module->locations.get(cons, loc)) {
                    if (last_line < loc.line) {
                        do {
                            printf("\n");
                            ++last_line;
                        } while (last_line < loc.line);
                        for (int i = 0; i < loc.col; ++i) {
                            printf(" ");
                        }
                    }
                }

                print_form(cons->car);

                form = cons->cdr;
                if (form != &NIL) {
                    printf(" ");
                }
            }
            printf(")");
            break;
        }
        case TYPE_BOOL:
            if (form == &TRUE) {
                printf("#t");
            } else {
                printf("#f");
            }
            break;
        case TYPE_I8: {
            Ptr<i8> num(form);
            printf("%d", (i32)*num);
            break;
        }
        case TYPE_I16: {
            Ptr<i16> num(form);
            printf("%d", (i32)*num);
            break;
        }
        case TYPE_I32: {
            Ptr<i32> num(form);
            printf("%d", (i32)*num);
            break;
        }
        case TYPE_I64: {
            Ptr<i64> num(form);
            printf("%lld", (i64)*num);
            break;
        }
        case TYPE_U8: {
            Ptr<u8> num(form);
            printf("%u", (u32)*num);
            break;
        }
        case TYPE_U16: {
            Ptr<u16> num(form);
            printf("%u", (u32)*num);
            break;
        }
        case TYPE_U32: {
            Ptr<u32> num(form);
            printf("%u", (u32)*num);
            break;
        }
        case TYPE_U64: {
            Ptr<u64> num(form);
            printf("%llu", (u64)*num);
            break;
        }
        case TYPE_F32: {
            Ptr<f32> num(form);
            printf("%f", (f64)*num);
            break;
        }
        case TYPE_F64: {
            Ptr<f64> num(form);
            printf("%f", (f64)*num);
            break;
        }
        }
    }
};
