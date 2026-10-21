// Lasse Mads Fenske 260ADM040
// https://github.com/Lasse-RTU/c-graded-lab-arithmetic-parser
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>


#include <stdio.h>
#include <stdlib.h>

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

// reads the whole file inot one buffer with '\0' at the end
char *read_file(const char *filename, long *len);


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