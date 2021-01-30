#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "code/chunk.h"
// #include "code/value.h"
// #include "code/opcodes.h"
#include "vm.h"
// #include "debugging/disassembly.h"
// #include "scanning/scanner.h"

typedef FILE File;

static char* read_file(const char* path) {
    File* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);

    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void start_repl() {
    char line[1024];

    // Keeps reading for new lines until EOF and runs them
    while (1) {
        printf(">> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("Exiting...\n");
            break;
        }

        // run the line
        vm_execsource(line);
    }
}

static void exec_file(const char* file_path) {
    char * source = read_file(file_path);
    InterpretResult result = vm_execsource(source);
    free(source);

    // emit exit code based on result
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(71);
}

int main(int argc, char** argv) {
    vm_init();
    if (argc == 1) {
        start_repl();
    }
    else if (argc == 2) {
        exec_file(argv[1]);
    }
    else {
        fprintf(stderr, "Usage: volt [file]\n");
    }

    vm_free();
    return 0;
}

// int main() {
//     Value val = MK_VAL_NUM(23.65);
//     print_val(&val);
//     return 0;
// }

// int main() {
//     Chunk cnk;
//     chunk_init(&cnk);
    
//     int constant;
//     Value val = MK_VAL_NUM(89.34);

//     constant = chunk_addconst(&cnk, val);
    
//     chunk_write(&cnk, OP_LOADCONST);
//     chunk_write(&cnk, constant);
//     chunk_write(&cnk, OP_LOADCONST);
//     chunk_write(&cnk, constant);
//     chunk_write(&cnk, OP_ADD);
//     chunk_write(&cnk, OP_RETURN);

//     disassemble_chunk(&cnk, "Test chunk");

//     vm_init();

//     vm_execchunk(&cnk);

//     vm_free();
//     chunk_free(&cnk);

//     Value k = vm_peekstack(0);
//     print_val(&k);
//     return 0;
// }