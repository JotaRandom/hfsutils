/*
 * test_mkfs_common.c - Unit tests for mkfs common functionality
 * Copyright (C) 2025 Pablo Lezaeta
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../../src/mkfs/mkfs_common.h"

/* Test parse_size_common function */
void test_parse_size_common(void)
{
    printf("Testing parse_size_common...\n");
    
    /* Test basic numbers */
    assert(parse_size_common("1024") == 1024);
    assert(parse_size_common("0") == -1);  /* Invalid */
    assert(parse_size_common("-1") == -1); /* Invalid */
    
    /* Test K suffix */
    assert(parse_size_common("1K") == 1024);
    assert(parse_size_common("1k") == 1024);
    assert(parse_size_common("10K") == 10240);
    
    /* Test M suffix */
    assert(parse_size_common("1M") == 1024 * 1024);
    assert(parse_size_common("1m") == 1024 * 1024);
    assert(parse_size_common("5M") == 5 * 1024 * 1024);
    
    /* Test G suffix */
    assert(parse_size_common("1G") == 1024LL * 1024 * 1024);
    assert(parse_size_common("1g") == 1024LL * 1024 * 1024);
    
    /* Test invalid inputs */
    assert(parse_size_common(NULL) == -1);
    assert(parse_size_common("") == -1);
    assert(parse_size_common("abc") == -1);
    assert(parse_size_common("1X") == -1);  /* Invalid suffix */
    assert(parse_size_common("1KB") == -1); /* Extra characters */
    
    printf("parse_size_common tests passed\n");
}

/* Test volume name validation */
void test_validate_volume_names(void)
{
    printf("Testing volume name validation...\n");
    
    /* Test HFS volume names */
    assert(validate_volume_name_hfs("Test") == 0);
    assert(validate_volume_name_hfs("A") == 0);
    assert(validate_volume_name_hfs("1234567890123456789012345678") == -1); /* Too long */
    assert(validate_volume_name_hfs("Test:Name") == -1); /* Contains colon */
    assert(validate_volume_name_hfs("") == -1); /* Empty */
    assert(validate_volume_name_hfs(NULL) == 0); /* NULL is OK */
    
    /* Test HFS+ volume names */
    assert(validate_volume_name_hfsplus("Test") == 0);
    assert(validate_volume_name_hfsplus("A") == 0);
    /* Create a 256-character string */
    char long_name[257];
    memset(long_name, 'A', 256);
    long_name[256] = '\0';
    assert(validate_volume_name_hfsplus(long_name) == -1); /* Too long */
    assert(validate_volume_name_hfsplus("Test:Name") == -1); /* Contains colon */
    assert(validate_volume_name_hfsplus("") == -1); /* Empty */
    assert(validate_volume_name_hfsplus(NULL) == 0); /* NULL is OK */
    
    printf("Volume name validation tests passed\n");
}

int main(void)
{
    printf("Running mkfs common functionality tests...\n\n");
    
    test_parse_size_common();
    test_validate_volume_names();
    
    printf("\nAll tests passed!\n");
    return 0;
}