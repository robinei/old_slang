

class Reader {
    Context *ctx;

    const char *text;
    uint32_t pos;
    SourceLoc loc;

    static const uint32_t SCRATCH_LEN = 1024;
    char scratch[SCRATCH_LEN];

public:
    Reader(Context *ctx) : ctx(ctx) {
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
                result = &TRUE;
            } else if (ch == 'f') {
                step();
                result = &FALSE;
            } else {
                read_error("expected #t or #f");
            }
        } else if (ch == '\'') {
            step();
            Any *form = read_form();
            result = cons(ctx, symbol(ctx, "quote"),
                        cons(ctx, form, &NIL));
        } else if (ch == '"') {
            step();
            result = read_string();
        } else if (is_alpha(ch) || ch == '_') {
            result = read_symbol();
        } else if (is_digit(ch)) {
            result = read_number();
        } else {
            read_error("expected an expression");
        }
        skip_space();
        if (peek() == '.') {
            step();
            skip_space();
            Any *sym = read_symbol();
            result = cons(ctx, symbol(ctx, "prop"),
                        cons(ctx, sym,
                            cons(ctx, result, &NIL)));
            skip_space();
        }
        if (peek() == '[') {
            step();
            Any *list = read_list(']');
            result = cons(ctx, result, list);
            skip_space();
        }
        if (peek() == ':') {
            step();
            Any *form = read_form();
            result = cons(ctx, symbol(ctx, "ascribe"),
                        cons(ctx, result,
                            cons(ctx, form, &NIL)));
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
        ctx->locations.put(result, orig_loc); 
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
            if (!is_alphanum(ch) && ch != '_') {
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
        return NULL;
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

    void read_error(const char *fmt, ...) {
        printf("line %d, col %d: ", loc.line + 1, loc.col + 1);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
        abort();
    }
};
