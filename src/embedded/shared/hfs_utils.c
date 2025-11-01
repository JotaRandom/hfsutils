/*
 * hfs_utils.c - Additional HFS utility functions
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
#include <time.h>
#include <stdarg.h>

#include "hfs_detect.h"

/* hfsck option flags */
#define HFSCK_REPAIR      0x0001
#define HFSCK_VERBOSE     0x0100
#define HFSCK_YES         0x0200

/* Global options variable for compatibility */
extern int options;

/* d_mtime and d_ltime are already defined in stubs.c */

/*
 * NAME:    ask()
 * DESCRIPTION: Ask user a question and get yes/no response
 */
int ask(const char *question, ...)
{
    va_list args;
    char answer[80];
    int result = -1;
    
    va_start(args, question);
    vprintf(question, args);
    va_end(args);
    
    /* If not in repair mode, just report the issue */
    if (!(options & HFSCK_REPAIR)) {
        printf(".\n");
        return 0;
    }
    
    /* If YES flag is set, automatically answer yes */
    if (options & HFSCK_YES) {
        printf(": fixing.\n");
        return 1;
    }
    
    /* Interactive mode - ask user */
    while (result == -1) {
        printf(". Fix? ");
        fflush(stdout);
        
        if (fgets(answer, sizeof(answer), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                return 0;
            }
            return 0;
        }
        
        switch (answer[0]) {
            case 'y':
            case 'Y':
                result = 1;
                break;
            case 'n':
            case 'N':
                result = 0;
                break;
            default:
                printf("Please answer 'y' or 'n': ");
                break;
        }
    }
    
    return result;
}

/* Global options variable for compatibility - defined in hfs_check.c */