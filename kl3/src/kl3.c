#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

// Error handling
typedef enum {
    ERR_NONE,
    ERR_SYNTAX,
    ERR_UNBOUND_SYMBOL,
    ERR_TYPE_MISMATCH,
    ERR_INVALID_ARGUMENT,
    ERR_OUT_OF_MEMORY
} ErrorType;

// Global error state
ErrorType error_type = ERR_NONE;
char error_message[256];
jmp_buf error_jmp_buf;
int _debug = 0;

// Cell type definitions
typedef enum {
    CELL_ATOM,
    CELL_NUMBER,
    CELL_PAIR,
    CELL_FUNCTION,
    CELL_SPECIAL    // For special forms like QUOTE, LAMBDA, etc.
} CellType;

typedef struct Cell {
    CellType type;
    union {
        char *atom;          // For atoms
        int number;          // For numbers (16-bit integers)
        struct {
            struct Cell *car;
            struct Cell *cdr;
        } pair;              // For pairs
        struct Cell* (*func)(struct Cell*);  // For built-in functions
    } value;
} Cell;

// String reader structure
typedef struct {
    const char *input;
    size_t pos;
    size_t len;
} StringReader;

// Global values
Cell *NIL;
Cell *T;
Cell *QUOTE_SYM;
Cell *LAMBDA_SYM;
Cell *COND_SYM;
Cell *LABEL_SYM;
Cell *DEFINE_SYM;

// Environment - a simple association list
Cell *env = NULL;

// Forward declarations
Cell *cons(Cell *car, Cell *cdr);
Cell *car(Cell *cell);
Cell *cdr(Cell *cell);
Cell *atom(Cell *cell);
Cell *debug(Cell *cell);
Cell *eq(Cell *a, Cell *b);
Cell *make_atom(const char *name);
Cell *make_number(int n);
Cell *eval(Cell *expr, Cell *env);
Cell *apply(Cell *fn, Cell *args, Cell *env);
Cell *read_expr(StringReader *reader);
void print_expr(Cell *expr);
void init_lisp();
void cleanup_lisp();
Cell *bind(Cell *sym, Cell *val, Cell *env);
Cell *define(Cell *sym, Cell *val);
Cell *lookup(Cell *sym, Cell *env);
Cell *list_of_values(Cell *list, Cell *env);

// Arithmetic functions
Cell *add(Cell *args);
Cell *sub(Cell *args);
Cell *mul(Cell *args);
Cell *div_func(Cell *args);
Cell *sqrt_func(Cell *args);

// Comparison functions
Cell *lt_func(Cell *args);  // Less than
Cell *lte_func(Cell *args); // Less than or equal
Cell *gt_func(Cell *args);  // Greater than
Cell *gte_func(Cell *args); // Greater than or equal

// StringReader functions
StringReader create_string_reader(const char *input) {
    StringReader reader;
    reader.input = input;
    reader.pos = 0;
    reader.len = strlen(input);
    return reader;
}

int reader_getc(StringReader *reader) {
    if (reader->pos >= reader->len) {
        return EOF;
    }
    return reader->input[reader->pos++];
}

void reader_ungetc(StringReader *reader) {
    if (reader->pos > 0) {
        reader->pos--;
    }
}

// Error handling
void set_error(ErrorType type, const char *message) {
    error_type = type;
    strncpy(error_message, message, sizeof(error_message) - 1);
    error_message[sizeof(error_message) - 1] = '\0';
    longjmp(error_jmp_buf, 1);
}

void clear_error() {
    error_type = ERR_NONE;
    error_message[0] = '\0';
}

// Initialize LISP environment
void init_lisp() {
    // Create NIL as a special atom
    NIL = (Cell*)malloc(sizeof(Cell));
    if (!NIL) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate NIL");
    NIL->type = CELL_ATOM;
    NIL->value.atom = strdup("NIL");

    // Create T as a special atom
    T = (Cell*)malloc(sizeof(Cell));
    if (!T) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate T");
    T->type = CELL_ATOM;
    T->value.atom = strdup("T");

    // Create special symbols
    QUOTE_SYM = make_atom("QUOTE");
    LAMBDA_SYM = make_atom("LAMBDA");
    COND_SYM = make_atom("COND");
    LABEL_SYM = make_atom("LABEL");
    DEFINE_SYM = make_atom("DEFINE");

    // Initialize environment with built-in functions
    env = NIL;
    env = bind(make_atom("T"), T, env);   // Add this line
    env = bind(make_atom("NIL"), NIL, env); // Also good to add NIL
    //maybe?
    env = bind(make_atom("QUOTE"), QUOTE_SYM, env);
    env = bind(make_atom("LAMBDA"), LAMBDA_SYM, env);
    env = bind(make_atom("COND"), COND_SYM, env);
    env = bind(make_atom("LABEL"), LABEL_SYM, env);
    env = bind(make_atom("DEFINE"), DEFINE_SYM, env);
    //maybe?
    env = bind(make_atom("CONS"), make_atom("CONS"), env);
    env = bind(make_atom("CAR"), make_atom("CAR"), env);
    env = bind(make_atom("CDR"), make_atom("CDR"), env);
    env = bind(make_atom("ATOM"), make_atom("ATOM"), env);
    env = bind(make_atom("EQ"), make_atom("EQ"), env);
    env = bind(make_atom("DEBUG"), make_atom("DEBUG"), env);

    // Register arithmetic functions
    env = bind(make_atom("ADD"), make_atom("ADD"), env);
    env = bind(make_atom("SUB"), make_atom("SUB"), env);
    env = bind(make_atom("MUL"), make_atom("MUL"), env);
    env = bind(make_atom("DIV"), make_atom("DIV"), env);
    env = bind(make_atom("SQRT"), make_atom("SQRT"), env);

    // Register comparison functions
    env = bind(make_atom("LT"), make_atom("LT"), env);   // Less than
    env = bind(make_atom("LTE"), make_atom("LTE"), env); // Less than or equal
    env = bind(make_atom("GT"), make_atom("GT"), env);   // Greater than
    env = bind(make_atom("GTE"), make_atom("GTE"), env); // Greater than or equal
}

// Clean up LISP environment (basic - not handling all memory)
void cleanup_lisp() {
    free(NIL->value.atom);
    free(NIL);
    free(T->value.atom);
    free(T);
    // TODO: Free other memory
}

// CONS: Create a new pair
Cell *cons(Cell *car_val, Cell *cdr_val) {
    Cell *cell = (Cell*)malloc(sizeof(Cell));
    if (!cell) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate cons cell");

    cell->type = CELL_PAIR;
    cell->value.pair.car = car_val;
    cell->value.pair.cdr = cdr_val;
    return cell;
}

// CAR: Get the first element of a pair
Cell *car(Cell *cell) {
    // Safety check for NULL pointer
    if (cell == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "CAR: NULL pointer");
        return NIL;
    }

    if (cell == NIL) {
        return NIL;
    }

    if (cell->type != CELL_PAIR) {
        set_error(ERR_TYPE_MISMATCH, "CAR: Expected a pair");
        return NIL;
    }

    // Safety check for NULL car
    if (cell->value.pair.car == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "CAR: NULL car pointer");
        return NIL;
    }

    return cell->value.pair.car;
}

// CDR: Get the rest of a pair
Cell *cdr(Cell *cell) {
    // Safety check for NULL pointer
    if (cell == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "CDR: NULL pointer");
        return NIL;
    }

    if (cell == NIL) {
        return NIL;
    }

    if (cell->type != CELL_PAIR) {
        set_error(ERR_TYPE_MISMATCH, "CDR: Expected a pair");
        return NIL;
    }

    // Safety check for NULL cdr
    if (cell->value.pair.cdr == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "CDR: NULL cdr pointer");
        return NIL;
    }

    return cell->value.pair.cdr;
}

// ATOM: Test if a cell is an atom
Cell *atom(Cell *cell) {
    if (cell == NIL || cell->type == CELL_ATOM) {
        return T;
    }
    return NIL;
}

// DEBUG: Test if a cell is an atom
Cell *debug(Cell *args) {
    // Check if we have at least one argument
    if (args == NIL) {
        if (_debug == 0)
            return NIL;
        else return T;
    }

    Cell *current = args;

    while (current != NIL) {
        Cell *arg = car(current);

        // Make sure the argument is a number
        if (arg->type != CELL_NUMBER) {
            set_error(ERR_TYPE_MISMATCH, "DEBUG requires numeric arguments (1==On, 0==Off)");
            return NIL;
        }

        _debug = arg->value.number;
        current = cdr(current);
    }

    if (_debug == 0)
        return NIL;
    else return T;

}

// EQ: Test if two cells are equal
Cell *eq(Cell *a, Cell *b) {
    // Safety checks
    if (a == NULL || b == NULL) {
        return NIL;
    }

    // If both are the same object, they're equal
    if (a == b) {
        return T;
    }

    // If both are atoms, compare their names
    if (a != NIL && b != NIL && a->type == CELL_ATOM && b->type == CELL_ATOM) {
        if (strcmp(a->value.atom, b->value.atom) == 0) {
            return T;
        }
    }

    // If both are numbers, compare their values
    if (a != NIL && b != NIL && a->type == CELL_NUMBER && b->type == CELL_NUMBER) {
        if (a->value.number == b->value.number) {
            return T;
        }
    }

    return NIL;
}

// Create a new atom with the given name
Cell *make_atom(const char *name) {
    // Check for NIL
    if (strcmp(name, "NIL") == 0) {
        return NIL;
    }

    // Check for T
    if (strcmp(name, "T") == 0) {
        return T;
    }

    // Create a new atom
    Cell *atom = (Cell*)malloc(sizeof(Cell));
    if (!atom) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate atom");

    atom->type = CELL_ATOM;
    atom->value.atom = strdup(name);
    if (!atom->value.atom) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate atom name");

    return atom;
}

// Create a new number cell
Cell *make_number(int n) {
    Cell *cell = (Cell*)malloc(sizeof(Cell));
    if (!cell) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate number");

    cell->type = CELL_NUMBER;
    cell->value.number = n;

    return cell;
}

// Arithmetic functions
Cell *add(Cell *args) {
    // Check if we have at least one argument
    if (args == NIL) {
        return make_number(0); // Adding nothing gives 0
    }

    int result = 0;
    Cell *current = args;

    while (current != NIL) {
        Cell *arg = car(current);

        // Make sure the argument is a number
        if (arg->type != CELL_NUMBER) {
            set_error(ERR_TYPE_MISMATCH, "ADD requires numeric arguments");
            return NIL;
        }

        result += arg->value.number;
        current = cdr(current);
    }

    return make_number(result);
}

Cell *sub(Cell *args) {
    // Check if we have at least one argument
    if (args == NIL) {
        set_error(ERR_INVALID_ARGUMENT, "SUB requires at least one argument");
        return NIL;
    }

    Cell *first = car(args);
    // Make sure the first argument is a number
    if (first->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "SUB requires numeric arguments");
        return NIL;
    }

    int result = first->value.number;
    Cell *rest = cdr(args);

    // If only one argument, negate it
    if (rest == NIL) {
        return make_number(-result);
    }

    // Otherwise, subtract all remaining arguments
    while (rest != NIL) {
        Cell *arg = car(rest);

        // Make sure the argument is a number
        if (arg->type != CELL_NUMBER) {
            set_error(ERR_TYPE_MISMATCH, "SUB requires numeric arguments");
            return NIL;
        }

        result -= arg->value.number;
        rest = cdr(rest);
    }

    return make_number(result);
}

Cell *mul(Cell *args) {
    // Check if we have at least one argument
    if (args == NIL) {
        return make_number(1); // Multiplying nothing gives 1
    }

    int result = 1;
    Cell *current = args;

    while (current != NIL) {
        Cell *arg = car(current);

        // Make sure the argument is a number
        if (arg->type != CELL_NUMBER) {
            set_error(ERR_TYPE_MISMATCH, "MUL requires numeric arguments");
            return NIL;
        }

        result *= arg->value.number;
        current = cdr(current);
    }

    return make_number(result);
}

Cell *div_func(Cell *args) {
    // Check if we have at least one argument
    if (args == NIL) {
        set_error(ERR_INVALID_ARGUMENT, "DIV requires at least one argument");
        return NIL;
    }

    Cell *first = car(args);
    // Make sure the first argument is a number
    if (first->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "DIV requires numeric arguments");
        return NIL;
    }

    int result = first->value.number;
    Cell *rest = cdr(args);

    // If only one argument, return 1/n
    if (rest == NIL) {
        if (result == 0) {
            set_error(ERR_INVALID_ARGUMENT, "DIV: Division by zero");
            return NIL;
        }
        return make_number(1 / result);
    }

    // Otherwise, divide by all remaining arguments
    while (rest != NIL) {
        Cell *arg = car(rest);

        // Make sure the argument is a number
        if (arg->type != CELL_NUMBER) {
            set_error(ERR_TYPE_MISMATCH, "DIV requires numeric arguments");
            return NIL;
        }

        if (arg->value.number == 0) {
            set_error(ERR_INVALID_ARGUMENT, "DIV: Division by zero");
            return NIL;
        }

        result /= arg->value.number;
        rest = cdr(rest);
    }

    return make_number(result);
}

Cell *sqrt_func(Cell *args) {
    // Check if we have exactly one argument
    if (args == NIL || cdr(args) != NIL) {
        set_error(ERR_INVALID_ARGUMENT, "SQRT requires exactly one argument");
        return NIL;
    }

    Cell *arg = car(args);

    // Make sure the argument is a number
    if (arg->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "SQRT requires a numeric argument");
        return NIL;
    }

    // Check for negative input
    if (arg->value.number < 0) {
        set_error(ERR_INVALID_ARGUMENT, "SQRT: Cannot compute square root of negative number");
        return NIL;
    }

    // Simple integer square root - find largest integer whose square is <= n
    int n = arg->value.number;
    int i = 0;
    while ((i+1) * (i+1) <= n) {
        i++;
    }

    return make_number(i);
}

// LT: Less than function (a < b)
Cell *lt_func(Cell *args) {
    // Check if we have exactly two arguments
    if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
        set_error(ERR_INVALID_ARGUMENT, "LT requires exactly two arguments");
        return NIL;
    }

    Cell *a = car(args);
    Cell *b = car(cdr(args));

    // Make sure both arguments are numbers
    if (a->type != CELL_NUMBER || b->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "LT requires numeric arguments");
        return NIL;
    }

    // Perform the comparison
    if (a->value.number < b->value.number) {
        return T;
    } else {
        return NIL;
    }
}

// LTE: Less than or equal function (a <= b)
Cell *lte_func(Cell *args) {
    // Check if we have exactly two arguments
    if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
        set_error(ERR_INVALID_ARGUMENT, "LTE requires exactly two arguments");
        return NIL;
    }

    Cell *a = car(args);
    Cell *b = car(cdr(args));

    // Make sure both arguments are numbers
    if (a->type != CELL_NUMBER || b->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "LTE requires numeric arguments");
        return NIL;
    }

    // Perform the comparison
    if (a->value.number <= b->value.number) {
        return T;
    } else {
        return NIL;
    }
}

// GT: Greater than function (a > b)
Cell *gt_func(Cell *args) {
    // Check if we have exactly two arguments
    if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
        set_error(ERR_INVALID_ARGUMENT, "GT requires exactly two arguments");
        return NIL;
    }

    Cell *a = car(args);
    Cell *b = car(cdr(args));

    // Make sure both arguments are numbers
    if (a->type != CELL_NUMBER || b->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "GT requires numeric arguments");
        return NIL;
    }

    // Perform the comparison
    if (a->value.number > b->value.number) {
        return T;
    } else {
        return NIL;
    }
}

// GTE: Greater than or equal function (a >= b)
Cell *gte_func(Cell *args) {
    // Check if we have exactly two arguments
    if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
        set_error(ERR_INVALID_ARGUMENT, "GTE requires exactly two arguments");
        return NIL;
    }

    Cell *a = car(args);
    Cell *b = car(cdr(args));

    // Make sure both arguments are numbers
    if (a->type != CELL_NUMBER || b->type != CELL_NUMBER) {
        set_error(ERR_TYPE_MISMATCH, "GTE requires numeric arguments");
        return NIL;
    }

    // Perform the comparison
    if (a->value.number >= b->value.number) {
        return T;
    } else {
        return NIL;
    }
}

// Parser
Cell *read_expr(StringReader *reader) {
    int c;
    char token[256];
    int token_len = 0;

     // Skip whitespace and comments
     while (1) {
        c = reader_getc(reader);
        if (c == EOF) {
            return NULL;
        }

        // Check for comments (;; to end of line)
        if (c == ';' && reader->pos < reader->len && reader->input[reader->pos] == ';') {
            // Skip the second semicolon
            reader_getc(reader);

            // Collect comment text for display
            char comment[1024] = {0};
            int comment_len = 0;

            // Read until end of line
            while ((c = reader_getc(reader)) != EOF && c != '\n' && c != '\r') {
                if (comment_len < sizeof(comment) - 1) {
                    comment[comment_len++] = c;
                }
            }
            comment[comment_len] = '\0';

            // Print the comment
            printf(";; %s\n", comment);

            // Continue skipping whitespace or finding more comments
            continue;
        }

        // If not a comment or whitespace, break out
        if (!isspace(c)) {
            break;
        }
    }

    // Check for list syntax
    if (c == '(') {
        // Read list elements
        Cell *head = NIL;
        Cell *tail = NIL;

        // Skip whitespace after open paren
        while ((c = reader_getc(reader)) != EOF && isspace(c)) {
            // Skip
        }

        // Check for empty list
        if (c == ')') {
            return NIL;
        }

        // Put back the non-whitespace character
        reader_ungetc(reader);

        // Read elements until closing paren
        while (1) {
            Cell *elem = read_expr(reader);
            if (!elem && error_type != ERR_NONE) {
                return NULL;
            }

            // Create list node
            Cell *node = cons(elem, NIL);

            if (head == NIL) {
                head = node;
                tail = node;
            } else {
                tail->value.pair.cdr = node;
                tail = node;
            }

            // Skip whitespace
            while ((c = reader_getc(reader)) != EOF && isspace(c)) {
                // Skip
            }

            if (c == ')') {
                break;
            } else if (c == '.') {
                // Handle dotted pair notation
                Cell *last = read_expr(reader);
                if (!last && error_type != ERR_NONE) {
                    return NULL;
                }

                tail->value.pair.cdr = last;

                // Expect closing paren
                while ((c = reader_getc(reader)) != EOF && isspace(c)) {
                    // Skip
                }

                if (c != ')') {
                    set_error(ERR_SYNTAX, "Expected ')' after dotted pair");
                    return NULL;
                }

                break;
            } else {
                // Put back the non-whitespace character for next element
                reader_ungetc(reader);
            }
        }

        return head;
    } else if (c == '\'') {
        // Handle quoted expressions: 'expr -> (QUOTE expr)
        Cell *quoted = read_expr(reader);
        if (!quoted && error_type != ERR_NONE) {
            return NULL;
        }

        return cons(QUOTE_SYM, cons(quoted, NIL));
    } else {
        // Read atom
        token[0] = c;
        token_len = 1;

        while ((c = reader_getc(reader)) != EOF && !isspace(c) && c != '(' && c != ')') {
            if (token_len < sizeof(token) - 1) {
                token[token_len++] = c;
            } else {
                set_error(ERR_SYNTAX, "Token too long");
                return NULL;
            }
        }

        // Put back the terminating character
        if (c != EOF) {
            reader_ungetc(reader);
        }

        // Null-terminate the token
        token[token_len] = '\0';

        // Check if it's a number (simplified - just integers)
        bool is_number = true;
        // Check if it's a negative number
        int start_idx = 0;
        if (token_len > 0 && token[0] == '-') {
            start_idx = 1;
        }

        for (int i = start_idx; i < token_len; i++) {
            if (!isdigit(token[i])) {
                is_number = false;
                break;
            }
        }

        if (is_number && token_len > start_idx) {
            // It's a number, create a number cell
            Cell *cell = (Cell*)malloc(sizeof(Cell));
            if (!cell) set_error(ERR_OUT_OF_MEMORY, "Failed to allocate number");

            cell->type = CELL_NUMBER;
            cell->value.number = atoi(token);

            return cell;
        } else {
            // It's a symbol
            return make_atom(token);
        }
    }
}

// Print a LISP expression
void print_expr(Cell *expr) {
    if (expr == NIL) {
        printf("NIL");
        return;
    }

    if (expr->type == CELL_ATOM) {
        printf("%s", expr->value.atom);
        return;
    }

    if (expr->type == CELL_NUMBER) {
        printf("%d", expr->value.number);
        return;
    }

    if (expr->type == CELL_FUNCTION) {
        printf("<FUNCTION>");
        return;
    }

    if (expr->type == CELL_SPECIAL) {
        printf("<SPECIAL>");
        return;
    }

    // Print a list: (a b c)
    printf("(");
    print_expr(car(expr));

    // Print the rest of the list
    Cell *rest = cdr(expr);
    while (rest != NIL && rest->type == CELL_PAIR) {
        printf(" ");
        print_expr(car(rest));
        rest = cdr(rest);
    }

    // Handle dotted pairs: (a . b)
    if (rest != NIL) {
        printf(" . ");
        print_expr(rest);
    }

    printf(")");
}

// Environment functions
Cell *bind(Cell *sym, Cell *val, Cell *env) {
    return cons(cons(sym, val), env);
}

// Define function - adds a binding to the global environment
Cell *define(Cell *sym, Cell *val) {
    // Safety checks
    if (sym == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "DEFINE: NULL symbol");
        return NIL;
    }

    if (val == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "DEFINE: NULL value");
        return NIL;
    }

    // Find if the symbol is already defined
    Cell *current = env;
    while (current != NIL) {
        Cell *binding = car(current);
        if (eq(car(binding), sym) == T) {
            // Update the existing binding
            binding->value.pair.cdr = val;
            return val;
        }
        current = cdr(current);
    }

    // For debugging
    if (_debug) {
        printf("Defining: ");
        print_expr(sym);
        printf(" as: ");
        print_expr(val);
        printf("\n");
    }

    // Add new binding to global environment
    env = bind(sym, val, env);
    return val;
}

Cell *lookup(Cell *sym, Cell *env) {
    // Safety check - avoid segfaults
    if (env == NULL) {
        char error_msg[256];
        sprintf(error_msg, "Internal error: NULL environment when looking up symbol: %s",
                sym ? (sym->type == CELL_ATOM ? sym->value.atom : "<non-atom>") : "<null>");
        set_error(ERR_INVALID_ARGUMENT, error_msg);
        return NIL;
    }

    while (env != NIL) {
        Cell *binding = car(env);

        // Safety check for malformed bindings
        if (binding == NULL || binding->type != CELL_PAIR) {
            env = cdr(env);
            continue;
        }

        if (eq(car(binding), sym) == T) {
            return cdr(binding);
        }
        env = cdr(env);
    }

    char error_msg[256];
    sprintf(error_msg, "Unbound symbol: %s",
            sym ? (sym->type == CELL_ATOM ? sym->value.atom : "<non-atom>") : "<null>");
    set_error(ERR_UNBOUND_SYMBOL, error_msg);
    return NIL;
}

// Evaluate a list of expressions
Cell *list_of_values(Cell *list, Cell *env) {
    if (list == NIL) {
        return NIL;
    }

    return cons(eval(car(list), env), list_of_values(cdr(list), env));
}

// Evaluate a LISP expression
Cell *eval(Cell *expr, Cell *env) {
    // Safety check
    if (expr == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "EVAL: NULL expression");
        return NIL;
    }

    // Safety check
    if (env == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "EVAL: NULL environment");
        return NIL;
    }

    // Self-evaluating expressions
    if (expr == NIL || expr->type == CELL_FUNCTION || expr->type == CELL_SPECIAL || expr->type == CELL_NUMBER) {
        return expr;
    }

    // Atoms evaluate to their value in the environment
    if (expr->type == CELL_ATOM) {
        return lookup(expr, env);
    }

    // Check that expression is a list
    if (expr->type != CELL_PAIR) {
        set_error(ERR_TYPE_MISMATCH, "EVAL: Expected a list for evaluation");
        return NIL;
    }

    // Lists are evaluated as function applications or special forms
    Cell *op = car(expr);
    if (op == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "EVAL: NULL operator in expression");
        return NIL;
    }

    Cell *args = cdr(expr);

    // Special forms
    if (op->type == CELL_ATOM) {
        // QUOTE: (QUOTE expr) -> expr
        if (eq(op, QUOTE_SYM) == T) {
            if (args == NIL || cdr(args) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "QUOTE requires exactly one argument");
            }
            return car(args);
        }

        // DEFINE: (DEFINE 'symbol value)
        // or (DEFINE 'function-name (params) body)
        if (eq(op, DEFINE_SYM) == T) {
            if (args == NIL) {
                set_error(ERR_INVALID_ARGUMENT, "DEFINE requires at least two arguments");
                return NIL;
            }

            // Get the symbol name (first argument)
            Cell *name_arg = car(args);

            // The name should be quoted
            if (name_arg->type != CELL_PAIR ||
                car(name_arg) != QUOTE_SYM ||
                cdr(name_arg) == NIL) {
                set_error(ERR_TYPE_MISMATCH, "DEFINE: First argument must be a quoted symbol");
                return NIL;
            }

            Cell *sym = car(cdr(name_arg));

            if (sym->type != CELL_ATOM) {
                set_error(ERR_TYPE_MISMATCH, "DEFINE: Symbol name must be an atom");
                return NIL;
            }

            if (_debug) {
                printf("Defining symbol: ");
                print_expr(sym);
                printf("\n");
            }

            // Get the value or function definition (remaining arguments)
            Cell *val_args = cdr(args);

            // Check if it's a function definition (more than one argument after the symbol)
            if (cdr(val_args) != NIL) {
                // Format: (DEFINE 'name (params) body)
                Cell *params = car(val_args);
                Cell *body = car(cdr(val_args));

                if (_debug) {
                    printf("Function definition with params: ");
                    print_expr(params);
                    printf(" and body: ");
                    print_expr(body);
                    printf("\n");
                }

                // Create a lambda expression for the function
                Cell *lambda = cons(LAMBDA_SYM,
                                    cons(params,
                                         cons(body, NIL)));

                // Define it globally
                return define(sym, lambda);
            } else {
                // Format: (DEFINE 'name value)
                Cell *val = eval(car(val_args), env);

                if (_debug) {
                    printf("Value definition, evaluated to: ");
                    print_expr(val);
                    printf("\n");
                }

                // Define it in the global environment
                return define(sym, val);
            }
        }

        // LAMBDA: (LAMBDA (args) body)
        if (eq(op, LAMBDA_SYM) == T) {
            // Create a proper closure by capturing the current environment
            return cons(LAMBDA_SYM,
                        cons(car(cdr(expr)),        // parameters
                            cons(car(cdr(cdr(expr))),  // body
                                cons(env, NIL)))); // current environment
        }

        // COND: (COND (test1 expr1) (test2 expr2) ...)
        if (eq(op, COND_SYM) == T) {
            Cell *clauses = args;

            while (clauses != NIL) {
                Cell *clause = car(clauses);

                if (clause->type != CELL_PAIR) {
                    set_error(ERR_INVALID_ARGUMENT, "COND: clause must be a list");
                }

                Cell *test = car(clause);
                Cell *result = eval(test, env);

                // If test evaluates to non-NIL, evaluate the consequent
                if (result != NIL) {
                    // Special case for one-element clauses (return the test result)
                    if (cdr(clause) == NIL) {
                        return result;
                    }

                    Cell *expr = car(cdr(clause));
                    return eval(expr, env);
                }

                clauses = cdr(clauses);
            }

            // No matching clause
            return NIL;
        }
    }

    // LABEL: (LABEL name (LAMBDA (args) body))
    if (eq(op, LABEL_SYM) == T) {
        if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
            set_error(ERR_INVALID_ARGUMENT, "LABEL requires exactly two arguments");
        }

        Cell *name = car(args);
        Cell *lambda_expr = car(cdr(args));

        // Ensure the name is an atom
        if (name->type != CELL_ATOM) {
            set_error(ERR_TYPE_MISMATCH, "LABEL: First argument must be a symbol");
        }

        // Ensure second argument is a lambda expression
        if (lambda_expr->type != CELL_PAIR ||
            eq(car(lambda_expr), LAMBDA_SYM) != T) {
            set_error(ERR_TYPE_MISMATCH, "LABEL: Second argument must be a LAMBDA expression");
        }

        // Add function to environment for recursive calls - use global env for simplicity
        Cell *result = define(name, lambda_expr);

        // Return the lambda expression
        return lambda_expr;
    }
    // Function application
    Cell *function = eval(op, env);
    Cell *evaluated_args = list_of_values(args, env);

    return apply(function, evaluated_args, env);
    //return function; // Placeholder for now
}

// Apply a function to arguments
Cell *apply(Cell *fn, Cell *args, Cell *env) {
    // Safety check
    if (fn == NULL) {
        set_error(ERR_INVALID_ARGUMENT, "APPLY: NULL function");
        return NIL;
    }

    // Built-in functions
    if (fn->type == CELL_ATOM) {
        if (strcmp(fn->value.atom, "CONS") == 0) {
            if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "CONS requires exactly two arguments");
            }
            return cons(car(args), car(cdr(args)));
        }

        if (strcmp(fn->value.atom, "CAR") == 0) {
            if (args == NIL || cdr(args) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "CAR requires exactly one argument");
            }
            return car(car(args));
        }

        if (strcmp(fn->value.atom, "CDR") == 0) {
            if (args == NIL || cdr(args) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "CDR requires exactly one argument");
            }
            return cdr(car(args));
        }

        if (strcmp(fn->value.atom, "ATOM") == 0) {
            if (args == NIL || cdr(args) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "ATOM requires exactly one argument");
            }
            return atom(car(args));
        }

        if (strcmp(fn->value.atom, "EQ") == 0) {
            if (args == NIL || cdr(args) == NIL || cdr(cdr(args)) != NIL) {
                set_error(ERR_INVALID_ARGUMENT, "EQ requires exactly two arguments");
            }
            return eq(car(args), car(cdr(args)));
        }

        // Arithmetic functions
        if (strcmp(fn->value.atom, "ADD") == 0) {
            return add(args);
        }

        if (strcmp(fn->value.atom, "SUB") == 0) {
            return sub(args);
        }

        if (strcmp(fn->value.atom, "MUL") == 0) {
            return mul(args);
        }

        if (strcmp(fn->value.atom, "DIV") == 0) {
            return div_func(args);
        }

        if (strcmp(fn->value.atom, "SQRT") == 0) {
            return sqrt_func(args);
        }

        if (strcmp(fn->value.atom, "DEBUG") == 0) {
            return debug(args);
        }

        // Comparison functions
        if (strcmp(fn->value.atom, "LT") == 0) {
            return lt_func(args);
        }

        if (strcmp(fn->value.atom, "LTE") == 0) {
            return lte_func(args);
        }

        if (strcmp(fn->value.atom, "GT") == 0) {
            return gt_func(args);
        }

        if (strcmp(fn->value.atom, "GTE") == 0) {
            return gte_func(args);
        }

        char error_msg[256];
        sprintf(error_msg, "Unknown function: %s", fn->value.atom);
        set_error(ERR_UNBOUND_SYMBOL, error_msg);
        return NIL;
    }

    // Lambda expressions: (LAMBDA (params) body)
    if (fn->type == CELL_PAIR && eq(car(fn), LAMBDA_SYM) == T) {
        Cell *params = car(cdr(fn));
        Cell *body = car(cdr(cdr(fn)));

        if (_debug) {
            printf("Applying lambda with params: ");
            print_expr(params);
            printf(" and body: ");
            print_expr(body);
            printf(" to args: ");
            print_expr(args);
            printf("\n");
        }

        // Create a NEW environment for this function call
        // Importantly, we extend the CURRENT environment, not just the global environment
        // This ensures functions have access to the variables in scope when the function is called
        Cell *new_env = env;
        Cell *param = params;
        Cell *arg = args;

        // Bind arguments to parameters
        while (param != NIL && arg != NIL) {
            Cell *param_name = car(param);
            Cell *arg_value = car(arg);

            if (_debug) {
                printf("Binding param: ");
                print_expr(param_name);
                printf(" to value: ");
                print_expr(arg_value);
                printf("\n");
            }

            new_env = bind(param_name, arg_value, new_env);
            param = cdr(param);
            arg = cdr(arg);
        }

        if (param != NIL) {
            set_error(ERR_INVALID_ARGUMENT, "Too few arguments for lambda");
        }

        if (arg != NIL) {
            set_error(ERR_INVALID_ARGUMENT, "Too many arguments for lambda");
        }

        if (_debug) {
            printf("Evaluating lambda body: ");
            print_expr(body);
            printf(" in env: ");
            print_expr(new_env);
            printf("\n");
        }

        // Evaluate body in new environment
        Cell *result = eval(body, new_env);

        if (_debug) {
            printf("Lambda result: ");
            print_expr(result);
            printf("\n");
        }

        return result;
    }

    set_error(ERR_TYPE_MISMATCH, "Object is not a function");
    return NIL;
}

// Add this function to track parenthesis balance
int is_balanced(const char *input) {
    int balance = 0;
    bool in_string = false;
    bool escaped = false;

    for (int i = 0; input[i] != '\0'; i++) {
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (input[i] == '\\') {
                escaped = true;
            } else if (input[i] == '"') {
                in_string = false;
            }
            continue;
        }

        if (input[i] == '"') {
            in_string = true;
        } else if (input[i] == '(') {
            balance++;
        } else if (input[i] == ')') {
            balance--;
            if (balance < 0) return -1;  // Unmatched closing paren
        }
    }

    return balance;  // 0 = balanced, >0 = needs more closing parens
}

// REPL - Read-Eval-Print Loop
void repl() {
    char input_buffer[4096] = {0};  // Larger buffer to accumulate input
    char line[1024];
    int len = 0;

    printf("KLISP Interpreter\n");
    printf("Type expressions or 'exit' to quit\n");

    while (1) {
        // Show appropriate prompt based on input state
        if (len == 0) {
            printf("> ");
        } else {
            printf("... ");  // Continuation prompt
        }
        fflush(stdout);

        // Read a line
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // Check for exit command
        if (len == 0 && strncmp(line, "exit", 4) == 0) {
            break;
        }

        // Append to existing input
        strncat(input_buffer + len, line, sizeof(input_buffer) - len - 1);
        len = strlen(input_buffer);

        // Check if we have a complete expression
        int balance = is_balanced(input_buffer);

        if (balance == 0) {
            // Complete expression - process it
            if (setjmp(error_jmp_buf) == 0) {
                StringReader reader = create_string_reader(input_buffer);
                Cell *expr = read_expr(&reader);

                if (expr) {
                    if (_debug == 1) {
                        printf("** s-expr:\n");
                        print_expr(expr);
                        printf("\n** environment: \n");
                        print_expr(env);
                        printf("\n** result:\n");
                    }
                    Cell *result = eval(expr, env);
                    print_expr(result);
                    printf("\n");                                    }
            } else {
                // Error occurred
                printf("Error: %s\n", error_message);
                clear_error();
            }

            // Reset buffer for next input
            input_buffer[0] = '\0';
            len = 0;
        }
        // If balance > 0, continue reading more input
    }
}


void run_tests() {
    printf("\n** Very Simple UNIT Tests: \n");
    // Create some atoms
    Cell *a = make_atom("A");
    Cell *b = make_atom("B");
    Cell *c = make_atom("C");
    printf("Atom Tests\n");

    // Test CONS
    Cell *pair = cons(a, b);
    printf("CONS test: ");
    print_expr(pair);
    printf("\n");

    // Test CAR
    printf("CAR test: ");
    print_expr(car(pair));
    printf("\n");

    // Test CDR
    printf("CDR test: ");
    print_expr(cdr(pair));
    printf("\n");

    // Test ATOM
    printf("ATOM test on atom: ");
    print_expr(atom(a));
    printf("\n");

    printf("ATOM test on pair: ");
    print_expr(atom(pair));
    printf("\n");

    // Test EQ
    printf("EQ test on same atom: ");
    print_expr(eq(a, a));
    printf("\n");

    printf("EQ test on different atoms: ");
    print_expr(eq(a, b));
    printf("\n");

    // Create a small list (A B C)
    Cell *list = cons(a, cons(b, cons(c, NIL)));
    printf("List test: ");
    print_expr(list);
    printf("\n");

    printf("** End Tests\n\n");
}
// Function to run a LISP file
void run_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file '%s'\n", filename);
        return;
    }

    // Read the entire file into memory
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        printf("Error: Out of memory\n");
        fclose(file);
        return;
    }

    printf("running file: %s\n", filename);
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    // Process the file contents
    if (setjmp(error_jmp_buf) == 0) {
        StringReader reader = create_string_reader(buffer);
        Cell *expr;

        // Execute each expression in the file
        while ((expr = read_expr(&reader)) != NULL) {
            Cell *result = eval(expr, env);
            print_expr(expr);
            printf(" => ");
            print_expr(result);
            printf("\n");
            // Only print the result of the last expression
            // if (reader.pos >= reader.len || reader.input[reader.pos] == '\0') {
            //     print_expr(result);
            //     printf("\n");
            // }
        }
    } else {
        // Error occurred
        printf("Error: %s\n", error_message);
        clear_error();
    }

    free(buffer);
    printf("Done: %s\n", filename);

}

// Modify main to handle file argument
int main(int argc, char *argv[]) {
    // Initialize error handling
    clear_error();

    // Initialize LISP environment
    if (setjmp(error_jmp_buf) == 0) {
        init_lisp();

        if (argc > 1) {
            // Run the specified file
            run_file(argv[1]);
        } else {
            // No file specified, run tests and start REPL
            run_tests();
        }
        repl();

    } else {
        fprintf(stderr, "Fatal error during initialization: %s\n", error_message);
    }

    // Clean up
    cleanup_lisp();

    return 0;
}
