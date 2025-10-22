#ifndef HFSPLUS_FORMAT_H
#define HFSPLUS_FORMAT_H

#include <stdint.h>
#include <time.h>
#include "../common/hfs_detect.h"

/* HFS+ specific constants */
#define HFSPLUS_SIGNATURE           0x482B      /* 'H+' */
#define HFSPLUS_VERSION             4           /* HFS+ version */
#define HFSPLUS_MIN_BLOCK_SIZE      512         /* Minimum allocation block size */
#define HFSPLUS_DEFAULT_BLOCK_SIZE  4096        /* Default allocation block size */
#define HFSPLUS_MAX_BLOCK_SIZE      65536       /* Maximum allocation block size */

/* HFS+ Volume attributes */
#define HFSPLUS_VOL_UNMNT           0x00000100  /* Volume was cleanly unmounted */
#define HFSPLUS_VOL_SPARE_BLK       0x00000200  /* Volume has bad blocks spared */
#define HFSPLUS_VOL_NOCACHE         0x00000400  /* Disable extent cache */
#define HFSPLUS_VOL_INCNSTNT        0x00000800  /* Volume is inconsistent */
#define HFSPLUS_VOL_JOURNALED       0x00002000  /* Volume is journaled */
#define HFSPLUS_VOL_SOFTLOCK        0x00008000  /* Volume is software locked */

/* HFS+ B-tree node types */
#define HFSPLUS_NODE_LEAF           0xFF        /* Leaf node */
#define HFSPLUS_NODE_INDEX          0x00        /* Index node */
#define HFSPLUS_NODE_HEADER         0x01        /* Header node */
#define HFSPLUS_NODE_MAP            0x02        /* Map node */

/* HFS+ Catalog record types */
#define HFSPLUS_FOLDER_RECORD       0x0001      /* Folder record */
#define HFSPLUS_FILE_RECORD         0x0002      /* File record */
#define HFSPLUS_FOLDER_THREAD       0x0003      /* Folder thread record */
#define HFSPLUS_FILE_THREAD         0x0004      /* File thread record */

/* HFS+ Fork types */
#define HFSPLUS_DATA_FORK           0x00        /* Data fork */
#define HFSPLUS_RESOURCE_FORK       0xFF        /* Resource fork */

/* HFS+ special file IDs */
#define HFSPLUS_ROOT_FOLDER_ID      2           /* Root folder */
#define HFSPLUS_EXTENTS_FILE_ID     3           /* Extents overflow file */
#define HFSPLUS_CATALOG_FILE_ID     4           /* Catalog file */
#define HFSPLUS_BAD_BLOCKS_FILE_ID  5           /* Bad blocks file */
#define HFSPLUS_ALLOCATION_FILE_ID  6           /* Allocation file */
#define HFSPLUS_STARTUP_FILE_ID     7           /* Startup file */
#define HFSPLUS_ATTRIBUTES_FILE_ID  8           /* Attributes file */
#define HFSPLUS_REPAIR_CATALOG_ID   14          /* Repair catalog file */
#define HFSPLUS_BOGUS_EXTENT_ID     15          /* Bogus extent file */
#define HFSPLUS_FIRST_USER_ID       16          /* First user file ID */

/* HFS+ structures are defined in hfs_detect.h */

/* HFS+ B-tree node descriptor */
struct hfsplus_btree_node_desc {
    uint32_t next;                              /* Next node number */
    uint32_t prev;                              /* Previous node number */
    int8_t type;                                /* Node type */
    uint8_t height;                             /* Node height */
    uint16_t num_recs;                          /* Number of records */
    uint16_t reserved;                          /* Reserved */
} __attribute__((packed));

/* HFS+ B-tree header record */
struct hfsplus_btree_header_rec {
    uint16_t tree_depth;                        /* Tree depth */
    uint32_t root_node;                         /* Root node number */
    uint32_t leaf_records;                      /* Number of leaf records */
    uint32_t first_leaf_node;                   /* First leaf node */
    uint32_t last_leaf_node;                    /* Last leaf node */
    uint16_t node_size;                         /* Node size */
    uint16_t max_key_len;                       /* Maximum key length */
    uint32_t total_nodes;                       /* Total nodes */
    uint32_t free_nodes;                        /* Free nodes */
    uint16_t reserved1;                         /* Reserved */
    uint32_t clump_size;                        /* Clump size */
    uint8_t btree_type;                         /* B-tree type */
    uint8_t key_compare_type;                   /* Key compare type */
    uint32_t attributes;                        /* B-tree attributes */
    uint32_t reserved3[16];                     /* Reserved */
} __attribute__((packed));

/* HFS+ Unicode string */
struct hfsplus_unistr {
    uint16_t length;                            /* String length */
    uint16_t unicode[255];                      /* Unicode characters */
} __attribute__((packed));

/* HFS+ Catalog key */
struct hfsplus_cat_key {
    uint16_t key_len;                           /* Key length */
    uint32_t parent_id;                         /* Parent catalog node ID */
    struct hfsplus_unistr name;                 /* Catalog node name */
} __attribute__((packed));

/* HFS+ format options */
typedef struct {
    char *device_path;                          /* Device to format */
    char *volume_name;                          /* Volume name */
    uint32_t block_size;                        /* Allocation block size */
    uint64_t total_size;                        /* Total volume size */
    int force;                                  /* Force format flag */
    int journal;                                /* Enable journaling */
    int case_sensitive;                         /* Case sensitive (HFSX) */
    int verbose;                                /* Verbose output */
} hfsplus_format_opts_t;

/* Function prototypes */
int hfsplus_format_volume(const hfsplus_format_opts_t *opts);
int hfsplus_create_volume_header(int fd, const hfsplus_format_opts_t *opts, 
                                struct hfsplus_vh *vh);
int hfsplus_create_allocation_file(int fd, const hfsplus_format_opts_t *opts,
                                  struct hfsplus_vh *vh);
int hfsplus_create_extents_file(int fd, const hfsplus_format_opts_t *opts,
                               struct hfsplus_vh *vh);
int hfsplus_create_catalog_file(int fd, const hfsplus_format_opts_t *opts,
                               struct hfsplus_vh *vh);
int hfsplus_create_attributes_file(int fd, const hfsplus_format_opts_t *opts,
                                  struct hfsplus_vh *vh);
int hfsplus_write_volume_header(int fd, const struct hfsplus_vh *vh);
int hfsplus_calculate_sizes(const hfsplus_format_opts_t *opts,
                           uint32_t *total_blocks, uint32_t *block_size);

/* Utility functions */
uint32_t hfsplus_get_optimal_block_size(uint64_t volume_size);
int hfsplus_validate_options(const hfsplus_format_opts_t *opts);
void hfsplus_init_btree_header(struct hfsplus_btree_header_rec *header,
                              uint16_t node_size, uint8_t btree_type);
int hfsplus_utf8_to_unicode(const char *utf8, struct hfsplus_unistr *unicode);
void hfsplus_set_dates(struct hfsplus_vh *vh);

#endif /* HFSPLUS_FORMAT_H */