// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    num_openfile = 0;
    for (int i = 0; i < MAXFILENUM; i++) fileDescriptorTable[i] = NULL;
    DEBUG(dbgFile, "Initializing the file system.");
    if (format) {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
		FileHeader *mapHdr = new FileHeader;
		FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);	    
		freeMap->Mark(DirectorySector);

		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
		ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
		mapHdr->WriteBack(FreeMapSector);    
		dirHdr->WriteBack(DirectorySector);

		// OK to open the bitmap and directory files now
		// The file system operations assume these two files are left open
		// while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
     
		// Once we have the files "open", we can write the initial version
		// of each file back to disk.  The directory at this point is completely
		// empty; but the bitmap has been changed to reflect the fact that
		// sectors on the disk have been allocated for the file headers and
		// to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
		freeMap->WriteBack(freeMapFile);	 // flush changes to disk
		directory->WriteBack(directoryFile);

		if (debug->IsEnabled('f')) {
			freeMap->Print();
			directory->Print();
        }
        delete freeMap; 
		delete directory; 
		delete mapHdr; 
		delete dirHdr;
    } else {
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
	delete freeMapFile;
	delete directoryFile;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *path, int initialSize, bool Dir) 
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    if (Dir == TRUE) initialSize = DirectoryFileSize;//demo
    DEBUG(dbgFile, "Creating file " << path << " size " << initialSize);

    directory = new Directory(NumDirEntries); 
    char targetPath[500]; //demo 3
    strcpy(targetPath, path);
    OpenFile *current_dirfile = findsubdirectory(targetPath);
    if (current_dirfile == NULL)
    {
        delete directory;
        return FALSE;
    }
    directory->FetchFrom(current_dirfile);

    if (directory->Find(targetPath) != -1) success = FALSE; // file is already in directory
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1) success = FALSE; // no free block for file header
        else if (!directory->Add(targetPath, sector, Dir)) success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            int totalheadersize = hdr->Allocate(freeMap, initialSize); //demo 3(int)
            if (totalheadersize == 0) success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                directory->WriteBack(current_dirfile);
                freeMap->WriteBack(freeMapFile);
                printf ("Total header's size:  %d bytes\n", totalheadersize);
            }
            delete hdr;
        }
        delete freeMap;
    }

    if (current_dirfile != directoryFile) delete current_dirfile;
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

std::pair<OpenFile *, OpenFileId> FileSystem::Open(char *path) //demo 3
{    
    if (num_openfile == MAXFILENUM) return make_pair((OpenFile *)NULL, -1);
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    char targetPath[500];
    strcpy(targetPath, path);
    OpenFile *current_dirfile = findsubdirectory(targetPath);
    if (current_dirfile == NULL)
    {
        delete directory;
        return make_pair((OpenFile*)NULL, -1);
    }
    DEBUG(dbgFile, "Opening file" << targetPath);
    directory->FetchFrom(current_dirfile);
    sector = directory->Find(targetPath);

    
    if (sector >= 0) openFile = new OpenFile(sector); // name was found in directory
    if (openFile == NULL)
    {
        delete directory;
        if (current_dirfile != directoryFile) delete current_dirfile;
        return make_pair((OpenFile *)NULL, -1);
    }

    for (int i = 1; i <= MAXFILENUM; i++)
    {
        if (fileDescriptorTable[i] == NULL)
        {
            num_openfile++;
            fileDescriptorTable[i] = openFile;
            delete directory;
            if (current_dirfile != directoryFile) delete current_dirfile;
            return make_pair((OpenFile *)openFile, i);
        }
    }

    delete directory;
    if (current_dirfile != directoryFile) delete current_dirfile;
    return make_pair((OpenFile *)NULL, -1); // return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(bool recursive, char *path) 
{ 
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory(NumDirEntries);
    char targetPath[500];
    strcpy(targetPath, path);//demo 3
    OpenFile *current_dirfile = findsubdirectory(targetPath);
    if (current_dirfile == NULL)
    {
        delete directory;
        return FALSE;
    }
    directory->FetchFrom(current_dirfile);
    sector = directory->Find(targetPath);
    if (sector == -1)
    {
        delete directory;
        if (current_dirfile != directoryFile) delete current_dirfile;
        return FALSE; // file not found
    }

    if (directory->IsDir(targetPath) == TRUE && recursive == TRUE) // demo bonus
    {
        Directory *subdirectory = new Directory(NumDirEntries);
        OpenFile *subdirfile = new OpenFile(sector);
        subdirectory->FetchFrom(subdirfile);
        char targetPath[500];
        strcpy(targetPath, path);
        int offset = strlen(targetPath);
        targetPath[offset] = '/';
        for (int i = 0; i < subdirectory->gettablesize(); i++)
        {
            DirectoryEntry* tablei = subdirectory->gettable();
            if (tablei[i].inUse == TRUE)
            {
                strcpy(targetPath + offset + 1, tablei[i].name);
                Remove(recursive, targetPath);
            }
        }
        delete subdirectory;
        delete subdirfile;
    }

    printf("remove: %s\n",targetPath);

    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    directory->Remove(targetPath);

    freeMap->WriteBack(freeMapFile);  // flush to disk
    directory->WriteBack(current_dirfile); // flush to disk
    if (current_dirfile != directoryFile) delete current_dirfile;
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List(bool recursive, char *dirPath) //demo 3
{
    if (strcmp(dirPath, "/") == 0)//root
    { 
        Directory *directory = new Directory(NumDirEntries);
        directory->FetchFrom(directoryFile);
        if (recursive == TRUE) directory->ListRecursive();
        else directory->List();
        delete directory;
        return;
    }
    else
    {
        char targetPath[500];
        strcpy(targetPath, dirPath);

        OpenFile *subdirfile = findsubdirectory(targetPath);
        if (subdirfile == NULL) return;
        Directory *subdirectory = new Directory(NumDirEntries);
        subdirectory->FetchFrom(subdirfile);

        int targetsector = subdirectory->Find(targetPath);
        Directory *targetdirectory = new Directory(NumDirEntries);
        OpenFile *targetdirfile = new OpenFile(targetsector);
        targetdirectory->FetchFrom(targetdirfile);

        if (recursive == TRUE) targetdirectory->ListRecursive();
        else targetdirectory->List();

        delete targetdirectory;
        delete targetdirfile;
        delete subdirectory;
        if (subdirfile != directoryFile) delete subdirfile;
    }
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

OpenFile *FileSystem::findsubdirectory(char *path) //demo 3
{
    char *split = "/";
    char *token = strtok(path, split);

    OpenFile *current_dirfile = directoryFile;
    Directory *current_directory = new Directory(NumDirEntries);
    current_directory->FetchFrom(directoryFile);
    if (token != NULL)
    {
        char *nextToken = "";
        nextToken = strtok(NULL, split);
        while (nextToken != NULL && current_directory->IsDir(token) == TRUE)
        {
            int sector = current_directory->Find(token);
            if (current_dirfile != directoryFile) delete current_dirfile;
            if (sector != -1)
            {
                current_dirfile = new OpenFile(sector);
                current_directory->FetchFrom(current_dirfile);
            }
            else
            {
                delete current_directory;
                return NULL;
            }
            token = nextToken;
            nextToken = strtok(NULL, split);
        }
        strcpy(path, token);
        delete current_directory;
        return current_dirfile;
    }
    else
    {
        delete current_directory;
        return NULL;
    }
}
#endif // FILESYS_STUB
