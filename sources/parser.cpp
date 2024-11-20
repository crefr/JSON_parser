#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "parser.h"
#include "logger.h"

const size_t MAX_CHILDREN_NUM = 10;

static void nodeMakeDot(FILE * dot_file, json_obj_t * node, json_obj_t * parent);

static json_obj_t * parseObj(FILE * json_file, const char * name);

static void jsonDumpGraph(json_obj_t * root_node);

static void formatStrForDot(char * dest, const char * src);

json_obj_t * parseJSON(FILE * json_file)
{
    assert(json_file);

    json_obj_t * main_obj = parseObj(json_file, "main");

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

static json_obj_t * parseObj(FILE * json_file, const char * name)
{
    assert(json_file);
    assert(name);

    json_obj_t * new_obj = (json_obj_t *)calloc(1, sizeof(*new_obj));

    new_obj->children = NULL;
    strcpy(new_obj->name, name);

    char buffer[MAX_ARG_LEN] = "";
    size_t count = 0;

    int shift = 0;
    fscanf(json_file, " { %n", &shift);

    if (shift > 0){
        size_t child_capacity = START_CHILD_NUM;
        new_obj->children = (json_obj_t **)calloc(START_CHILD_NUM, sizeof(json_obj_t *));

        size_t child_index = 0;

        bool in_brackets = true;
        while (in_brackets){
            shift = 0;
            fscanf(json_file, " } %n", &shift);

            if (shift > 0){
                in_brackets = false;
                break;
            }

            fscanf(json_file, " \"%[^\"]\" : ", buffer);
            new_obj->children[child_index] = parseObj(json_file, buffer);

            child_index++;

            if (child_index >= child_capacity - 1){
                child_capacity *= CAP_MULTIPLIER;
                new_obj->children = (json_obj_t **)realloc(new_obj->children, sizeof(*(new_obj->children)) * child_capacity);
                memset(new_obj->children + child_index, 0, (child_capacity - child_index) * sizeof(*(new_obj->children)));
            }
        }
    }
    else {
        if (fscanf(json_file, " \"%[^\"]\" ", new_obj->value) <= 0)
            fscanf(json_file, " %[^,}] ", new_obj->value);
    }

    fscanf(json_file, " %*[,] ");

    return new_obj;
}

json_obj_t * findObject(json_obj_t * root, const char * sample)
{
    if (strcmp(sample, root->name) == 0)
        return root;

    if (root->children != NULL){
        size_t child_index = 0;
        while (root->children[child_index] != NULL){
            json_obj_t * searched = findObject(root->children[child_index], sample);
            if (searched != NULL)
                return searched;
            child_index++;
        }
    }
    return NULL;
}

void jsonDump(json_obj_t * obj)
{
    assert(obj);

    logPrint(LOG_DEBUG, "--------JSON_PARSER_DUMP--------\n");

    jsonObjDump(obj);
    jsonDumpGraph(obj);

    logPrint(LOG_DEBUG, "------JSON_PARSER_DUMP_END------\n");
}

void jsonObjDump(json_obj_t * obj)
{
    assert(obj);

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

const char * const ROOT_COLOR = "#FFFFAA";
const char * const LEAF_COLOR = "#AAFFAA";

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

    if (node->children == NULL){
        char value_str[MAX_ARG_LEN] = "";
        formatStrForDot(value_str, node->value);

        fprintf(dot_file, "node_%zu[shape=Mrecord,label=\"{\\\"%s\\\"|%s}\",fillcolor=\"%s\"];\n", node_num, node->name, value_str, LEAF_COLOR);
    }

    else {
        fprintf(dot_file, "node_%zu[shape=Mrecord,label=\"{\\\"%s\\\"}\",fillcolor=\"%s\"];\n", node_num, node->name, ROOT_COLOR);

        size_t child_index = 0;
        while (node->children[child_index] != NULL){
            nodeMakeDot(dot_file, node->children[child_index], node);
            child_index++;
        }
    }
    if (parent != NULL)
        fprintf(dot_file, "node_%zu->node_%zu;\n", (size_t)parent, node_num);
}

static void formatStrForDot(char * dest, const char * src)
{
    assert(dest);
    assert(src);

    while (*src != '\0'){
        switch (*src){
            case '\"':
                *dest = '\\';
                dest++;
                *dest = '\"';
                break;

            case '\\':
                *dest = '\\';
                dest++;
                *dest = '\\';
                break;

            case '\n':
                *dest = '\\';
                dest++;
                *dest = 'n';
                break;

            default:
                *dest = *src;
                break;
        }
        dest++;
        src ++;
    }
}

