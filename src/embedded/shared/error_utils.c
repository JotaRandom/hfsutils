/*
 * error_utils.c - Error handling and reporting utilities
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "error_utils.h"

/* Global error state */
static int verbose_mode = 0;
static const char *program_name = "hfsutils";
static FILE *error_log = NULL;

/*
 * NAME:    error_set_program_name()
 * DESCRIPTION: Set program name for error messages
 */
void error_set_program_name(const char *name)
{
    program_name = name ? name : "hfsutils";
}

/*
 * NAME:    error_set_verbose()
 * DESCRIPTION: Enable or disable verbose error reporting
 */
void error_set_verbose(int verbose)
{
    verbose_mode = verbose;
}

/*
 * NAME:    error_get_verbose()
 * DESCRIPTION: Get current verbose mode setting
 */
int error_get_verbose(void)
{
    return verbose_mode;
}

/*
 * NAME:    error_init_log()
 * DESCRIPTION: Initialize error logging to file
 */
int error_init_log(const char *log_path)
{
    if (error_log && error_log != stderr) {
        fclose(error_log);
    }
    
    if (!log_path) {
        error_log = NULL;
        return 0;
    }
    
    error_log = fopen(log_path, "a");
    if (!error_log) {
        error_log = stderr;
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    error_cleanup_log()
 * DESCRIPTION: Close error log file
 */
void error_cleanup_log(void)
{
    if (error_log && error_log != stderr) {
        fclose(error_log);
        error_log = NULL;
    }
}

/*
 * NAME:    error_print()
 * DESCRIPTION: Print error message with program name
 */
void error_print(const char *format, ...)
{
    va_list args;
    
    fprintf(stderr, "%s: ", program_name);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    /* Also log to file if enabled */
    if (error_log && error_log != stderr) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  /* Remove newline */
        }
        
        fprintf(error_log, "[%s] %s: ", time_str ? time_str : "unknown", program_name);
        
        va_start(args, format);
        vfprintf(error_log, format, args);
        va_end(args);
        
        fprintf(error_log, "\n");
        fflush(error_log);
    }
}

/*
 * NAME:    error_print_errno()
 * DESCRIPTION: Print error message with errno description
 */
void error_print_errno(const char *format, ...)
{
    va_list args;
    int saved_errno = errno;
    
    fprintf(stderr, "%s: ", program_name);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, ": %s\n", strerror(saved_errno));
    
    /* Also log to file if enabled */
    if (error_log && error_log != stderr) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  /* Remove newline */
        }
        
        fprintf(error_log, "[%s] %s: ", time_str ? time_str : "unknown", program_name);
        
        va_start(args, format);
        vfprintf(error_log, format, args);
        va_end(args);
        
        fprintf(error_log, ": %s\n", strerror(saved_errno));
        fflush(error_log);
    }
}

/*
 * NAME:    error_verbose()
 * DESCRIPTION: Print verbose message if verbose mode is enabled
 */
void error_verbose(const char *format, ...)
{
    va_list args;
    
    if (!verbose_mode) {
        return;
    }
    
    fprintf(stderr, "%s: ", program_name);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    /* Also log to file if enabled */
    if (error_log && error_log != stderr) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  /* Remove newline */
        }
        
        fprintf(error_log, "[%s] %s: ", time_str ? time_str : "unknown", program_name);
        
        va_start(args, format);
        vfprintf(error_log, format, args);
        va_end(args);
        
        fprintf(error_log, "\n");
        fflush(error_log);
    }
}

/*
 * NAME:    error_warning()
 * DESCRIPTION: Print warning message
 */
void error_warning(const char *format, ...)
{
    va_list args;
    
    fprintf(stderr, "%s: warning: ", program_name);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    /* Also log to file if enabled */
    if (error_log && error_log != stderr) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  /* Remove newline */
        }
        
        fprintf(error_log, "[%s] %s: warning: ", time_str ? time_str : "unknown", program_name);
        
        va_start(args, format);
        vfprintf(error_log, format, args);
        va_end(args);
        
        fprintf(error_log, "\n");
        fflush(error_log);
    }
}

/*
 * NAME:    error_fatal()
 * DESCRIPTION: Print fatal error message and exit
 */
void error_fatal(int exit_code, const char *format, ...)
{
    va_list args;
    
    fprintf(stderr, "%s: fatal: ", program_name);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    /* Also log to file if enabled */
    if (error_log && error_log != stderr) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  /* Remove newline */
        }
        
        fprintf(error_log, "[%s] %s: fatal: ", time_str ? time_str : "unknown", program_name);
        
        va_start(args, format);
        vfprintf(error_log, format, args);
        va_end(args);
        
        fprintf(error_log, "\n");
        fflush(error_log);
    }
    
    error_cleanup_log();
    exit(exit_code);
}

/*
 * NAME:    error_usage()
 * DESCRIPTION: Print usage message and exit
 */
void error_usage(const char *usage_text)
{
    fprintf(stderr, "Usage: %s %s\n", program_name, usage_text ? usage_text : "[options]");
    exit(EXIT_USAGE_ERROR);
}

/*
 * NAME:    error_get_exit_code()
 * DESCRIPTION: Convert errno to appropriate exit code
 */
int error_get_exit_code(int error_num)
{
    switch (error_num) {
        case 0:
            return EXIT_SUCCESS;
        case ENOENT:
        case ENOTDIR:
            return EXIT_OPERATIONAL_ERROR;
        case EACCES:
        case EPERM:
            return EXIT_OPERATIONAL_ERROR;
        case ENOSPC:
        case EDQUOT:
            return EXIT_OPERATIONAL_ERROR;
        case EINVAL:
        case ENOTBLK:
            return EXIT_USAGE_ERROR;
        case EBUSY:
            return EXIT_OPERATIONAL_ERROR;
        default:
            return EXIT_SYSTEM_ERROR;
    }
}