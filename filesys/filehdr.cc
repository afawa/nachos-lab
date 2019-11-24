// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

    if(numSectors<NumDirect){
        for (int i = 0; i < numSectors; i++)
	        dataSectors[i] = freeMap->Find();
        return TRUE;
    }else{
        for(int i=0;i<NumDirect-1;++i){
            dataSectors[i] = freeMap->Find();
        }
        dataSectors[NumDirect-1] = freeMap->Find();
        int left = numSectors - (NumDirect-1);
        int indirect_index[32]; 
        int pos=0;
        int index_sector = dataSectors[NumDirect-1];
        while(left>0){
            if(pos==31&&left>0){
                int temp=freeMap->Find();
                indirect_index[31]=temp;
                synchDisk->WriteSector(index_sector,(char*)indirect_index);
                index_sector=temp;
                pos=0;
                continue;
            }
            indirect_index[pos]=freeMap->Find();
            pos++;
            left--;
        }
        synchDisk->WriteSector(index_sector,(char*)indirect_index);
        //for(int i=0;i<numSectors-(NumDirect-1);++i){
        //    indirect_index[i]=freeMap->Find();
        //    if(indirect_index[i]==-1){
        //        printf("no memory\n");
        //        ASSERT(indirect_index!=-1);
        //    }
        //}
        //synchDisk->WriteSector(dataSectors[NumDirect-1],(char *)indirect_index);
        return TRUE;
    }
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if(numSectors<NumDirect){
        for (int i = 0; i < numSectors; i++) {
	        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
    }else{
        int left = numSectors-(NumDirect-1)-1;
        int rank = left/31;
        int sector[rank+1];
        char *indirect_index = new char[SectorSize];
        sector[0]=dataSectors[NumDirect-1];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirect_index);
        for(int i=1;i<=rank;++i){
            sector[i]=int(indirect_index[31*4]);
            synchDisk->ReadSector(sector[i],indirect_index);
        }
        int temp=left%31;
        for(int i=0;i<=temp;++i){
            freeMap->Clear((int) indirect_index[i*4]);
        }
        for(int i=rank-1;i>=0;--i){
            synchDisk->ReadSector(sector[i],indirect_index);
            for(int j=0;j<32;++j){
                freeMap->Clear((int) indirect_index[j*4]);
            }
        }
        for(int i=0;i<NumDirect;++i){
            freeMap->Clear((int) dataSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    if(offset<((NumDirect-1)*SectorSize)){
        return(dataSectors[offset / SectorSize]);
    }else{
        int sector_position = (offset-(NumDirect-1)*SectorSize)/SectorSize;
        int rank = (sector_position)/31;
        sector_position = sector_position%31;
        int sector=dataSectors[NumDirect-1];
        char *indirect_index = new char[SectorSize];
        while(rank>0){
            synchDisk->ReadSector(sector,indirect_index);
            sector = int(indirect_index[31*4]);
            rank--;
        }
        synchDisk->ReadSector(sector,indirect_index);
        return int(indirect_index[sector_position*4]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    printf("create_time: %s\n",create_time);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    printf("%d %d\n",numSectors,NumDirect);
    if(numSectors<NumDirect){
        for (i = k = 0; i < numSectors; i++) {
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n"); 
        }
    }else{
        for(i=k=0;i<NumDirect-1;++i){
            printf("sector: %d\n",dataSectors[i]);
            synchDisk->ReadSector(dataSectors[i],data);
            for(j=0;(j<SectorSize)&&(k<numBytes);++j,++k){
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
        char *indirect_index = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirect_index);
        for(int i=0;i<numSectors-(NumDirect-1);++i){
            printf("Sector: %d\n",int(indirect_index[i*4]));
            synchDisk->ReadSector(int(indirect_index[i*4]),data);
            for(j=0;(j<SectorSize)&&(k<numBytes);++j,++k){
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
    }
    delete [] data;
}

void FileHeader::set_create_time(){
    time_t timep;
    time (&timep);
    strncpy(create_time, asctime(gmtime(&timep)),25);
    create_time[24]=0;
    printf("create file %s\n",create_time);

}

void FileHeader::Extend(BitMap* freeMap,int bytes){
    numBytes = numBytes+bytes;
    int initial_sector = numSectors;
    numSectors = divRoundUp(numBytes,SectorSize);
    if(initial_sector == numSectors)
        return true;
    if(freeMap->NumClear()<numSectors - initial_sector)
        return false;
    printf("extend %d sectors\n",numSectors - initial_sector);

    if(initial_sector<NumDirect){
        if(numSectors<NumDirect){
            for(int i=initial_sector;i<numSectors;++i)
                dataSectors[i]=freeMap->Find();
        }else{
            for(int i=initial_sector;i<NumDirect-1;++i){
                dataSectors[i] = freeMap->Find();
            }
            dataSectors[NumDirect-1] = freeMap->Find();
            int left = numSectors - (NumDirect-1);
            int indirect_index[32]; 
            int pos=0;
            int index_sector = dataSectors[NumDirect-1];
            while(left>0){
                if(pos==31&&left>0){
                    int temp=freeMap->Find();
                    indirect_index[31]=temp;
                    synchDisk->WriteSector(index_sector,(char*)indirect_index);
                    index_sector=temp;
                    pos=0;
                    continue;
                }
                indirect_index[pos]=freeMap->Find();
                pos++;
                left--;
            }
            synchDisk->WriteSector(index_sector,(char*)indirect_index);

        }
    }else{
        int init_sec = initial_sector-NumDirect+1;//从这里出发
        int rank = init_sec /31;
        int pos = init_sec % 31;
        int sector = dataSectors[NumDirect-1];
        int indirect_index[32]; 
        for(int i=0;i<=rank;++i){
            synchDisk->ReadSector(sector,(char*)indirect_index);
            if(i!=rank){
                sector = indirect_index[31];
            }
        }
        int left = numSectors - initial_sector;
        while(left>0){
            indirect_index[pos]=freeMap->Find();
            pos++;
            left--;
            if(pos == 31){
                int temp=freeMap->Find();
                indirect_index[31]=temp;
                synchDisk->WriteSector(sector,(char*)indirect_index);
                sector=temp;
                pos=0;
            }
        }
        synchDisk->WriteSector(sector,(char*)indirect_index);
    }
    return true;
}
