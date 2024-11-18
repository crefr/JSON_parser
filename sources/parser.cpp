#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "parser.h"
#include "logger.h"

const size_t MAX_CHILDREN_NUM = 10;
const size_t MAX_FILE_SIZE = 4096;

typedef struct {
    char * buf;
    size_t pos;
} buffer_t;

static buffer_t makeBuffer(FILE * file, size_t size);

static void cleanBuffer(buffer_t * buffer);

static json_obj_t * parseObj(buffer_t file_buffer, const char * name);

json_obj_t * parseJSON(FILE * json_file)
{
    assert(json_file);

    buffer_t buffer = makeBuffer(json_file, MAX_FILE_SIZE);

    json_obj_t * main_obj = parseObj(buffer, "main");

    cleanBuffer(&buffer);

    return main_obj;
}

void jsonObjDtor(json_obj_t * obj)
{
    assert(obj);

    if (obj->children != NULL){
        size_t child_index = 0;

        while (obj->children[child_index] != NULL){
            jsonObjDtor(obj->children[child_index]);
            child_index++;
        }

        free(obj->children);
        obj->children = NULL;
    }

    free(obj);
}

static json_obj_t * parseObj(buffer_t file_buffer, const char * name)
{
    assert(name);

    json_obj_t * new_obj = (json_obj_t *)calloc(1, sizeof(*new_obj));

    new_obj->children = NULL;

    char buffer[MAX_ARG_LEN] = "";
    size_t count = 0;

    int shift = 0;
    if (sscanf(file_buffer.buf + file_buffer.pos, " { %n", &shift) > 0){
        file_buffer.pos += shift;

        new_obj->children = (json_obj_t **)calloc(1, sizeof(json_obj_t *));

        while (sscanf(file_buffer.buf + file_buffer.pos, " \"%s\": %n", buffer, &shift) > 0){
            file_buffer.pos += shift;
            new_obj->children[count] = parseObj(file_buffer, buffer);
            count++;
        }
        sscanf(file_buffer.buf + file_buffer.pos, " } %n", &shift);
        file_buffer.pos += shift;
    }
    else {
        sscanf(file_buffer.buf + file_buffer.pos, " %s , ", new_obj->value);
    }

    return new_obj;
}

static buffer_t makeBuffer(FILE * file, size_t size)
{
    assert(file);

    buffer_t buffer = {};
    buffer.buf = (char *)calloc(size, sizeof(*buffer.buf));

    fread(buffer.buf, sizeof(*buffer.buf), size, file);

    return buffer;
}

static void cleanBuffer(buffer_t * buffer)
{
    assert(buffer);

    free(buffer->buf);
    buffer->buf = NULL;
}

void jsonObjDump(json_obj_t * obj)
{
    assert(obj);


}
