////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the FS3 storage system.
//
//   Author        : *** INSERT YOUR NAME ***
//   Last Modified : *** DATE ***
//

// Includes
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <math.h>
// Project Includes
#include <fs3_driver.h>
#include <fs3_controller.h>
#include <fs3_cache.h>
//
// Defines

//
// Static Global Variables

//global dynamic variables
int32_t mounted = -1; //for state of disk (mounted/not mounted)
file* fileArr; //points to an array of files created 
//int16_t** diskArr; //to keep track of what sectors and tracks are being used 

//tracks how many files are in file array 
int fileAmount = 0; 
//for system calls
FS3CmdBlk check;


//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_mount_disk
// Description  : FS3 interface, mount/initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_mount_disk(void) {
	//checking if mounted
	//checking if mounted
	if (mounted == 0){return -1;}
		//---- allocating memeory ----//
	//dynamically allocated mounted variable to keep track of state of disk
	//an array to store all files with length of total max files
	fileArr = (file *) calloc(FS3_MAX_TOTAL_FILES,sizeof(file));
	//a two dimensional array that will track where all files are located 
	int i; 
	// holds filehandles 
	 //diskArr = (int16_t **) calloc(FS3_MAX_TRACKS,sizeof(int16_t*)); 
	// for(i=0;i<FS3_MAX_TRACKS;i++){
	 //	diskArr[i] = (int16_t *) calloc(FS3_TRACK_SIZE,sizeof(int16_t));
	 //}
	// checking for null pointers --> fail if true
	int allocated = 0; //used to track if allocation was successful for later return
	//if (diskArr == NULL){logMessage(FS3DriverLLevel, "could not allocate diskArr");allocated = -1;}
	if (fileArr == NULL){logMessage(FS3DriverLLevel, "could not allocate fileArr");allocated = -1;}
	
	//----- sys_calls for mount & seek -----//
	//mounting process
	uint8_t op = FS3_OP_MOUNT; uint8_t sec = 0x00; uint32_t trk = 0x00; uint8_t ret = 0x00;
	// check is cmdblk returned by syscall
	check = fs3_syscall(construct_fs3_cmdblock(op,sec,trk,ret),NULL);
	if (deconstruct_fs3cmdblock(check,&op,&sec,&trk,&ret) == 0){mounted = 0;}

	logMessage(FS3DriverLLevel, "For mount op: %x sec:%x trk%x ret%x", op, sec, trk, ret);

	// default seek to track zero / disk pointer now points to track 0
	uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = 0; uint8_t ret_t =0x00;
	check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
	if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}

	logMessage(FS3DriverLLevel, " For Seek op:%x sec:%x trk%x ret%x", op_t , sec_t , trk_t , ret_t );
	logMessage(FS3DriverLLevel, "FS3 DRVR MOUNTED");

	//--- checking allocation status and returning ---//
	//this if statement eleminates multiple points of return in function 
	if (allocated == -1){mounted = -1;}
	return mounted;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_unmount_disk
// Description  : FS3 interface, unmount the disk, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_unmount_disk(void) {
	
	int32_t out = 0; // eliminates mult breakpoints within function
	if (mounted == (int)1){return -1;}
	uint8_t op = FS3_OP_UMOUNT; uint8_t sec = 0x00; uint32_t trk = 0x00; uint8_t ret = 0x00;
	check = fs3_syscall(construct_fs3_cmdblock(op,sec,trk,ret),NULL);
	if (deconstruct_fs3cmdblock(check,&op,&sec,&trk,&ret) != 0){out = -1;}

	logMessage(FS3DriverLLevel, "For unmount: op:%x sec:%x trk%x ret%x", op, sec, trk, ret);

	//--- preventing memory leaks ---//
	check = 0;

	free(fileArr);
	fileArr = NULL;
	//free(diskArr);
	//diskArr = NULL;
	return out;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t fs3_open(char *path) {
	// initializing filehandle to return failure
	int16_t fH = -1;
	int i;
	int pos = -1; 
	// checks if file exists and gets position in fileArr for later use
	for (i=0;i<fileAmount;i++){
		if (fileArr[i].fileName == path){
			pos = i;
		}
	}
	//file already exists
	if (pos > 0){
		logMessage(FS3DriverLLevel,"File already Exists");
		//if file isn't open
		if (fileArr[pos].openStatus == 0){
			fileArr[pos].openStatus = 1;
			fileArr[pos].locPtr = 0;
			fH = fileArr[pos].fileHandle;
			logMessage(FS3DriverLLevel, "File %d opened",(int)fH);
		}
		//if file is already open return file handle
		else{
			fH = fileArr[pos].fileHandle;
			logMessage(FS3DriverLLevel, "File %d was already opened",(int)fH);
		}
		
	}
	//file does not exists must be created
	else{
		logMessage(FS3DriverLLevel,"File does not exist");
		//number of existing files increases by 1
		fileAmount += 1;
		fH = (int16_t)fileAmount;
		pos = fileAmount - 1;
		//copying file name to data structure
		strcpy(fileArr[pos].fileName, path);
		//initializing new file
		fileArr[pos].fileHandle = fH;
		fileArr[pos].openStatus = 1;
		fileArr[pos].locPtr = 0;
		fileArr[pos].size = 0;
		logMessage(FS3DriverLLevel,"%s was created and given handle: %d",fileArr[pos].fileName, fileArr[pos].fileHandle);
		
	}
	return fH;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t fs3_close(int16_t fd) {
	// NOTE: file is open if openStatus == 1
	//check if file is open
	int i;
	int16_t out = -1; 
	int pos = -1;
	//checking that file exists and getting it's position in fileArr
	for (i=0;i<fileAmount;i++){
		if (fileArr[i].fileHandle == fd){
			pos = i;
		}
	}
	//if file exists
	if (pos>0){
		//if file is open
		if (fileArr[pos].openStatus== 1){
			fileArr[0].openStatus = 0;
			out = 0;
		}
		//file is closed
		else{logMessage(FS3DriverLLevel,"File was already closed");}
	}
	//file does not exist
	else{logMessage(FS3DriverLLevel,"FileHandle given does not exist");}
	return out;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t fs3_read(int16_t fd, void *buf, int32_t count) {
	if (count < 0){return -1;}
	int i;
	int fil = -1;
	for (i=0;i<fileAmount;i++){if (fileArr[i].fileHandle == fd){fil = i;}}
	// return failure if file dne or is not open
	if (fil<0 || fileArr[fil].openStatus==0){logMessage(FS3DriverLLevel,"File does not exist or is not opened");return -1;}
	file thisFile = fileArr[fil];
	logMessage(FS3DriverLLevel,"File name : %s",thisFile.fileName);



	//char *readbuf;
	
	char *buffer;
	uint32_t posInFile = thisFile.locPtr;
	int currentSectorIndex = (int) floor(posInFile/FS3_SECTOR_SIZE);
	int bufferIndex = posInFile - (currentSectorIndex * FS3_SECTOR_SIZE);
	int pos;
	FS3TrackIndex track;
	FS3SectorIndex sector;


	//if reading more than 2 sectors (entire file)
	if (count > FS3_SECTOR_SIZE){
		int size = thisFile.size;
		int sectsToRead = (int)floor(size/FS3_SECTOR_SIZE);
		// buffer must be size sum of sectors it takes up
		//buffer = (char *)calloc(FS3_SECTOR_SIZE*(sectsToRead+1),sizeof(char));
		char buffer[FS3_SECTOR_SIZE*(sectsToRead+1)];
		track = thisFile.fileHandle;
		pos = 0;
		uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = track ; uint8_t ret_t =0x00;
		check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
		if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}
		for (i=0;i<=sectsToRead;i++){
			sector = i;
			char readbuf[FS3_SECTOR_SIZE];
			uint8_t op_r = FS3_OP_RDSECT; uint8_t sec_r = sector; uint32_t trk_r = 0x00 ; uint8_t ret_r =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_r,sec_r,trk_r,ret_r), readbuf);
			if (deconstruct_fs3cmdblock(check,&op_r,&sec_r,&trk_r,&ret_r) != 0){logMessage(FS3DriverLLevel,"returning -1 op:%x sec:%x trk:%x ret:%x",
			op_r,sec_r,trk_r,ret_r);return -1;}
			memcpy(&buffer[pos],readbuf,FS3_SECTOR_SIZE);
			pos += FS3_SECTOR_SIZE;
		}
		memcpy(buf,&buffer[bufferIndex],count);
		return count;
	}


	char *readbuf;
	char *readCache;
	//will require a read across two sectors
	if ((int)floor((posInFile + count)/FS3_SECTOR_SIZE) > currentSectorIndex){
		//char buffer[FS3_SECTOR_SIZE*2];
		buffer = (char *)calloc(2*FS3_SECTOR_SIZE,sizeof(char));
		sector = currentSectorIndex;
		track = thisFile.fileHandle;
		pos = 0;
		// will read the sector the pointer is currently in and the following sector	
		//seek each time with updated track (does not need to be in loop since file handle represents track)
		uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = track ; uint8_t ret_t =0x00;
		check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
		if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}
		for (i=0; i < 2; i++){
			readCache = fs3_get_cache(track,sector);
			if (readCache != NULL){
				memcpy(&buffer[pos],readCache,FS3_SECTOR_SIZE);
				pos+=1;
				sector += 1;
			}
			else{
				//read at that sector
				readbuf = (char *)calloc(FS3_SECTOR_SIZE,sizeof(char));
				uint8_t op_r = FS3_OP_RDSECT; uint8_t sec_r = sector; uint32_t trk_r = 0x00 ; uint8_t ret_r =0x00;
				check = fs3_syscall(construct_fs3_cmdblock(op_r,sec_r,trk_r,ret_r), readbuf);
				if (deconstruct_fs3cmdblock(check,&op_r,&sec_r,&trk_r,&ret_r) != 0){logMessage(FS3DriverLLevel,"returning -1 op:%x sec:%x trk:%x ret:%x",
				op_r,sec_r,trk_r,ret_r);return -1;}
				//copy into buffer
				memcpy(&buffer[pos],readbuf,FS3_SECTOR_SIZE);
				//put into cache
				fs3_put_cache(track,sector,readbuf);
				pos += FS3_SECTOR_SIZE;
				sector += 1;
				free(readbuf);
				readbuf = NULL;
			}
		}
		memcpy(buf,&buffer[bufferIndex],count);
		free(buffer);
		buffer = NULL;
	}
	//only requires to read one sector
	else{		
		sector = currentSectorIndex;
		track = thisFile.fileHandle;
		readCache = fs3_get_cache(track,sector);
		if (readCache != NULL){
			//if sector in cache
			memcpy(buf,&readCache[bufferIndex],count);
		}
		else{
			buffer = (char *)calloc(FS3_SECTOR_SIZE,sizeof(char));
			//seek
			uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = track ; uint8_t ret_t =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
			if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}

			//read
			uint8_t op_r = FS3_OP_RDSECT; uint8_t sec_r = sector; uint32_t trk_r = 0x00 ; uint8_t ret_r =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_r,sec_r,trk_r,ret_r), buffer);
			if (deconstruct_fs3cmdblock(check,&op_r,&sec_r,&trk_r,&ret_r) != 0){logMessage(FS3DriverLLevel,"returning -1 op:%x sec:%x trk:%x ret:%x",
			op_r,sec_r,trk_r,ret_r);return -1;}

			memcpy(buf,&buffer[bufferIndex],count);
			fs3_put_cache(track,sector,buffer);
			free(buffer);
			buffer = NULL;
		}
	}
	return count;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t fs3_write(int16_t fd, void *buf, int32_t count) {
	// fs3_write (local) scope variable tempbuf

	if (count < 0){return -1;}
	int i;
	int fil = -1;
	for (i=0;i<fileAmount;i++){if (fileArr[i].fileHandle == fd){fil = i;}}
	// return failure if file dne or is not open
	if (fil<0 || fileArr[fil].openStatus==0){logMessage(FS3DriverLLevel,"File does not exist or is not opened");return -1;}
	file thisFile = fileArr[fil];
	logMessage(FS3DriverLLevel,"File name : %s",thisFile.fileName);

	uint32_t size = thisFile.size;
	uint32_t posInFile = thisFile.locPtr;
	int currentSectorIndex = (int) floor(posInFile/FS3_SECTOR_SIZE);
	int bufferIndex = posInFile - (currentSectorIndex * FS3_SECTOR_SIZE);
	int pos;
	FS3TrackIndex track;
	FS3SectorIndex sector;

	


	// write counts do not exceed size of sector 
	// for writes that will span two sectors due to position
	if ((int)floor((posInFile+count)/FS3_SECTOR_SIZE)>currentSectorIndex){
		char buffer[FS3_SECTOR_SIZE*2];
		sector = currentSectorIndex;
		track = thisFile.fileHandle;
		char *cacheRead;
		pos = 0;
		// will read the sector the pointer is currently in and the following sector	
		//seek each time with updated track (does not need to be in loop since file handle represents track)
		uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = track ; uint8_t ret_t =0x00;
		check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
		if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}
		for (i=0; i < 2; i++){
			//read at that sector
			cacheRead = fs3_get_cache(track,sector);
			if (cacheRead != NULL){
				logMessage(FS3DriverLLevel,"Cache hit");
				memcpy(&buffer[pos],cacheRead,FS3_SECTOR_SIZE);
				pos+= FS3_SECTOR_SIZE;
				sector += 1;
			}
			else{
				logMessage(FS3DriverLLevel,"Cache Miss");
				char readbuf[FS3_SECTOR_SIZE];
				uint8_t op_r = FS3_OP_RDSECT; uint8_t sec_r = sector; uint32_t trk_r = 0x00 ; uint8_t ret_r =0x00;
				check = fs3_syscall(construct_fs3_cmdblock(op_r,sec_r,trk_r,ret_r), readbuf);
				if (deconstruct_fs3cmdblock(check,&op_r,&sec_r,&trk_r,&ret_r) != 0){logMessage(FS3DriverLLevel,"returning -1 op:%x sec:%x trk:%x ret:%x",
				op_r,sec_r,trk_r,ret_r);return -1;}
				//copy into buffer
				memcpy(&buffer[pos],readbuf,FS3_SECTOR_SIZE);
				pos += FS3_SECTOR_SIZE;
				sector += 1;
			}
		}
		//copy buf into buffer
		memcpy(&buffer[bufferIndex],buf,count);

	
		//now to write to the disk
		sector = currentSectorIndex;
		track = thisFile.fileHandle;
		pos = 0; 
		//seek just in case
		op_t = FS3_OP_TSEEK; sec_t = 0x00; trk_t = track ; ret_t =0x00;
		check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
		if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}
		//write to disk loop
		for (i=0; i<2; i++){
			char writeBuf[FS3_SECTOR_SIZE];
			memcpy(writeBuf,&buffer[pos],FS3_SECTOR_SIZE);
			uint8_t op_w = FS3_OP_WRSECT; uint8_t sec_w = sector; uint32_t trk_w = 0x00 ; uint8_t ret_w =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_w,sec_w,trk_w,ret_w),writeBuf);
			if (deconstruct_fs3cmdblock(check,&op_w,&sec_w,&trk_w,&ret_w) != 0){return -1;}
			//insert in cache
			fs3_put_cache(track,sector,writeBuf);
			pos += FS3_SECTOR_SIZE ;
			sector += 1;
		}
		//update data structures
		//location pointer
		fileArr[fil].locPtr = posInFile + count;
		//size updated if needed
		if (posInFile + count > size){
			fileArr[fil].size = posInFile + count;
		}
		//since it is writing in a new sector must add that sector to sector Array in struct
		fileArr[fil].sectors[fileArr[fil].sectorAmount].trackNumber = track;
		fileArr[fil].sectors[fileArr[fil].sectorAmount].sectorNumber = currentSectorIndex + 1;
		fileArr[fil].sectorAmount += 1;
	}
	//for writes that only span one sector
	else{
		char *cacheRead;

		logMessage(FS3DriverLLevel,"This write will only span 1 sector");
		char buffer[FS3_SECTOR_SIZE];
		sector = currentSectorIndex;
		track = thisFile.fileHandle;

		//seek
		uint8_t op_t = FS3_OP_TSEEK; uint8_t sec_t = 0x00; uint32_t trk_t = track ; uint8_t ret_t =0x00;
		check = fs3_syscall(construct_fs3_cmdblock(op_t,sec_t,trk_t,ret_t),NULL);
		if (deconstruct_fs3cmdblock(check,&op_t,&sec_t,&trk_t,&ret_t) != 0){return -1;}


		cacheRead = fs3_get_cache(track,sector);
		if (cacheRead != NULL){
			logMessage(LOG_INFO_LEVEL,"cache hit");
			//cache read conains the data
			memcpy(&cacheRead[bufferIndex],buf,count);

			//write to disk
			uint8_t op_w = FS3_OP_WRSECT; uint8_t sec_w = sector; uint32_t trk_w = 0x00 ; uint8_t ret_w =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_w,sec_w,trk_w,ret_w),cacheRead);
			if (deconstruct_fs3cmdblock(check,&op_w,&sec_w,&trk_w,&ret_w) != 0){return -1;}

			//update cache 
			fs3_put_cache(track,sector,cacheRead);

			//update data structure
			//location pointer
			fileArr[fil].locPtr = posInFile + count;
			//if size needs to be adjusted
			if (posInFile + count > size){
				fileArr[fil].size = posInFile + count;
			}
	
		}
		//cache miss needs to read from disk
		else{
			//read
			uint8_t op_r = FS3_OP_RDSECT; uint8_t sec_r = sector; uint32_t trk_r = 0x00 ; uint8_t ret_r =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_r,sec_r,trk_r,ret_r), buffer);
			if (deconstruct_fs3cmdblock(check,&op_r,&sec_r,&trk_r,&ret_r) != 0){logMessage(FS3DriverLLevel,"returning -1 op:%x sec:%x trk:%x ret:%x",
			op_r,sec_r,trk_r,ret_r);return -1;}
			memcpy(&buffer[bufferIndex],buf,count);

			//write
			uint8_t op_w = FS3_OP_WRSECT; uint8_t sec_w = sector; uint32_t trk_w = 0x00 ; uint8_t ret_w =0x00;
			check = fs3_syscall(construct_fs3_cmdblock(op_w,sec_w,trk_w,ret_w),buffer);
			if (deconstruct_fs3cmdblock(check,&op_w,&sec_w,&trk_w,&ret_w) != 0){return -1;}

			//putting sector into cache
			fs3_put_cache(track,sector,buffer);
			
			//update data structure
			//location pointer
			fileArr[fil].locPtr = posInFile + count;
			//if size needs to be adjusted
			if (posInFile + count > size){
				fileArr[fil].size = posInFile + count;
			}
			if (size == 0){
				//it is a new file must add sector to sectors array in struct
				fileArr[fil].sectorAmount += 1;
				fileArr[fil].sectors[0].sectorNumber = sector;
				fileArr[fil].sectors[0].trackNumber = track;
			}
		}
	}	
	logMessage(FS3DriverLLevel,"\n----------------Write Stats-----------------\n count: %d\n previous location: %d\n updated location: %d\n "
		"previous size: %d\n updated size: %d\n", count, posInFile, fileArr[fil].locPtr,size, fileArr[fil].size);


	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_seek(int16_t fd, uint32_t loc) {
	// ----overview---- //
	// check file exists, is open, and that new ptr does not exceed maximum file size
	// assign new cursor location for given file
	// check to see if up-to-date size is still accurate 
	// if new location exceeds size set size == loc
	// return succesfully
	// ---------------------------------------------//
	int i;
	int out = -1;
	int pos = -1;
	//checking that file exists and getting it's position in fileArr
	for (i=0;i<fileAmount;i++){
		if (fileArr[i].fileHandle == fd){
			pos = i;
		}
	}
	// file exists
	if (pos>=0){
		// checking that file is open and loc is less than file size
		if (fileArr[pos].openStatus == 1 && loc <= fileArr[pos].size){
			fileArr[pos].locPtr = loc;
			out = 0;
			logMessage(FS3DriverLLevel, "NEW cursor location : %u", (unsigned long)fileArr[pos].locPtr);
		}
		// loc is not a valid location or file was not open
		else{logMessage(FS3DriverLLevel, "File was not opened / location overstepped boudns of file");}
	}
	//file does not exist
	else{logMessage(FS3DriverLLevel, "Could not find file handle");}
	return out;
}


//---- contstruct/deconstruct cmdblock methods ----//

//builds 64 bit integer for sys_call
FS3CmdBlk construct_fs3_cmdblock(uint8_t op, uint8_t sec, uint_fast32_t trk, uint8_t ret){
	//initializing local variables
	FS3CmdBlk out = 0x00 , tempop, tempsec, temptrk, tempret;
	//shifting
	tempop =  ((uint64_t)op)<<60;
	tempsec = ((uint64_t)sec)<<44;
	temptrk = ((uint64_t)trk)<<12;
	tempret = ((uint64_t)ret)<<11;
	//combining through "or" operator
	out = tempop|tempsec|temptrk|tempret;
	return out;
}

//dissects 64 bit integer from sys_call
int deconstruct_fs3cmdblock(FS3CmdBlk cmdblock, uint8_t *op, uint8_t *sec, uint32_t *trk, uint8_t *ret){
	//shifting 
	*ret = cmdblock >> 11;
	*trk = cmdblock >> 12;
	*sec = cmdblock >> 44;
	*op = cmdblock >> 60;
	//return updated syscall status
	int out = ((cmdblock >> 11) & 0x01);
	return out;
}