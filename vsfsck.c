#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define FS_BLOCK_SIZE 4096
#define FS_BLOCK_COUNT 64
#define FS_INODE_SIZE 256
#define FS_MAX_INODES 80


#define SUPERBLOCK_IDX 0
#define INODE_BITMAP_IDX 1
#define DATA_BITMAP_IDX 2
#define INODE_TABLE_START_IDX 3 //3 theke 7 number block porjonto inode table
#define DATA_START_IDX 8 // 8 theke 63 data blocks
#define DATA_END_IDX 63


#define FS_MAGIC 0xd34d


typedef struct {
    uint16_t magic;               
    uint16_t padding;             
    uint32_t block_size;         
    uint32_t block_count;        
    uint32_t inode_bitmap_loc;    
    uint32_t data_bitmap_loc;    
    uint32_t inode_start;        
    uint32_t data_start;          
    uint32_t inode_size;          
    uint32_t inode_count;         
    char unused[4096 - 36];      //remaining space of 4kb
} __attribute__((packed)) SuperBlock;


typedef struct {
    uint32_t mode;             
    uint32_t uid;                 
    uint32_t gid;               
    uint32_t size;               
    uint32_t atime;            
    uint32_t ctime;              
    uint32_t mtime;              
    uint32_t dtime;               
    uint32_t links_count;        
    uint32_t blocks;              
    uint32_t direct_pointer;      
    uint32_t indirect_pointer;   
    uint32_t double_indirect;     
    uint32_t triple_indirect;    
    char padding[156];            
} __attribute__((packed)) Inode;


uint8_t inode_bmap[FS_BLOCK_SIZE];  
uint8_t data_bmap[FS_BLOCK_SIZE];  
SuperBlock sb;                     
Inode inodes[FS_MAX_INODES];        
int repair_mode = 0;              


void read_fs_block(FILE *image, int block_num, void *buffer) {
    long offset = (long)block_num * FS_BLOCK_SIZE;
    fseek(image, offset, SEEK_SET);
    size_t bytes_read = fread(buffer, 1, FS_BLOCK_SIZE, image);
    
    if (bytes_read != FS_BLOCK_SIZE) {
        fprintf(stderr, "Warning: Read only %ld bytes from block %d\n", 
                bytes_read, block_num);
    }
}


void write_fs_block(FILE *image, int block_num, void *buffer) {
    long offset = (long)block_num * FS_BLOCK_SIZE;
    fseek(image, offset, SEEK_SET);
    size_t bytes_written = fwrite(buffer, 1, FS_BLOCK_SIZE, image);
    
    if (bytes_written != FS_BLOCK_SIZE) {
        fprintf(stderr, "Error: Wrote only %ld bytes to block %d\n", 
                bytes_written, block_num);
    }
}



void verify_superblock(FILE *image) {
    printf("Checking superblock information...\n");
    
  
    read_fs_block(image, SUPERBLOCK_IDX, &sb);
    
  
    if (sb.magic != FS_MAGIC) 
    {
        printf("Error: Invalid magic number 0x%x (expected 0x%x)\n", 
               sb.magic, FS_MAGIC);
    }
    
 
    if (sb.block_size != FS_BLOCK_SIZE) 
    {
        printf("Error: Block size mismatch: %d (expected %d)\n", 
               sb.block_size, FS_BLOCK_SIZE);
    }
    
    if (sb.block_count != FS_BLOCK_COUNT) 
    {
        printf("Error: Block count mismatch: %d (expected %d)\n", 
               sb.block_count, FS_BLOCK_COUNT);
    }
    

    if (sb.inode_bitmap_loc != INODE_BITMAP_IDX || 
        sb.data_bitmap_loc != DATA_BITMAP_IDX ||
        sb.inode_start != INODE_TABLE_START_IDX || 
        sb.data_start != DATA_START_IDX) 
        {
        printf("Error: Filesystem block layout is incorrect\n");
        printf("  Inode bitmap: %d (expected %d)\n", 
               sb.inode_bitmap_loc, INODE_BITMAP_IDX);
        printf("  Data bitmap: %d (expected %d)\n", 
               sb.data_bitmap_loc, DATA_BITMAP_IDX);
        printf("  Inode table: %d (expected %d)\n", 
               sb.inode_start, INODE_TABLE_START_IDX);
        printf("  Data blocks: %d (expected %d)\n", 
               sb.data_start, DATA_START_IDX);
    }
    

    if (sb.inode_size != FS_INODE_SIZE) 
    {
        printf("Error: Inode size mismatch: %d (expected %d)\n", 
               sb.inode_size, FS_INODE_SIZE);
    }
    
    if (sb.inode_count > FS_MAX_INODES) 
    {
        printf("Error: Too many inodes: %d (max allowed: %d)\n", 
               sb.inode_count, FS_MAX_INODES);
    }
    
    printf("Superblock check complete.\n\n");
}


void load_fs_metadata(FILE *image) 
{
   
    read_fs_block(image, sb.inode_bitmap_loc, inode_bmap);
    read_fs_block(image, sb.data_bitmap_loc, data_bmap);
    

    for (int i = 0; i < sb.inode_count; i++) {
        long offset = (sb.inode_start * FS_BLOCK_SIZE) + (i * FS_INODE_SIZE);
        fseek(image, offset, SEEK_SET);
        fread(&inodes[i], sizeof(Inode), 1, image);
    }
}


void save_fs_metadata(FILE *image) 
{
    if (repair_mode) {
        write_fs_block(image, sb.inode_bitmap_loc, inode_bmap);
        write_fs_block(image, sb.data_bitmap_loc, data_bmap);
        printf("Saved fixed bitmaps to disk.\n");
    }
}


int is_inode_valid(int inode_num) 
{
    return (inodes[inode_num].links_count > 0 && 
            inodes[inode_num].dtime == 0);
}


int get_bitmap_bit(uint8_t *bitmap, int index) //usesd to find weather a particular block is used or not
{
    int byte_idx = index / 8;
    int bit_pos = index % 8;
    return (bitmap[byte_idx] >> bit_pos) & 1;
}


void set_bitmap_bit(uint8_t *bitmap, int index) 
{
    int byte_idx = index / 8;
    int bit_pos = index % 8;
    bitmap[byte_idx] |= (1 << bit_pos);
}


void clear_bitmap_bit(uint8_t *bitmap, int index) 
{
    int byte_idx = index / 8;
    int bit_pos = index % 8;
    bitmap[byte_idx] &= ~(1 << bit_pos);
}


void verify_inode_bitmap(FILE *image) 
{
    printf("Checking inode bitmap consistency...\n");
    int fixed_count = 0;
    
    for (int i = 0; i < sb.inode_count; i++) 
    {
        int is_marked = get_bitmap_bit(inode_bmap, i);
        int is_valid = is_inode_valid(i);
        
        if (is_marked && !is_valid) 
        {
            printf("Error: Inode %d is marked as used but is invalid\n", i);
            if (repair_mode) 
            {
                clear_bitmap_bit(inode_bmap, i);
                fixed_count++;
            }
        } 
        else if (!is_marked && is_valid) 
        {
            printf("Error: Inode %d is valid but not marked as used\n", i);
            if (repair_mode)
            {
                set_bitmap_bit(inode_bmap, i);
                fixed_count++;
            }
        }
    }
    
    if (repair_mode && fixed_count > 0) 
    {
        printf("Fixed %d issues in the inode bitmap.\n", fixed_count);
    }
    
    printf("Inode bitmap check complete.\n\n");
}


void verify_data_bitmap(FILE *image) 
{
    printf("Checking data bitmap consistency...\n");
    int fixed_count = 0;
    
    
    int data_block_count = DATA_END_IDX - DATA_START_IDX + 1;
    uint8_t *used_blocks = calloc(data_block_count, sizeof(uint8_t));
    
    if (!used_blocks) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return;
    }
    
 
    for (int i = 0; i < sb.inode_count; i++) 
    {
        if (is_inode_valid(i)) 
        {
            int block_num = inodes[i].direct_pointer;
            if (block_num >= DATA_START_IDX && block_num <= DATA_END_IDX) 
            {
                used_blocks[block_num - DATA_START_IDX] = 1;
            }
        }
    }
    
  
    for (int i = 0; i < data_block_count; i++)
    {
        int block_num = i + DATA_START_IDX;
        int is_marked = get_bitmap_bit(data_bmap, i);
        int is_used = used_blocks[i];
        
        if (is_marked && !is_used) 
        {
            printf("Error: Data block %d is marked as used but not referenced\n", 
                   block_num);
            if (repair_mode) 
            {
                clear_bitmap_bit(data_bmap, i);
                fixed_count++;
            }
        } else if (!is_marked && is_used) {
            printf("Error: Data block %d is referenced but not marked as used\n", block_num);
            if (repair_mode) 
            {
                set_bitmap_bit(data_bmap, i);
                fixed_count++;
            }
        }
    }
    
    free(used_blocks);
    
    if (repair_mode && fixed_count > 0) 
    {
        printf("Fixed %d issues in the data bitmap.\n", fixed_count);
    }
    
    printf("Data bitmap check complete.\n\n");
}


void check_block_references(FILE *image) 
{
    printf("Checking for duplicate and invalid block references...\n");
    
    int data_block_count = DATA_END_IDX - DATA_START_IDX + 1;
    int *block_owners = calloc(data_block_count, sizeof(int));
    
    if (!block_owners) 
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return;
    }
    

    for (int i = 0; i < data_block_count; i++) 
    {
        block_owners[i] = -1;
    }
    

    for (int i = 0; i < sb.inode_count; i++) 
    {
        if (is_inode_valid(i)) {
            int block_num = inodes[i].direct_pointer;
            
            if (block_num < DATA_START_IDX || block_num > DATA_END_IDX) 
            {
                printf("Error: Inode %d points to invalid block %d "
                       "(valid range: %d-%d)\n", 
                       i, block_num, DATA_START_IDX, DATA_END_IDX);
                continue;
            }
            
            int block_idx = block_num - DATA_START_IDX;
            if (block_owners[block_idx] != -1) 
            {
                printf("Error: Block %d is claimed by both inode %d and inode %d\n",block_num, block_owners[block_idx], i);
            } 
            else 
            {
                block_owners[block_idx] = i;
            }
        }
    }
    
    free(block_owners);
    printf("Block reference check complete.\n\n");
}


int main(int argc, char *argv[]) {

    if (argc < 2) 
    {
        printf("Usage: %s <filesystem_image> [--repair]\n", argv[0]);
        return EXIT_FAILURE;
    }
    

    for (int i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "--repair") == 0) 
        {
            repair_mode = 1;
            printf("Repair mode enabled.\n");
            break;
        }
    }
    

    FILE *image = fopen(argv[1], "r+b");
    if (!image) 
    {
        perror("Failed to open filesystem image");
        return EXIT_FAILURE;
    }
    

    verify_superblock(image);
    load_fs_metadata(image);
    verify_inode_bitmap(image);
    verify_data_bitmap(image);
    check_block_references(image);
    

    if (repair_mode) 
    {
        save_fs_metadata(image);
    }
    

    fclose(image);
    printf("Filesystem check complete.\n");
    return EXIT_SUCCESS;
}