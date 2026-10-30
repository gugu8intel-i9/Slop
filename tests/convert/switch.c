#include <stdio.h>
int main() {
    int day = 2;
    switch (day) {
        case 1:
            printf("one\n");
            break;
        default:
            printf("other\n");
    }
    int big = day > 1 ? 1 : 0;
    for (int i = 10; i > 0; i -= 3) {
        if (i == 4) {
            break;
        }
        printf("%d\n", i);
    }
    printf("done %d\n", big);
    return 0;
}
