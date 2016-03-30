#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: %s <num keys> <num probes> <list of fanout parameters...>", argv[0]);
    }
    printf("building the tree\n");
    return 0;
}
