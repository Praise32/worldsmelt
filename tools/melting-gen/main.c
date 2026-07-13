#include "llama.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            llama_backend_init();
            printf("melting-gen (llama.cpp b9979)\n%s\n", llama_print_system_info());
            llama_backend_free();
            return 0;
        }
    }
    fprintf(stderr, "melting-gen: uso: --version (altre opzioni nei task successivi)\n");
    return 1;
}
