// Lasse Mads Fenske 260ADM040
// https://github.com/Lasse-RTU/c-graded-lab-arithmetic-parser
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: https://craftinginterpreters.com/parsing-expressions.html [1] I used this to understand to how to build a parser and the logic behind recursive descent
// AI Chat (Claude Opus 5.5): https://claude.ai/share/f56b9601-f38d-48fb-a5dd-49177f2f7c59
// TODO: document where AI helped and where exactly the recursive descent source helped, add required explanatory comments, add more explanatory comments, clean up of typos

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  
#include <string.h> 
#include <math.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#define LAST_NAME "fenske"
#define FIRST_NAME "lasse"
#define STUDENT_ID "260adm040"

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
int ends_with_txt(const char *name);
int make_output_dir(const char *output_dir);
int run_file(const char *input_path, const char *output_dir);
int run_dir(const char *input_dir, const char *output_dir);
void get_base_name(const char *path, char *base_name);



int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *input_dir = NULL;
    const char *output_dir = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "missing argument value for \"-d\" or \"--dir\"");
                return 1;
            }
            if (input_dir != NULL) {
                fprintf(stderr, "too many uses of flag \"-d\" or \"--dir\"");
                return 1;
            }
            input_dir = argv[i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "missing argument value for \"-o\" or \"--output-dir\"");
                return 1;
            }
            if (output_dir != NULL) {
                fprintf(stderr, "too many uses of flag \"-o\" or \"--output-dir\"");
                return 1;
            }
            output_dir = argv[i];
        } else {
            if (input_file != NULL) {
                fprintf(stderr, "Please only provide one input file specifically. For multiple input files use \"-d\" or \"--dir\" and the folder name");
                return 1;
            }
            input_file = argv[i];
        }
    }
    if (input_file == NULL && input_dir == NULL) {
        fprintf(stderr, "Missing input file or folder.");
        return 1;
    }

    const char *input = input_file;
    if (input_dir != NULL) {
        input = input_dir;
    }

    char base_name[1024];
    get_base_name(input, base_name);

    char default_dir[1100];
    if (output_dir == NULL) {
        sprintf(default_dir, "%s_%s_%s", base_name, FIRST_NAME, STUDENT_ID);
        output_dir = default_dir;
    }

    if (make_output_dir(output_dir) != 0) {
        fprintf(stderr, "Error creating output directory");
        return 1;
    }

    if (input_dir != NULL) {
        if (run_dir(input_dir, output_dir) != 0) {
            fprintf(stderr, "Error reading from input directory");
            return 1;
        }
    } else {
        if (run_file(input_file, output_dir) != 0) {
            fprintf(stderr, "Error processing file.");
            return 1;
        }
    }
    return 0;
}

int make_output_dir(const char *output_dir) {
    if (mkdir(output_dir, 0755) != 0 && errno != EEXIST) {
        perror(output_dir);
        return -1;
    }
    return 0;
}

int run_file(const char *input_path, const char *output_dir) {
    char base_name[1024];
    get_base_name(input_path, base_name);

    char output_path[2200];
    sprintf(output_path, "%s/%s_%s_%s_%s.txt",
            output_dir, base_name, FIRST_NAME, LAST_NAME, STUDENT_ID);

    return process_file(input_path, output_path);
}

int run_dir(const char *input_dir, const char *output_dir) {
    DIR *dir = opendir(input_dir);
    if (dir == NULL) {
        perror(input_dir);
        return -1;
    }

    int result = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // skip non txt files
        if (!ends_with_txt(entry->d_name)) {
            continue; 
        }
        char input_path[2200];
        sprintf(input_path, "%s/%s", input_dir, entry->d_name);
        if (run_file(input_path, output_dir) != 0) {
            result = -1;
        }
    }
    closedir(dir);
    return result;
}

void get_base_name(const char *path, char *base_name) {

    char *slash = strrchr(path, '/');
    if (slash != NULL) {
        strcpy(base_name, ++slash);
    } else {
        strcpy(base_name, path);
    }
    
    if (ends_with_txt(base_name) != 0) {
        base_name[strlen(base_name) - 4] = '\0';
    }

}

int ends_with_txt(const char *name) {
    size_t len = strlen(name);
    return len > 4 && strstr(name + len - 4, ".txt") != NULL;
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
            // close the file and reopen it in write mode to overwrite everythig
            fclose(fp);
            fp = fopen(output_path, "w");
            if (fp == NULL) {
                free(buffer);
                return -1;
            }
            fprintf(fp, "ERROR:%ld\n", lex.error_pos);
            break;
        }

        if (v.is_float) {
            fprintf(fp, "%.15g\n", v.d);
        } else {
            fprintf(fp, "%lld\n", v.i);
        }
        i = lex.index + 1;
    }
    fclose(fp);
    free(buffer);
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