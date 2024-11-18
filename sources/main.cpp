#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "logger.h"
#include "parser.h"

const char * LOG_FOLDER    = "logs";
const char * LOG_FILE_NAME = "logs/log.html";

int main()
{
    mkdir(LOG_FOLDER, S_IFDIR);
    logStart(LOG_FILE_NAME, LOG_DEBUG_PLUS, LOG_HTML);

    FILE * json_file = fopen("test.json", "r");

    json_obj_t * main_obj = parseJSON(json_file);

    jsonObjDtor(main_obj);

    logExit();

    return 0;
}
