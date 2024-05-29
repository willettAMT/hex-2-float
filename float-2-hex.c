#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define BUF_LEN 1000
#define FLOAT 0
#define DOUBLE 1

int float_to_hex(short);

int main (int argc, char **argv)
{
    int opt, f_d = FLOAT; 
    while ((opt = getopt(argc, argv, "fdH")) != -1)
    {
        switch (opt)
        {
            case 'f':
                f_d = FLOAT;
                break;
            case 'd':
                f_d = DOUBLE;
                break;
            case 'H':
                printf("Usage: ./float-2-hex [OPTION ...]\n"
                        "\t-f convert the input into floats for hex output (this is the default)\n"
                        "\t-d convert the input into doubles for hex output\n"
                        "\t-H display this help message and exit\n");
                exit(EXIT_SUCCESS);
            default: 
                break;
        }
    }

    if (optind < argc)
    {
        fprintf(stderr, "\nFlags not recognized:\n");
        for (int i = optind; i < argc; ++i)
            printf("\t%s\n", argv[i]);
    }

    return float_to_hex(f_d);
}


int float_to_hex(short f_d)
{
    float f;
    double lf;
    unsigned int u_f;
    unsigned long u_lf;
    char buf[BUF_LEN] = {'\0'};

    while (fgets(buf, BUF_LEN, stdin))
    {
        buf[strlen(buf) - 1] = '\0'; // rids the \n
        if (!f_d)
        {
            if (sscanf(buf, "%f", &f) != 1) // sscanf, param(buf string, type format, address to fill value)
            {
                fprintf(stderr, "Failed to scan value from input <%s>\n", buf);
                exit(EXIT_FAILURE);
            }
            else
            { 
                memcpy(&u_f, &f, sizeof(f));
                printf("%-40s\t%.10e\t%.10f\t0x%08x\n", buf, (double)f, (double)f, u_f); 
                
            }
        }
        else
        {
            if (sscanf(buf, "%lf", &lf) != 1)
            {
                fprintf(stderr, "Failed to scan value from input <%s>\n", buf);
                exit(EXIT_FAILURE);
            }
            else
            {
                memcpy(&u_lf, &lf, sizeof(lf));
                printf("%-40s\t%.16le\t%.16lf\t0x%016lx\n", buf, (double)lf, (double)lf, u_lf);
            }
        }
    }

    return EXIT_SUCCESS;
}
