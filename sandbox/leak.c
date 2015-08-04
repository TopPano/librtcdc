#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct leak_example {
    char *ptr;
};

void leak1(void)
{
    int *ptr = (int *)malloc(100);

    if (!ptr) {
        printf("Oops, malloc fail!\n");
    }
}

char *leak2(void)
{
    char *ptr = strdup("Hey Hey Hey!\n");

    if (!ptr) {
        printf("Oops, strdup fail!\n");
        return 0;
    }
    return ptr;
}

struct leak_example *leak3(void)
{
    struct leak_example *ptr = 0;

    ptr = (struct leak_example *) malloc(sizeof(struct leak_example));
    if (!ptr) {
        printf("Oops, malloc fail!\n");
    }
    ptr->ptr = strdup("Hey Hey Hey!\n");

    return ptr;
}

int main()
{
    struct leak_example *lk_ptr;
    char * ch_ptr = 0;

    leak1();

    ch_ptr = leak2();
    if(ch_ptr) {
        printf("%s", ch_ptr);
    }

    lk_ptr = leak3();
    if(lk_ptr) {
        printf("%s", lk_ptr->ptr);
        free(lk_ptr);
    }

    return 0;
}
