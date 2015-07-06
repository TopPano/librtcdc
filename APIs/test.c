#include <stdio.h>
#include <string.h>
#include <jansson.h>

int main()
{
    json_t *root, *array, *integer;
    root = json_object();
    array = json_array();
    
    json_object_set_new(root, "command", json_string("hihihi"));
    json_object_set_new(root, "results", array);
    json_array_append(array, json_string("kkkk"));

    char *jj = json_dumps(root, 0);
    json_decref( root );
    printf("%s\n", jj);
   
    const char *cc = (const char *)jj;
    json_error_t *err;
    json_t *kk = json_loads(cc, JSON_DECODE_ANY, err);

    char *command, *rr;
    json_unpack(kk, "{s:s, s:[s]}", "command", &command, "results", &rr);
    printf("command: %s\n", command);
    printf("results: %s\n\n", rr);


    json_t *arr = json_pack("[i,i,i,i,i]", 1, 2, 3,4,5);
    int a;
    size_t index;
    json_t *value;
    json_array_foreach(arr, index, value){
        json_unpack(value, "i", &a);
        printf("a:%d\n", a);
    }
    return 0;
}
