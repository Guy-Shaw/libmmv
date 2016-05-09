#include <stdio.h>
#include <unistd.h>	// Import size_t, ssize_t
#include <stdlib.h>     // Import exit()

extern ssize_t qp_encode(char *buf, size_t sz, char *str);

int main(int argc, char **argv)
{
    char line_buf[1024];
    char qp_buf[1024];
    size_t len;
    ssize_t rlen;
    int c;

    len = 0;
    while (1) {
        c = fgetc(stdin);
        if ((c == EOF && len != 0) || c == '\n') {
#if 0
            if (len != 0 && line_buf[len - 1] == '\r') {
                --len;
            }
#endif
            line_buf[len] = '\0';
            rlen = qp_encode(qp_buf, sizeof (qp_buf), line_buf);
            if (rlen < 0) {
                fprintf(stderr, "Error %d\n", rlen);
                fputs(qp_buf, stdout);
                fputs(" ... ERROR\n", stdout);
            }
            else {
                fputs(qp_buf, stdout);
                fputc('\n', stdout);
            }
            len = 0;
        }
        else if (c == EOF) {
            break;
        }
        else {
            line_buf[len++] = c;
        }
    }

    exit(0);
}
