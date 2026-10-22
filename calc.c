// Lasse Mads Fenske 260ADM040
// https://github.com/Lasse-RTU/c-graded-lab-arithmetic-parser
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: https://craftinginterpreters.com/parsing-expressions.html [1] I used this to understand to how to build a parser and the logic behind recursive descent

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  
#include <string.h> 
#include <math.h>

typedef enum {
    NUM, PLUS, MINUS, MUL, DIV, POWER, LEFTP, RIGHTP, END, INVALID
    } TokenType;

typedef struct {
    TokenType type;
    long pos;
    int is_float; // 1 if the NUM token had a '.' otherwise false
    long long i; // integer values
    double d; // float values for floats
} Token;

typedef struct {
    const char *buffer;
    long length;
    long index;
    Token current;
    long error_pos;
} Lexer;

typedef struct {
    int is_float;
    long long i;
    double d;
} Value;

// reads the whole file inot one buffer with '\0' at the end
char *read_file(const char *filename, long *len);
void next_token(Lexer *lex);
Value expr(Lexer *lex);
Value term(Lexer *lex);
Value power(Lexer *lex);
Value primary(Lexer *lex);
double to_double(Value v);
long long pow_sucks(long long a, long long b);
int process_file(const char *input_path, const char *output_path);



int main(int argc, char **argv) {
    char filename = "";
    const char *input = "12.5 ** (3 + 4)";
    const char *input2 = "42 * 4 - 2 + 5. + 3";
    Lexer lex = {input2, (long)strlen(input2), 0, {0}, 0};

    return 0;
}

char *read_file(const char *filename, long *len) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    // allocates one more byte than the file size for adding \0 at the end
    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }
    
    if(fread(buffer, 1, size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    buffer[size] = '\0';
    *len = size;
    return buffer;

}

void next_token(Lexer *lex) {
    long i = lex -> index;
    Token t = {0};
    // skip spaces and tabs to find next actual token
    while (i < lex->length && (lex->buffer[i] == ' ' || lex->buffer[i] == '\t')) {
        i++;
    }
    // Token position is 1 based so the token pos is one above the buffer index
    t.pos = i + 1;

    if (i>=lex->length || lex->buffer[i] == '\n') {
        t.type = END;
    } else {
        switch(lex->buffer[i]) {
            case '+':
                t.type = PLUS;
                i++;
                break;
            case '-':
                t.type = MINUS;
                i++;
                break;
            case '/':
                t.type = DIV;
                i++;
                break;
            case '*':
                // check the next character to see if it also is a '*'
                // which would mean its the power symbol and not multiplication
                if (i + 1 < lex->length && lex->buffer[i + 1] == '*') {
                    t.type = POWER;
                    i+=2;
                } else {
                    t.type = MUL;
                    i++;
                }
                break;
            case '(':
                t.type = LEFTP;
                i++;
                break;
            case ')':
                t.type = RIGHTP;
                i++;
                break;
            default:
                if (isdigit((unsigned char)lex->buffer[i])) {
                    long start = i;
                    while (i < lex->length && isdigit((unsigned char)lex->buffer[i])) {
                        i++;
                    }
                    if (i + 1 < lex->length && lex->buffer[i] == '.' && isdigit((unsigned char)lex->buffer[i + 1])) {
                        i++; // skip the dot
                        while (i < lex->length && isdigit((unsigned char)lex->buffer[i])) {
                            i++;
                        }
                        t.type = NUM;
                        t.is_float = 1;
                        t.d = strtod(lex->buffer + start, NULL);
                    } else {
                        // this call return an integer and
                        // the next call returns the '.' as INVALID
                        t.type = NUM;
                        t.is_float = 0;
                        t.i = strtoll(lex->buffer + start, NULL, 10);
                    }
                } else {
                    t.type = INVALID;
                    i++;
                }
                break;
        }
    }
    lex->index = i;
    lex->current = t;
}

// based on the logic and explanations from [1]
Value expr(Lexer *lex) {
    Value v = term(lex);
    if (lex->error_pos != 0) {
        return v;
    }

    while (lex->current.type == PLUS || lex->current.type == MINUS) {
        Token operator = lex->current;
        next_token(lex);
        Value right_value = term(lex);
        if (lex->error_pos != 0) {
            return v;
        }
        if (v.is_float || right_value.is_float) {
            double a = to_double(v);
            double b = to_double(right_value);
            v.is_float = 1;
            if (operator.type == PLUS) {
                v.d = a + b;
            } else {
                v.d = a - b;
            }
            if (isnan(v.d) || isinf(v.d)) {
                lex->error_pos = operator.pos;
                return v;
            }  
        } else {
            if (operator.type == PLUS) {
                v.i = v.i + right_value.i;
            } else {
                v.i = v.i - right_value.i;
            }
        }
    }
    return v;
}

// based on the logic and explanations from [1]
Value term(Lexer *lex) {
    Value v = power(lex);
    if (lex->error_pos != 0) {
        return v;
    }
    while (lex->current.type == MUL || lex->current.type == DIV) {
        Token operator = lex->current;
        next_token(lex);
        long right_position = lex->current.pos; // start of the right operand for error reporting
        Value right_value = power(lex);
        if (lex->error_pos != 0) {
            return v;
        }
        if (v.is_float || right_value.is_float) {
            double a = to_double(v);
            double b = to_double(right_value);
            v.is_float = 1;
            if (operator.type == MUL) {
                v.d = a * b;
            } else {
                if (b == 0) {
                    lex->error_pos = right_position;
                    return v;
                } else {
                    v.d = a / b;
                }
            }
            if (isnan(v.d) || isinf(v.d)) {
                lex->error_pos = operator.pos;
                return v;
            }         
        } else {
            if (operator.type == MUL) {
                v.i = v.i * right_value.i;
            } else {
                if (right_value.i == 0) {
                    lex->error_pos = right_position;
                    return v;
                } else {
                    v.i = v.i / right_value.i;
                }
            }
        }
    }
    return v;
}

// based on the logic and explanations from [1]
Value power(Lexer *lex) {
    Value v = primary(lex);
    if (lex->error_pos != 0) {
        return v;
    }

    if (lex->current.type == POWER) {
        long operator_pos = lex->current.pos; // needed for error index in case of NaN and infinite
        next_token(lex);
        long exponent_pos = lex->current.pos; // needed for error index in case of negative exponent for integer exponentiation 
        Value right_value = power(lex);
        if (lex->error_pos != 0) {
            return v;
        }
        if (v.is_float || right_value.is_float) {
            double a = to_double(v);
            double b = to_double(right_value);
            v.is_float = 1;
            v.d = pow(a, b);
            if (isnan(v.d) || isinf(v.d)) {
                lex->error_pos = operator_pos;
                return v;
            }  
        } else {
            if (right_value.i < 0) {
                lex->error_pos = exponent_pos;
                return v;
            } else {
                v.i = pow_sucks(v.i, right_value.i);
            }
        }
    }
    return v;
}

// based on the logic and explanations from [1]
Value primary(Lexer *lex) {
    Value v = {0};
    Token t = lex->current;
    if (t.type == NUM) {
        v.is_float = t.is_float;
        v.i = t.i;
        v.d = t.d;
        next_token(lex);
    } else if (t.type == LEFTP) {
        next_token(lex);
        v = expr(lex);
        if (lex->error_pos != 0) {
            return v;
        }
        if (lex->current.type == RIGHTP) {
            next_token(lex);
        } else {
            lex->error_pos = lex->current.pos;
            return v;
        }
    } else {
        lex->error_pos = t.pos;
    }
    return v;
}

int process_file(const char *input_path, const char *output_path) {
    Lexer lex = {0};
    long length = 0;

    char *buffer;
    buffer = read_file(input_path, &length);

    if (buffer == NULL) {
        return -1;
    }

    FILE *fp = fopen(output_path, "w");
    if (fp == NULL) {
        free(buffer);
        return -1;
    }

    lex.buffer = buffer;
    lex.length = length;

    long i = 0;

    while (i < length) {

        long j = i;
        // skip white spaces and tabs at the beginning
        while (j < length && (buffer[j] == ' ' || buffer[j] == '\t')) {
            j++;
        }
        // only blank spaces till end of file
        if (j >= length) {
            break;
        }
        // blank line
        if (buffer[j] == '\n') {
            i = j + 1;
            continue;
        }
        // skip to end of comment line
        if (j < length && buffer[j] == '#') {
            while (j < length && buffer[j] != '\n') {
                j++;
            }
            // skip new line
            i = j + 1;
            continue;
        }

        lex.index = i;
        next_token(&lex);
        Value v = expr(&lex);

        if (lex.error_pos == 0 && lex.current.type != END) {
            lex.error_pos = lex.current.pos;
        }

        if (lex.error_pos != 0) {
            break;
        }

        if (v.is_float) {
            fprintf(fp, "%.15g\n", v.d);
        } else {
            fprintf(fp, "%lld\n", v.i);
        }
        i = lex.index + 1;
    }

    return 0;
}

double to_double(Value v) {
    if (v.is_float) {
        return v.d;
    } else {
        return (double) v.i;
    }
}

// the default pow function of the math library uses double for calculation
// and thus is imprecise. That's why I had to create this helper function
// b needs to be at least 0
long long pow_sucks(long long a, long long b) {
    long long result = 1;
    for (long long i = 1; i <= b ;i++) {
        result *= a;
    }
    return result;
}