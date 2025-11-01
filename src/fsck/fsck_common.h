/*
 * fsck_common.h - Common functions shared between fsck.hfs and fsck.hfs+
 * Copyright (C) 2025 Pablo Lezaeta
 */

#ifndef FSCK_COMMON_H
#define FSCK_COMMON_H

#include "../embedded/fsck/fsck_hfs.h"

/* Command-line options structure */
typedef struct {
    char *device_path;
    int partition_number;
    int repair;
    int verbose;
    int auto_repair;
    int force;
    int yes_to_all;
    int read_only;
    int show_version;
    int show_help;
    int show_license;
} fsck_options_t;

/* Common function declarations */
void fsck_show_license(const char *program_name);
int fsck_parse_command_line(int argc, char *argv[], fsck_options_t *opts);
int fsck_validate_options(fsck_options_t *opts);
void fsck_cleanup_options(fsck_options_t *opts);

#endif /* FSCK_COMMON_H */