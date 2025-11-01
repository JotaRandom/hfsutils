/*
 * error_utils.h - Error handling and reporting utilities
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ERROR_UTILS_H
#define ERROR_UTILS_H

/* Standard exit codes for filesystem utilities */
#define EXIT_SUCCESS            0   /* No errors */
#define EXIT_OPERATIONAL_ERROR  1   /* Operational error */
#define EXIT_USAGE_ERROR        2   /* Usage or syntax error */
#define EXIT_SYSTEM_ERROR       4   /* System error */
#define EXIT_LIBRARY_ERROR      8   /* Library error */
#define EXIT_USER_CANCEL        16  /* User cancelled */
#define EXIT_UNCORRECTED_ERRORS 32  /* Uncorrected errors found */

/* Error reporting functions */
void error_set_program_name(const char *name);
void error_set_verbose(int verbose);
int error_get_verbose(void);

/* Error logging */
int error_init_log(const char *log_path);
void error_cleanup_log(void);

/* Error printing functions */
void error_print(const char *format, ...);
void error_print_errno(const char *format, ...);
void error_verbose(const char *format, ...);
void error_warning(const char *format, ...);
void error_fatal(int exit_code, const char *format, ...);
void error_usage(const char *usage_text);

/* Utility functions */
int error_get_exit_code(int error_num);

#endif /* ERROR_UTILS_H */