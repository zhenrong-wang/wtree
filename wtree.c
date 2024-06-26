#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#define NUM_CMD_OPTIONS 2

#define PATH_PTR_ERR -1
#define GET_STAT_ERR -3
#define OPEN_DIR_ERR -5
#define MEM_ALLC_ERR -7
#define READLINK_ERR -9

#define FATAL_RED_BOLD   "\033[1;31m"
#define HIGH_GREEN_BOLD  "\033[1;32m"
#define HIGH_CYAN_BOLD   "\033[1;4;36m"
#define GENERAL_BOLD     "\033[1m"
#define RESET_DISPLAY    "\033[0m"
#define GREY_LIGHT       "\033[2;37m"
#define HIGH_BLUE_BOLD   "\033[1;34m"
#define WARN_YELLOW      "\033[1;33m"

const char cmd_flags[NUM_CMD_OPTIONS][16] = {
    "-a",
    "-l"
};

struct lnk_node {
    char lnk_target[FILENAME_MAX];
    struct lnk_node *p_next;
};

size_t num_of_dirs = 0;
size_t num_of_files = 0;
struct lnk_node *head = NULL;
int show_lnk_dirs = 0;
int show_all_files = 0;

int check_list(struct lnk_node *head, char *lnk_target) {
    if(head == NULL) {
        return 0;
    }
    if(lnk_target == NULL) {
        return -1;
    }
    struct lnk_node *ptr = head;
    while(ptr != NULL) {
        if(strcmp(ptr->lnk_target, lnk_target) == 0) {
            return 1;
        }
        ptr = ptr->p_next;
    }
    return 0;
}

int push_to_list(struct lnk_node **head, char *lnk_target) {
    if(head == NULL || lnk_target == NULL) {
        return -1;
    }
    struct lnk_node *new_node = (struct lnk_node *)calloc(1, sizeof(struct lnk_node));
    if(new_node == NULL) {
        return -3;
    }
    strncpy(new_node->lnk_target, lnk_target, FILENAME_MAX - 1);
    if(*head == NULL) {
        *head = new_node;
        return 0;
    }
    new_node->p_next = *head;
    *head = new_node;
    return 0;
}

void free_list(struct lnk_node *head) {
    struct lnk_node *ptr = head;
    struct lnk_node *ptr_next;
    while(ptr != NULL) {
        ptr_next = ptr->p_next;
        free(ptr);
        ptr = ptr_next;
    }
}

int get_lnk_target_path(const char *lnk_name, char lnk_target[], char lnk_target_abs[], size_t max_len) {
    if(lnk_name == NULL) {
        return PATH_PTR_ERR;
    }
    char lnk_dir[FILENAME_MAX] = "";
    char lnk_target_buffer[FILENAME_MAX] = "";
    memset(lnk_dir, '\0', FILENAME_MAX);
    memset(lnk_target, '\0', max_len);
    if(readlink(lnk_name, lnk_target, max_len - 1) == -1 || realpath(dirname(strdup(lnk_name)), lnk_dir) == NULL) {
        return READLINK_ERR;
    }
    memset(lnk_target_abs, '\0', max_len);
    if(lnk_target[0] != '/') {
        snprintf(lnk_target_buffer, FILENAME_MAX, "%s/%s", lnk_dir, lnk_target);
        return ((realpath(lnk_target_buffer, lnk_target_abs) == NULL) ? READLINK_ERR : 0); 
    }
    else {
        return ((realpath(lnk_target, lnk_target_abs) == NULL) ? READLINK_ERR : 0); 
    }
}

int wtree(char *path_prefix, char *file_name, size_t depth, int lnk_dir_flag) {
    if(path_prefix == NULL || file_name == NULL) {
        return PATH_PTR_ERR;
    }
    size_t prefix_len = strlen(path_prefix);
    size_t filename_len = strlen(file_name);
    size_t full_path_len = prefix_len + filename_len + 2;
    char lnk_target[FILENAME_MAX] = "";
    char lnk_target_abs[FILENAME_MAX] = "";
    char *print_prefix = (char *)calloc(depth * 4 + 1, sizeof(char));
    char *p_file_name;
    if(print_prefix == NULL) {
        return MEM_ALLC_ERR;
    }
    memset(print_prefix, ' ', depth * 4);
    if(depth != 0) {
        strncpy(print_prefix + depth * 4 - 4, "+---", 4);
        for(size_t i = 0; i < depth - 1; i++) {
            strncpy(print_prefix + i * 4, "|   ", 4);
        }
    }
    char *full_path = (char *)calloc(full_path_len, sizeof(char));
    if(full_path == NULL) {
        free(print_prefix);
        return MEM_ALLC_ERR;
    }
    if(depth != 0) {
        snprintf(full_path, full_path_len, "%s/%s", path_prefix, file_name);
        p_file_name = file_name;
        if(!show_all_files && p_file_name[0] == '.' && strlen(p_file_name) > 1) {
            return 0;
        }
    }
    else {
        strncpy(full_path, path_prefix, full_path_len - 1);
        p_file_name = path_prefix;
        push_to_list(&head, full_path);
    }

    struct stat path_stat;
    struct stat lnk_file_stat;
    struct dirent *entry = NULL;

    if(lstat(full_path, &path_stat) == -1) {
        free(print_prefix);
        free(full_path);
        printf(GREY_LIGHT "%s" RESET_DISPLAY FATAL_RED_BOLD "%s" RESET_DISPLAY WARN_YELLOW " [invalid file or dir]" RESET_DISPLAY "\n", print_prefix, p_file_name);
        return GET_STAT_ERR;
    }
    if(!S_ISDIR(path_stat.st_mode)) {
        if(S_ISLNK(path_stat.st_mode)) {
            if(get_lnk_target_path(full_path, lnk_target, lnk_target_abs, FILENAME_MAX) != 0 || lstat(lnk_target_abs, &lnk_file_stat) == -1) {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY FATAL_RED_BOLD " -> %s" RESET_DISPLAY WARN_YELLOW " [invalid target]" RESET_DISPLAY "\n", print_prefix, p_file_name, lnk_target);
                free(print_prefix);
                free(full_path);
                num_of_files++;
                return READLINK_ERR;
            }
            if(!S_ISDIR(lnk_file_stat.st_mode)) {
                if(S_ISLNK(lnk_file_stat.st_mode)) {
                    printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name, lnk_target);
                }
                else if(lnk_file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                    printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY HIGH_GREEN_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name, lnk_target);
                }
                else {
                    printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY "%s\n", print_prefix, p_file_name, lnk_target);
                }
                num_of_files++;
            }
            else if(show_lnk_dirs){
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY HIGH_BLUE_BOLD "%s" RESET_DISPLAY , print_prefix, p_file_name, lnk_target);
                int check_flag = check_list(head, lnk_target_abs);
                if (check_flag == 1) {
                    printf(WARN_YELLOW " [recursive, not followed]" RESET_DISPLAY "\n");
                    num_of_dirs++;
                }
                else if (check_flag == 0){
                    push_to_list(&head, lnk_target_abs);
                    printf("\n");
                    wtree(lnk_target_abs, "", depth, 1);
                    num_of_dirs++;
                }
                else {
                    free(print_prefix);
                    free(full_path);
                    return MEM_ALLC_ERR;
                }
            }
            else {
                printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_CYAN_BOLD "%s" RESET_DISPLAY GREY_LIGHT " -> " RESET_DISPLAY HIGH_BLUE_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name, lnk_target);
                num_of_dirs++;
            }
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
        printf(GREY_LIGHT "%s" RESET_DISPLAY FATAL_RED_BOLD "%s" RESET_DISPLAY WARN_YELLOW " [failed to open dir]" RESET_DISPLAY "\n", print_prefix, p_file_name);
        return OPEN_DIR_ERR;
    }
    if(lnk_dir_flag == 0) {
        if(depth == 0) {
            printf(HIGH_BLUE_BOLD "%s" RESET_DISPLAY "\n", full_path);
        }
        else {
            printf(GREY_LIGHT "%s" RESET_DISPLAY HIGH_BLUE_BOLD "%s" RESET_DISPLAY "\n", print_prefix, p_file_name);
            num_of_dirs++;
        }
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        wtree(full_path, entry->d_name, depth + 1, 0);
    }
    closedir(dir);
    free(print_prefix);
    free(full_path);
    return 0;
}

int cmd_flag_check(int argc, char **argv, char *flag) {
    if(flag == NULL) {
        return -1;
    }
    for(int i = 1; i < argc; i++) {
        if(strcmp(flag, argv[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

int is_cmd_flag(char *argv) {
    if(argv == NULL) {
        return -1;
    }
    for(int i = 0; i < NUM_CMD_OPTIONS; i++) {
        if(strcmp(argv, cmd_flags[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void get_target_path (int argc, char **argv, char target_path[], int max_len) {
    memset(target_path, '\0', max_len);
    for(int i = 1; i < argc; i++) {
        if(!is_cmd_flag(argv[i]) && strcmp(argv[i], "./") != 0) {
            strncpy(target_path, argv[i], max_len - 1);
            return;
        }
    }
    strncpy(target_path, ".", 1);
}

int main(int argc, char **argv) {
    int run_flag = 0;
    char target_path[FILENAME_MAX] = "";
    get_target_path(argc, argv, target_path, FILENAME_MAX);

    show_all_files = (cmd_flag_check(argc, argv, "-a") == 0) ? 1 : 0;
    show_lnk_dirs = (cmd_flag_check(argc, argv, "-l") == 0) ? 1 : 0;

    run_flag = wtree(target_path, "", 0, 0);
    
    printf("\n");
    (num_of_dirs > 1) ? (printf("%ld directories, ", num_of_dirs)) : (printf("%ld directory, ", num_of_dirs));
    (num_of_files > 1) ? (printf("%ld files\n", num_of_files)) : (printf("%ld file\n", num_of_files));

    if(run_flag != 0) {
        printf(WARN_YELLOW "error occured. code: %d" RESET_DISPLAY "\n", run_flag);
    }
    free_list(head);
    return (run_flag == 0) ? 0 : 1;
}
