#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

const size_t MAX_ARG_LEN = 64;
const size_t MAX_CHILD_NUM = 16;

typedef struct json_obj {
    char name [MAX_ARG_LEN];
    char value[MAX_ARG_LEN];
    json_obj ** children;
} json_obj_t;

json_obj_t * parseJSON(FILE * json_file);

void jsonObjDtor(json_obj_t * obj);

void jsonObjDump(json_obj_t * obj);

void jsonMakeDot(FILE * dot_file, json_obj_t * node);

void jsonDump(json_obj_t * obj);

#endif
