/*
 * simple_test.c - Simple test for mkfs.hfs basic functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    printf("mkfs.hfs test program\n");
    printf("Version: 4.1.0A.1\n");
    
    if (argc < 2) {
        printf("Usage: %s device\n", argv[0]);
        return 2;
    }
    
    printf("Would format device: %s\n", argv[1]);
    printf("This is a test version - no actual formatting performed\n");
    
    return 0;
}