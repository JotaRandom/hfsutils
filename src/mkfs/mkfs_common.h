/*
 * mkfs_common.h - Common functions shared between mkfs.hfs and mkfs.hfs+
 * Copyright (C) 2025 Pablo Lezaeta
 */

#ifndef MKFS_COMMON_H
#define MKFS_COMMON_H

#include "../embedded/mkfs/mkfs_hfs.h"

/* Common function declarations */
void show_license_common(const char *program_name);
long long parse_size_common(const char *size_str);
int validate_volume_name_hfs(const char *name);
int validate_volume_name_hfsplus(const char *name);
int parse_command_line_common(int argc, char *argv[], mkfs_options_t *opts, int is_hfsplus);
int validate_options_common(mkfs_options_t *opts);
void cleanup_options_common(mkfs_options_t *opts);

/* Prefixed versions for compatibility */
void mkfs_show_license(const char *program_name);
long long mkfs_parse_size(const char *size_str, int is_hfsplus);
int mkfs_validate_volume_name(const char *name, int is_hfsplus);
int mkfs_parse_command_line(int argc, char *argv[], mkfs_options_t *opts, int is_hfsplus);
int mkfs_validate_options(mkfs_options_t *opts, int is_hfsplus);
void mkfs_cleanup_options(mkfs_options_t *opts);

#endif /* MKFS_COMMON_H */