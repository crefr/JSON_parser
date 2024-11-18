#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

static void nodeMakeDot(FILE * dot_file, json_obj_t * node, json_obj_t * parent);

static json_obj_t * parseObj(buffer_t * file_buffer, const char * name);

static void jsonDumpGraph(json_obj_t * root_node);

json_obj_t * parseJSON(FILE * json_file)
{
    assert(json_file);

    buffer_t buffer = makeBuffer(json_file, MAX_FILE_SIZE);

    json_obj_t * main_obj = parseObj(&buffer, "main");

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

static json_obj_t * parseObj(buffer_t * file_buffer, const char * name)
{
    assert(name);

    json_obj_t * new_obj = (json_obj_t *)calloc(1, sizeof(*new_obj));

    new_obj->children = NULL;
    strcpy(new_obj->name, name);

    char buffer[MAX_ARG_LEN] = "";
    size_t count = 0;

    int shift = 0;
    sscanf(file_buffer->buf + file_buffer->pos, " { %n", &shift);

    if (shift > 0){
        file_buffer->pos += shift; printf("shift: %d\n", shift);

        new_obj->children = (json_obj_t **)calloc(MAX_CHILDREN_NUM, sizeof(json_obj_t *));

        while (sscanf(file_buffer->buf + file_buffer->pos, " \"%[^\"]\": %n", buffer, &shift) > 0){
            file_buffer->pos += shift;
            printf("buffer: '%s', shift: %d, pos: %zu , count: %d\n", buffer, shift, file_buffer->pos, count);
            new_obj->children[count] = parseObj(file_buffer, buffer);
            count++;
        }
        sscanf(file_buffer->buf + file_buffer->pos, " } %n", &shift);
        file_buffer->pos += shift;
    }
    else {
        sscanf(file_buffer->buf + file_buffer->pos, " %[^,] , %n", new_obj->value, &shift);
        file_buffer->pos += shift;

        printf("val: %s\n", new_obj->value);
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

void jsonDump(json_obj_t * obj)
{
    assert(obj);

    jsonObjDump(obj);
    jsonDumpGraph(obj);
}

void jsonObjDump(json_obj_t * obj)
{
    assert(obj);

    printf("entered '%s'\n", obj->name);

    logPrint(LOG_DEBUG, "\"%s\": ", obj->name);

    if (obj->children != NULL){
        logPrint(LOG_DEBUG, "{\n");

        size_t child_index = 0;
        while (obj->children[child_index] != NULL){
            jsonObjDump(obj->children[child_index]);
            child_index++;
        }

        logPrint(LOG_DEBUG, "}\n");
    }
    else {
        logPrint(LOG_DEBUG, "%s, \n", obj->value);
    }
}

static void jsonDumpGraph(json_obj_t * root_node)
{
    assert(root_node);

    const int  IMG_WIDTH_IN_PERCENTS = 95;
    const int IMG_HEIGTH_IN_PERCENTS = 70;

    static size_t dump_count = 0;

    const size_t MAX_FILE_NAME = 256;
    char dot_file_name[MAX_FILE_NAME] = "";
    char img_file_name[MAX_FILE_NAME] = "";

    system("mkdir -p logs/dots/");
    system("mkdir -p logs/imgs/");

    sprintf(dot_file_name, "logs/dots/graph_%zu.dot", dump_count);
    sprintf(img_file_name, "logs/imgs/graph_%zu.svg", dump_count);

    FILE * dot_file = fopen(dot_file_name, "w");
    jsonMakeDot(dot_file, root_node);
    fclose(dot_file);

    char sys_dot_cmd[MAX_FILE_NAME] = "";
    sprintf(sys_dot_cmd, "dot %s -Tsvg -o %s", dot_file_name, img_file_name);
    system(sys_dot_cmd);

    char img_file_name_log[MAX_FILE_NAME] = "";
    sprintf(img_file_name_log, "imgs/graph_%zu.svg", dump_count);
    logPrint(LOG_DEBUG, "<img src = %s width = \"%d%%\" height = \"%d%%\">",
                        img_file_name_log,
                        IMG_WIDTH_IN_PERCENTS,
                        IMG_HEIGTH_IN_PERCENTS);

    logPrint(LOG_DEBUG, "<hr>");

    dump_count++;
}

const char * const NODE_COLOR = "#FFFFAA";
// const char * const  LEFT_COLOR = "#AAFFAA";
// const char * const RIGHT_COLOR = "#FFAAAA";

void jsonMakeDot(FILE * dot_file, json_obj_t * node)
{
    assert(node);
    assert(dot_file);

    fprintf(dot_file, "digraph {\n");
    fprintf(dot_file, "node [style=filled,color=\"#000000\"];\n");

    nodeMakeDot(dot_file, node, NULL);

    fprintf(dot_file, "}\n");
}

static void nodeMakeDot(FILE * dot_file, json_obj_t * node, json_obj_t * parent)
{
    assert(node);
    assert(dot_file);

    size_t node_num = (size_t )node;

    if (node->children == NULL)
        fprintf(dot_file, "node_%zu[shape=Mrecord,label=\"{\\\"%s\\\"|\\\"%s\\\"}\",fillcolor=\"%s\"];\n", node_num, node->name, node->value, NODE_COLOR);

    else {
        fprintf(dot_file, "node_%zu[shape=Mrecord,label=\"{'%s'}\",fillcolor=\"%s\"];\n", node_num, node->name, NODE_COLOR);

        size_t child_index = 0;
        while (node->children[child_index] != NULL){
            nodeMakeDot(dot_file, node->children[child_index], node);
        }
    }

    if (parent != NULL)
        fprintf(dot_file, "node_%zu->node_%zu;\n");
}
