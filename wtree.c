#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define FILENAME_MAXLEN 4096

#define PATH_PTR_ERR -1
#define GET_STAT_ERR -3
#define OPEN_DIR_ERR -5
#define MEM_ALLC_ERR -7
#define READLINK_ERR -9

#define FATAL_RED_BOLD   "\033[1;31m"
#define HIGH_GREEN_BOLD  "\033[1;32m"
#define HIGH_CYAN_BOLD   "\033[1;36m"
#define GENERAL_BOLD     "\033[1m"
#define RESET_DISPLAY    "\033[0m"
#define GREY_LIGHT       "\033[2;37m"

size_t num_of_dirs = 0;
size_t num_of_files = 0;

int dir_tree(char *path_prefix, char *file_name, size_t depth) {
    if(path_prefix == NULL || file_name == NULL) {
        return PATH_PTR_ERR;
    }
    size_t prefix_len = strlen(path_prefix);
    size_t filename_len = strlen(file_name);
    size_t full_path_len = prefix_len + filename_len + 2;
    char lnk_target_file[FILENAME_MAXLEN] = "";
    char *print_prefix = (char *)calloc(depth * 4 + 1, sizeof(char));
    char *p_file_name;
    if(print_prefix == NULL) {
        return MEM_ALLC_ERR;
    }
    memset(print_prefix, ' ', depth * 4);
    if(depth != 0) {
        strncpy(print_prefix + depth * 4 - 4, "+---", 4);
    }
    char *full_path = (char *)calloc(full_path_len, sizeof(char));
    if(full_path == NULL) {
        free(print_prefix);
        return MEM_ALLC_ERR;
    }
    if(depth != 0) {
        snprintf(full_path, full_path_len, "%s/%s", path_prefix, file_name);
        p_file_name = file_name;
    }
    else {
        strncpy(full_path, path_prefix, full_path_len - 1);
        p_file_name = path_prefix;
    }
    struct stat path_stat;
    struct stat lnk_file_stat;
    struct dirent *entry = NULL;
    if(lstat(full_path, &path_stat) == -1) {
        free(print_prefix);
        free(full_path);
        printf(GREY_LIGHT "%s" RESET_DISPLAY FATAL_RED_BOLD "!!!STAT_ERROR!!! %s" RESET_DISPLAY "\n", print_prefix, p_file_name);
        return GET_STAT_ERR;
    }
    if(!S_ISDIR(path_stat.st_mode)) {
        if(S_ISLNK(path_stat.st_mode)) {
            if(readlink(full_path, lnk_target_file, FILENAME_MAXLEN - 1) == -1 || lstat(lnk_target_file, &lnk_file_stat) == -1) {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY FATAL_RED_BOLD " -> !!!INVALID_TARGET!!!" RESET_DISPLAY "\n", print_prefix, p_file_name);
                free(print_prefix);
                free(full_path);
                num_of_files++;
                return READLINK_ERR;
            }
            if(lnk_file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY HIGH_GREEN_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name, lnk_target_file);
            }
            else {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY "%s\n", print_prefix, p_file_name, lnk_target_file);
            }
            num_of_files++;
        }
        else {
            if(path_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_GREEN_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name);
            }
            else {
                printf(GREY_LIGHT "%s" RESET_DISPLAY "%s\n", print_prefix, p_file_name);
            }
            num_of_files++;
        }
        free(print_prefix);
        free(full_path);
        return 0;
    }
    DIR *dir = opendir(full_path);
    if(dir == NULL) {
        free(print_prefix);
        free(full_path);
        printf("%s*!!!OPENDIR_ERROR!!!*%s\n", print_prefix, p_file_name);
        return OPEN_DIR_ERR;
    }
    if(depth == 0) {
        printf(GENERAL_BOLD "%s" RESET_DISPLAY "\n", full_path);
    }
    else {
        printf(GREY_LIGHT "%s" RESET_DISPLAY GENERAL_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name);
        num_of_dirs++;
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        dir_tree(full_path, entry->d_name, depth + 1);
    }
    closedir(dir);
    free(print_prefix);
    free(full_path);
    return 0;
}

int main(int argc, char **argv) {
    int run_flag = 0;
    char *root_path = "./";
    if(argc > 1) {
        root_path = argv[1];
    }
    run_flag = dir_tree(root_path, "", 0);
    if(run_flag != 0) {
        printf("\nFAILED! ERROR_CODE: %d\n", run_flag);
    }
    else {
        printf("\n");
        (num_of_dirs > 1) ? (printf("%ld directories, ", num_of_dirs)) : (printf("%ld directory, ", num_of_dirs));
        (num_of_files > 1) ? (printf("%ld files\n", num_of_files)) : (printf("%ld file\n", num_of_files));
    }
    return (run_flag == 0) ? 0 : 1;
}
