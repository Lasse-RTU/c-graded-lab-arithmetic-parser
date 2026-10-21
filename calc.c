// Lasse Mads Fenske 260ADM040
// https://github.com/Lasse-RTU/c-graded-lab-arithmetic-parser
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  

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
} Lexer;

// reads the whole file inot one buffer with '\0' at the end
char *read_file(const char *filename, long *len);
void next_token(Lexer *lex);



int main() {
  
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
    // Token position is 1 based
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
                    if (i < lex->length && lex->buffer[i] == '.') {
                        if (i + 1 < lex->length && isdigit((unsigned char) lex->buffer[i + 1])) {                        
                            i++; // skip the dot
                            while (i + 1 < lex->length && isdigit((unsigned char)lex->buffer[i + 1])) {
                                i++;
                            }
                            t.type = NUM;
                            t.is_float = 1;
                            t.d = strtod(lex->buffer + start, NULL);
                            i++;
                        }
                    } else {
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