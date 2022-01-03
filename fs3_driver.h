#ifndef FS3_DRIVER_INCLUDED
#define FS3_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the FS3 storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 19 Sep 2021 08:12:43 AM PDT
//

// Include files
#include <stdint.h>
#include <fs3_controller.h>

// Defines
#define FS3_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define FS3_MAX_PATH_LENGTH 128 // Maximum length of filename length

//
// Interface functions

int32_t fs3_mount_disk(void);
	// FS3 interface, mount/initialize filesystem

int32_t fs3_unmount_disk(void);
	// FS3 interface, unmount the disk, close all files

int16_t fs3_open(char *path);
	// This function opens a file and returns a file handle

int16_t fs3_close(int16_t fd);
	// This function closes a file

int32_t fs3_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t fs3_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t fs3_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

//My functions
FS3CmdBlk construct_fs3_cmdblock(uint8_t op, uint8_t sec, uint_fast32_t trk, uint8_t ret);
int deconstruct_fs3cmdblock(FS3CmdBlk cmdblock, uint8_t *op, uint8_t *sec, uint32_t *trk, uint8_t *ret);

//structure data types within disk table
typedef struct{
	FS3SectorIndex sectorNumber;
	FS3TrackIndex trackNumber;
} Sector;
//file structure represents important info ab each file
typedef struct{
	char fileName[128]; // file name
	int16_t fileHandle; // file handle
	int16_t openStatus; // 0 if closed 1 if open
	uint32_t locPtr; // the location of "cursor" within the file
	uint32_t size; // how many bytes file consumes
	Sector sectors[1000]; //in theory could take up more than 1000 sectors if large enough
	int sectorAmount;
} file; 

#endif
