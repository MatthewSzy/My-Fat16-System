#ifndef __FAT16_H__
#define __FAT16_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512

struct BOOT_PARAMETERS
{
	uint16_t 	bytes_per_sector;				
	uint8_t 	sectors_per_cluster;			
	uint16_t 	reserved_sectors;				
	uint8_t 	fat_count;						
	uint16_t 	entries_in_root_directory;		
	uint16_t 	logical_sectors16;				
	uint8_t 	media_type;						
	uint16_t 	sectors_per_fat;				
	uint16_t 	sectors_per_path;				
	uint16_t 	sectors_per_cylinder;			
	uint32_t 	hidden_sectors;					
	uint32_t 	logical_sectors32;				
} __attribute__((packed));

struct BOOT_SECTOR
{
	uint8_t 					jump_addr[3];					
	uint8_t 					OEM_name[8];					
	struct BOOT_PARAMETERS 		parameters;						
	uint8_t 					loader_code[448];				
	uint16_t 					boot_sector_end;				
} __attribute__((packed));

struct DIR_ENTRY_DATA
{
	char 		name[8];						
	char 		extension[3];					
	uint8_t 	attribute;						
	uint8_t 	reserved;						
	uint8_t 	bit_time;						
	uint16_t 	creation_time;					
	uint16_t 	creation_date;					
	uint16_t 	access_date;					
	uint16_t 	reserved_word;					
	uint16_t 	modification_time;				
	uint16_t 	modification_date;				
	uint16_t 	file_start;						
	uint32_t 	file_size;						
} __attribute__((packed));

struct SUPPORT_STRUCT
{            
	FILE 					*disc_handle;
    char 					current_folder[4096];       
    struct BOOT_SECTOR 		boot_sector;    		
    uint32_t 				*fat_table;            
    uint32_t 				fat_size;              	
    uint32_t 				adress_of_main_dir;     
    uint32_t 				adrees_of_cluster_area; 
    uint32_t 				fat_entries_count;     	
    uint32_t 				fat_cluster_size;      	
};

struct TIME
{
	int min;
	int hour;
	int day;
	int month;
	int year;
};

struct DIR_HANDLE
{
    uint16_t start;
	uint32_t pos;
};

struct DIR_ENTRY
{
	char filename[257];
	uint32_t type;
};

struct FILE_HANDLE
{
    char *path;
	uint32_t pos;
};

struct FILE_DATA
{
	uint32_t 		size;
	struct TIME 	create_time;
	struct TIME 	modify_time;
	struct TIME 	access_time;
	bool 			read_only;
	bool 			hidden;
	bool 			system;
	bool 			volume;
	bool 			directory;
	bool 			archive;
	uint32_t 		clusters_count;
	uint32_t 		first_cluster;
};

size_t readblock(void* buffer, uint32_t first_block, size_t block_count);

int load_disc(const char *file);
int load_fat();
int load_entry_name(uint32_t position, const char *name, struct DIR_ENTRY_DATA *result);
int load_entry_path(const char *path, struct DIR_ENTRY_DATA *result);
int load_entry_pos(uint32_t position_start, uint32_t position, struct DIR_ENTRY_DATA *result);
int load_filedata(const char *path, struct FILE_DATA *file_data);
int fat_check(uint8_t fat[], uint8_t fat_copy[]);

struct DIR_HANDLE *opendir(const char *path);
void closedir(struct DIR_HANDLE *dir);
int readdir(struct DIR_HANDLE *dir, struct DIR_ENTRY *entry);
struct FILE_HANDLE *openfile(const char *path, const char* mode);
void closefile(struct FILE_HANDLE *file);
int readfile(void *buffer, uint32_t size, struct FILE_HANDLE *file);

void dir_function();
void cd_function(char *folder);
void pwd_function();
void fileinfo_function(char *filename);
void cat_function(char *filename);
void spaceinfo_function();
void rootinfo_function();
void get_function(char *filename);
void zip_function(char *filename1, char *filename2, char *filename3);

char *combining_the_name(const char *filename, const char *fileextension, char *result);
#endif
