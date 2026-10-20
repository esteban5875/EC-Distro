#include <dirent.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "./tools-common/constants.h"

int main(int argc, char* const argv[])
{
    if (argc > 2){
        errLine("%s: Too many args", argv[0]);
        return 1;
    }
    
    const char* dir_path = (argc == 1) ? "." : argv[1];
    DIR *dir_stream = opendir(dir_path);

    if (dir_stream == NULL) {
        char *sys_err = strerror(errno);
        errLine("ls: %s: %s", dir_path, sys_err);
        return EXIT_FAILURE;
    }

    struct dirent *entry;

    while ((entry = readdir(dir_stream)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_DIR) {
            printLine("[FOLDER] /%s", entry->d_name);
        } 
        else if (entry->d_type == DT_REG) {
            printLine("[FILE] %s", entry->d_name);
        } 
        else {
            printLine("[OTHER] %s", entry->d_name);
        }
    }
    closedir(dir_stream);
    return 0;
}
