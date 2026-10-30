#include <stdio.h>
#include <string.h>

struct Point {
    int x;
    int y;
};

// squares a number
int square(int n) {
    return n * n;
}

int main(int argc, char *argv[]) {
    char *name = "Slop";
    int nums[] = {4, 5, 6};
    int total = 0;
    for (int i = 0; i < 3; i++) {
        total += nums[i];
    }
    struct Point p = {3, 4};
    printf("Hello %s! total=%d len=%d\n", name, total, strlen(name));
    printf("square: %d, point: %d,%d\n", square(7), p.x, p.y);
    double avg = total / 3.0;
    printf("avg %.2f\n", avg);
    int k = 10;
    while (k > 0) {
        k -= 4;
    }
    if (k < 0) printf("below zero\n");
    else printf("zero or more\n");
    return 0;
}
