
#include <limits.h>
#include <ctype.h>
#include "strtoll.cpp"
#include "strtod.cpp"

class Reader {
    Context *ctx;
    Module *module;

    const char *text;
    uint32_t pos;
    SourceLoc loc;

    static const uint32_t SCRATCH_LEN = 1024;
    char scratch[SCRATCH_LEN];

public:
    Reader(Context *ctx) : ctx(ctx), module(ctx->module) {
    }

    Any *read_file(const char *text) {
        this->text = text;
        pos = 0;
        loc.line = 0;
        loc.col = 0;
        return read_list('\0');
    }

    Any *read_form() {
        Any *result = NULL;
        skip_space();
        char ch = peek();
        if (ch == '(') {
            step();
            result = read_list(')');
        } else if (ch == '#') {
            step();
            ch = peek();
            if (ch == 't') {
                step();
                expect_delim();
                result = &TRUE;
            } else if (ch == 'f') {
                step();
                expect_delim();
                result = &FALSE;
            } else {
                read_error("expected #t or #f");
            }
        } else if (ch == '\'') {
            step();
            Any *form = read_form();
            result = list(ctx, symbol(ctx, "quote"), form);
        } else if (ch == '"') {
            step();
            result = read_string();
        } else if (is_alpha(ch) || is_symchar(ch)) {
            result = read_symbol();
        } else if (is_digit(ch) || (ch == '+' || ch == '-') && is_digit(peek(1))) {
            result = read_number();
            expect_delim();
        } else {
            read_error("expected an expression");
        }
        while (true) {
            skip_space();
            ch = peek();
            if (ch == '.') {
                step();
                skip_space();
                Any *sym = read_symbol();
                result = list(ctx, symbol(ctx, "prop"), sym, result);
            } else if (ch == '[') {
                step();
                Any *list = read_list(']');
                result = cons(ctx, result, list);
            } else {
                break;
            }
        }
        if (ch == ':') {
            step();
            Any *typeform = read_form();
            result = list(ctx, symbol(ctx, "ascribe"), result, typeform);
        }
        return result;
    }

    Any *read_list(char end) {
        skip_space();
        if (peek() == end) {
            step();
            return &NIL;
        }

        SourceLoc orig_loc(loc);
        Any *form = read_form();
        Any *result = cons(ctx, form, read_list(end));
        // store location of all car forms, using the containing cons as key
        module->locations.put(result, orig_loc); 
        return result;
    }

    Any *read_string() {
        uint32_t len = 0;
        while (true) {
            char ch = peek();
            if (ch == '"') {
                step();
                if (len >= SCRATCH_LEN) {
                    read_error("string is too long");
                }
                scratch[len] = '\0';
                return string(ctx, scratch);
            } else if (ch == '\\') {
                step();
                ch = peek();
                if (ch == '\0') {
                    read_error("unexpected end of input while reading string");
                }
                if (len >= SCRATCH_LEN) {
                    read_error("string is too long");
                }
                switch (ch) {
                case '\'': scratch[len++] = '\''; break;
                case '"': scratch[len++] = '"'; break;
                case '?': scratch[len++] = '?'; break;
                case '\\': scratch[len++] = '\\'; break;
                case 'a': scratch[len++] = '\a'; break;
                case 'b': scratch[len++] = '\b'; break;
                case 'f': scratch[len++] = '\f'; break;
                case 'n': scratch[len++] = '\r'; break;
                case 'r': scratch[len++] = '\r'; break;
                case 't': scratch[len++] = '\t'; break;
                case 'v': scratch[len++] = '\v'; break;
                // TODO: handle \nnn \xnn \unnnn \Unnnnnnnn
                default: read_error("unexpected escape char: %c", ch);
                }
                step();
            } else if (ch == '\0') {
                read_error("unexpected end of input while reading string");
            } else {
                if (ch == '\r' || ch == '\n') {
                    spacestep();
                } else {
                    step();
                }
                if (len >= SCRATCH_LEN) {
                    read_error("string is too long");
                }
                scratch[len++] = ch;
            }
        }
        return NULL;
    }

    Any *read_symbol() {
        uint32_t len = 0;
        while (true) {
            char ch = peek();
            if (!is_alphanum(ch) && !is_symchar(ch)) {
                if (len == 0) {
                    read_error("expected a symbol");
                }
                if (len >= SCRATCH_LEN) {
                    read_error("string is too long");
                }
                scratch[len] = '\0';
                return symbol(ctx, scratch);
            }
            if (len >= SCRATCH_LEN) {
                read_error("string is too long");
            }
            scratch[len++] = ch;
            step();
        }
    }

    Any *read_number() {
        errno = 0;
        const char *end;
        const char *start = text + pos;
        i64 llval = my_strtoll(start, &end, 0);
        if (start == end) {
            read_error("error parsing number");
        }
        if (errno == ERANGE) {
            read_error("number too large");
        }
        if (*end != '.') {
            pos += (int)(end - start);
            return box(ctx, llval);
        }
        start = text + pos;
        f64 dval = my_strtod(start, &end);
        if (start == end) {
            read_error("error parsing number");
        }
        if (errno == ERANGE) {
            read_error("number too large");
        }
        pos += (int)(end - start);
        return box(ctx, dval);
    }

    inline char peek(int offset = 0) {
        return text[pos + offset];
    }

    inline void step() {
        ++loc.col;
        ++pos;
    }

    inline void spacestep() {
        if (text[pos] == '\r') {
            if (text[pos + 1] != '\n') {
                ++loc.line;
                loc.col = 0;
                ++pos;
                return;
            }
        } else if (text[pos] == '\n') {
            ++loc.line;
            loc.col = 0;
            ++pos;
            return;
        }
        ++loc.col;
        ++pos;
    }

    void skip_space() {
        char ch;
        while (true) {
            switch (peek()) {
            case ' ':
            case '\t':
            case '\f':
            case '\v':
            case '\r':
            case '\n':
                spacestep();
                continue;
            case ';': // line comment
                do {
                    spacestep();
                    ch = peek();
                } while (ch && ch != '\n' && ch != '\r');
                continue;
            default:
                return;
            }
        }
    }

    inline bool is_alpha(char ch) { return is_upper(ch) || is_lower(ch); }
    inline bool is_upper(char ch) { return ch >= 'A' && ch <= 'Z'; }
    inline bool is_lower(char ch) { return ch >= 'a' && ch <= 'z'; }
    inline bool is_digit(char ch) { return ch >= '0' && ch <= '9'; }
    inline bool is_alphanum(char ch) { return is_alpha(ch) || is_digit(ch); }
    inline bool is_symchar(char ch) {
        switch (ch) {
        case '_':
        case '-':
        case '=':
        case '+':
        case '*':
        case '/':
        case '?':
        case '!':
        case '&':
        case '%':
        case '^':
        case '~':
            return true;
        default:
            return false;
        }
    }

    void expect_delim() {
        switch (peek()) {
        case ' ':
        case '\t':
        case '\f':
        case '\v':
        case '\r':
        case '\n':
        case '.':
        case ':':
        case '(':
        case ')':
        case '[':
        case ']':
            return;
        }
        read_error("expected delimiter after expression");
    }

    void read_error(const char *fmt, ...) {
        printf("line %d, col %d: ", loc.line + 1, loc.col + 1);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
        exit(1);
    }
};
