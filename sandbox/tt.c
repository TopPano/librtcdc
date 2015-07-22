#include <stdio.h>

int main()
{
    int a, b;
    int *p = NULL;
    
    printf("p:%p\n", p);
    
    p = &a;

    printf("&a:%p, p:%p\n",&a, p);

    *p = a; 
    printf("&a:%p, p:%p\n",&a, p);
}
