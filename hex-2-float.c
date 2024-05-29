#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define F_EXP_BITS_DEFAULT 8
#define F_FRAC_BITS_DEFAULT 23
#define F_FRAC_ADD_DEFAULT 1.0
#define HEX 16
#define DOUBLE_FRAC 52
#define DOUBLE_EXP 11
#define B16_FRAC 10
#define B16_EXP 5
#define BFLOAT_FRAC 7
#define BFLOAT_EXP 8
#define MINI_FLOAT_FRAC 3
#define MINI_FLOAT_EXP 4
#define MINI_FLOAT_BIAS -2.0
#define EXP_MASK  0x1Lu
#define FRAC_MASK 0x0Lu;
#define BIT 1L
#define TRUE 1
#define FALSE 0
#define DOUBLE_INIT 0.0
#define LONG_INIT 0;
#define BUF_LEN 1000

int h2f(char *file, long exp_bits, long frac_bits, double exp_bias, double frac_add, short prec, short verb);
unsigned int get_bits(unsigned int uv, int begin_bit, int num_bits);
double find_bias (int k, short verb);
int settings(short prec, long int *frac_bits, long int *exp_bits, double *exp_bias);
unsigned long e_mask(unsigned long frac_bits, long exp_bits);
void print_bits(unsigned long mask, long sign, long frac_bits, long exp_bits);

int main (int argc, char **argv)
{
    short opt, prec = 0, verb = FALSE;
    char file[BUF_LEN] = {'\0'}, *buf;
    long exp_bits = F_EXP_BITS_DEFAULT, 
         frac_bits = F_FRAC_BITS_DEFAULT;
    double exp_bias = DOUBLE_INIT,
           frac_add = F_FRAC_ADD_DEFAULT;

    while ((opt = getopt(argc, argv, "Hhbmvde:E:f:F:i:")) != -1)
    {
        switch (opt)
        {
            case 'i': /* takes argument for input file  if none is passed, default is stdin */
                strcpy(file, optarg);
                break;
            case 'd': /* settings for double, 64 (1 (s) - 11 (exp) - 52 (frac)) */
                prec = 1;
                break;
            case 'h':/* settings for half precision binary16, 16-bits  (1 (s) - 5(exp) - 10(frac)) */
                prec = 2;
                break;
            case 'b': /* settings for half precision  bfloat16, 16-bits (1 (s) - 8(exp) - 7(frac)) */
                prec = 3;
                break;
            case 'm': /* settings for quarter precision minifloat, 8-bits, bias (-2) (1 (s) - 4(exp) - 3(frac)) */
                prec = 4;
                break;
            case 'e':/* set # of bits to use for the exponent */
                exp_bits = strtol(optarg, NULL, 10);
                break;
            case 'E':
                exp_bias = strtod(optarg, &buf); /* set value used for exponent bias */
                break;
            case 'f':
                frac_bits = strtol(optarg, NULL, 10);/* set # of bits to use for fraction */
                break;
            case 'F':
                frac_add = strtod(optarg, &buf); /* set value to add to the fraction (unstored fraction bits) */
                break;
            case 'v':
                verb = TRUE;
                break;
            case 'H':
                printf("Usage: ./hex-2-float.c [OPTION ...]\n"
                        "\t-i [file_name] | specifies the name of an input file| DEFAULT stdin\n"
                        "\t-d use settings for double precision (double, 64-bits)\n"
                        "\t-h use settings for half precision (binary16, 16-bits)\n"
                        "\t-b use settings for half precision (bfloat16, 16-bits)\n"
                        "\t-m use settings for quarter precision (minifloat, 8-bits)\n"
                        "\t-e # set the number of bits to use for the exponent\n"
                        "\t-E # set the value used for the exponent bias\n"
                        "\t-f # set the number of bits to use for the fraction\n"
                        "\t-F # set the value to add to the fraction (unstored fraction bits)\n"
                        "\t-v verbose mode\n"
                        "\t-H display this help message and exit.\n");
                break;
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

    if (verb) 
        fprintf(stderr, "\n\nfile read!\nfile: %s\n", file);
    if (frac_bits && !frac_add) frac_add = 1;

    return h2f(file, exp_bits, frac_bits, exp_bias, frac_add, prec, verb);
}

int h2f(char *file, long exp_bits, long frac_bits, double exp_bias, double frac_add, short prec, short verb)
{
    char buf[BUF_LEN] = {'\0'}, special[BUF_LEN] = {'\0'};
    FILE *stream = NULL;
    uint8_t nan = FALSE;
    int sign_mul;
    double exp, ub_exp, plain_frac, frac, power, value;
    unsigned long uv, mask, sign, sign_mask, exp_mask;

    (!file[0] == '\0') ? (stream = fopen(file, "r")) : (stream = stdin);//   Read from file or stdin 

    while (fgets(buf, BUF_LEN, stream))
    {
        buf[strlen(buf) - 1] = '\0'; // rids the \n
        ub_exp = LONG_INIT; // value of unbiased exponent
        exp = 0; // final exponent value
        plain_frac = DOUBLE_INIT; // frac before calc
        frac = DOUBLE_INIT; // frac after calc
        sign = 1.0;
        nan = FALSE;
        power = 1;
        value = DOUBLE_INIT; // final value
        uv = 0x0;
        exp_mask = EXP_MASK;
        sign_mask = BIT << (exp_bits + frac_bits); // sets sign mask

        if (sscanf(buf, "%lx", &uv) != 1)
        {
            fprintf(stderr, "Failed to scan value from input <%s>\n", buf);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s\n", buf); // echo buf

            if (prec) settings(prec, &frac_bits, &exp_bits, &exp_bias); // flagged presets
            if (!exp_bias) exp_bias = find_bias(exp_bits, verb);
            mask = BIT << (exp_bits + frac_bits); // sets mask with calculated values
            sign = uv & sign_mask;  // calculates and sets sign
            sign_mul = (uv & mask) ? -1 : 1; // assigns sign multiple for final sign
            plain_frac = DOUBLE_INIT; 
            exp_mask = e_mask(frac_bits, exp_bits); // calculate exponent mask
            ub_exp = uv & exp_mask;                             
            power = 1;

            // PRINTS UV
            printf("\t%d ", (sign) ? 1 : 0);
            for (int i = exp_bits - 1; i >= 0; --i) {
                mask >>= 1;
                printf("%d", (uv & mask) ? 1 : 0);
            }
            printf(" ");
            for (int i = frac_bits - 1; i >= 0; --i) {
                mask >>= 1;
                printf("%d", (uv & mask) ? 1 : 0);
                if (uv & mask)
                {
                    plain_frac += pow(2, (-1 * power));
                    nan = TRUE;
                }
                ++power;
            } // LABELS
            printf("\n\ts ");
            for (int i = exp_bits; i > 0; --i) {
                mask >>= 1;
                printf("e");
            }
            printf(" ");
            for (int i = frac_bits; i > 0; --i) {
                mask >>= 1;
                printf("f");
            }

            // Special Values: check if exp_mask is all 1's. If it is then branch into special values
            if (ub_exp == exp_mask)
            {
                if (!nan)
                {
                    if (!sign) 
                        strcpy(special, "positive infinity");
                    else strcpy(special, "negative infinity");
                }
                else
                    strcpy(special, "NaN");
                printf("\n\tspecial value\n\t%s\n\n", special);
            }
            else
            {

                ub_exp = uv & exp_mask; // WORKS! 
                ub_exp = (unsigned long) ub_exp >> frac_bits; // WORKS! 
                frac = frac_add + plain_frac; // turn into fraction

                if (ub_exp == 0) // Denormalized
                {
                    exp = (1 - exp_bias); // denormalized bias in exp
                    frac = plain_frac;
                }
                else
                {
                    exp = ub_exp - exp_bias;
                    frac = frac_add + plain_frac;
                }

                value = (sign_mul * frac * pow(2, exp));
                printf("\n");
                printf("\t%s\n", (ub_exp == 0) ? "denormalized value" : "normalized value");
                printf("\tsign:\t\t%s\n", (!sign) ? "positive" : "negative"); // sign:
                printf("\tbias:\t\t%-10.0lf\n", exp_bias);
                printf("\tunbiased exp:\t%-10.0lf\n", ub_exp);
                printf("\tE:\t\t%-10.0f\n", exp);
                printf("\tfrac:\t\t%-.20lf\n", plain_frac);
                printf("\tM:\t\t%-.20lf\n", frac);
                printf("\tvalue:\t\t%-.20lf\n", value);
                printf("\tvalue:\t\t%-.20le\n\n", value);
            }
        }
    }

    return EXIT_SUCCESS;
}

int settings(short prec, long int *frac_bits, long int *exp_bits, double *exp_bias)
{
    if (prec > 0 && prec < 5)
        switch (prec){
            case 1:
                *exp_bits = DOUBLE_EXP;
                *frac_bits = DOUBLE_FRAC;
                break;
            case 2:
                *exp_bits = B16_EXP;
                *frac_bits = B16_FRAC;
                break;
            case 3: 
                *exp_bits = BFLOAT_EXP;
                *frac_bits = BFLOAT_FRAC;
                break;
            case 4:
                *exp_bits = MINI_FLOAT_EXP;
                *frac_bits = MINI_FLOAT_FRAC;
                *exp_bias = MINI_FLOAT_BIAS;
            default: /* ?? */
                break;
        }

    return EXIT_SUCCESS;
}

double find_bias (int k, short verb)
{
    double bias = pow(2, k - 1) - 1;

    if (verb)
        fprintf(stderr, "\n\nCalculated bias: %lf\n", bias);

    return bias;
}

unsigned long e_mask(unsigned long frac_bits, long exp_bits)
{
    unsigned long exp_mask = EXP_MASK;

    for (int i = 0; i < exp_bits - 1; ++i)
    {
        exp_mask <<= BIT;
        exp_mask |= EXP_MASK;
    }

    exp_mask <<= frac_bits;
    return exp_mask;
}
