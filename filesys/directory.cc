// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)
//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
	    return i;
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

bool Directory::isEmpty(){
    for(int i=0;i<tableSize;++i){
        if(table[i].inUse)
            return false;
    }
    return true;
}
int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

int Directory::getType(char* name){
    int i = FindIndex(name);
    if(i!=-1){
        return table[i].type;
    }
    printf("can't find\n");
    ASSERT(i!=-1);
    return -1;
}

int Directory::FindDir(char *name){
    int sector = 1;
    OpenFile *dir_file = new OpenFile(sector);
    Directory *dir = new Directory(NumDirEntries);
    dir->FetchFrom(dir_file);
    int str_pos=0;
    int sub_str_pos=0;
    char sub_str[10];
    while(str_pos<strlen(name)){
        sub_str[sub_str_pos++]=name[str_pos++];
        if(name[str_pos]=='/'){
            sub_str[sub_str_pos]=0;
            sector = dir->Find(sub_str);
            dir_file = new OpenFile(sector);
            dir = new Directory(NumDirEntries);
            dir->FetchFrom(dir_file);
            str_pos++;
            sub_str_pos=0;
        }
    }
    return sector;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector,int type)
{ 
    char file_name[FileNameMaxLen+1];
    int pos=-1;
    for(int i=strlen(name)-1;i>=0;--i){
        if(name[i]=='/'){
            pos=i+1;
            break;
        }
    }
    if(pos==-1)
        pos=0;
    int j=0;
    for(int i=pos;i<strlen(name);++i){
        file_name[j++]=name[i];
    }
    file_name[j]=0;
    if(FindIndex(file_name)!=-1)
        return false;
    for(int i=0;i<tableSize;++i){
        if(!table[i].inUse){
            table[i].inUse=true;
            strncpy(table[i].path,name,20);
            strncpy(table[i].name,file_name,FileNameMaxLen);
            table[i].sector = newSector;
            table[i].type = type;
            return true;
        }
    }
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}
