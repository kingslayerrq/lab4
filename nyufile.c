#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct DirEntry {
  unsigned char  DIR_Name[11];      // File name
  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day
  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day
  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

void printFSInfo(const char * diskName){
    // Open file
    int fd = open(diskName, O_RDONLY);
    if (fd == -1)
        perror("err");

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("err");

    // Map file into memory
    char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED){
        perror("err");
        close(fd);
        return;
    }

    BootEntry * be = (BootEntry *)(addr);
    fprintf(stdout, "Number of FATs = %u\n", be->BPB_NumFATs);
    fprintf(stdout, "Number of bytes per sector = %u\n", be->BPB_BytsPerSec);
    fprintf(stdout, "Number of sectors per cluster = %u\n", be->BPB_SecPerClus);
    fprintf(stdout, "Number of reserved sectors = %u\n", be->BPB_RsvdSecCnt);
}

void lsRootDir(const char * diskName){
    // Open file
    int fd = open(diskName, O_RDONLY);
    if (fd == -1)
        perror("err");

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("err");

    // Map file into memory
    char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED){
        perror("err");
        close(fd);
        return;
    }

    BootEntry * be = (BootEntry *)(addr);
    unsigned short offset = be->BPB_BytsPerSec * be->BPB_FATSz32 * be->BPB_NumFATs + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec;
    
    DirEntry * de = (DirEntry *)(addr + offset);

    int count = 0;
    
    char* name = malloc(13 * sizeof(char));
    while(de){
        if (de->DIR_Name[0] == 0x00){
            break;
        }
        else if (de->DIR_Name[0] == 0xE5){
            de = (DirEntry *)(addr + offset + count * sizeof(DirEntry));
            continue;
        }
        unsigned int combinedAddr = (de->DIR_FstClusHI<<16) + de->DIR_FstClusLO;
        bool hasDot = false;
        if (de->DIR_Name[8] != 0x20){
            hasDot = true;
        }
        int ind = 0;
        for (int i = 0; i < 11; i++){
            if(de->DIR_Name[i] != 0x20){
                name[ind] = de->DIR_Name[i];
                ind++;
            }
            else{
                if(hasDot){
                    name[ind] = '.';
                    ind++;
                    i = 7;
                }
                else{
                    break;
                }
            }
        }
        name[ind] = '\0';

        if (de->DIR_FileSize == 0){          
            if (de->DIR_Attr == 0x10){
                fprintf(stdout, "%s/ (starting cluster = %u)\n", name, combinedAddr);
            }
            else{
                fprintf(stdout, "%s (size = 0)\n", name);
            }
        }
        else{
            fprintf(stdout, "%s (size = %u, starting cluster = %u)\n", name, de->DIR_FileSize, combinedAddr);
        }
        count++;

        de = (DirEntry *)(addr + offset + count * sizeof(DirEntry));
        
    }
    free(name);
    fprintf(stdout, "Total number of entries = %d\n", count);
}

int main(int argc, char **argv)
{
    int opt;
    if ((opt = getopt(argc, argv, "ilr:R:")) != -1)
    {
        // fprintf(stderr, "argc = %d\n", argc);
        if (argc == 3)
        {
            if (opt == 'i')
            {
                printFSInfo((argv + 1)[0]);
            }
            else if (opt == 'l')
            {
                lsRootDir((argv + 1)[0]);
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else if (argc == 4)
        {
            if (opt == 'r' && optarg)
            {
                fprintf(stderr, "r then nothing\n");
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else if (argc == 6)
        {
            if (opt == 'r' && optarg)
            {
                if ((opt = getopt(argc, argv, "s:") != -1) && strcmp(optarg, "sha1") == 0)
                {
                    fprintf(stderr, "r then s\n");
                }
                else
                {
                    fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                    return 0;
                }
            }
            else if (opt == 'R' && optarg)
            {
                if ((opt = getopt(argc, argv, "s:") != -1) && strcmp(optarg, "sha1") == 0)
                {
                    fprintf(stderr, "R then s\n");
                }
                else
                {
                    fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                    return 0;
                }
            }
            else
            {
                fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                return 0;
            }
        }
        else
        {
            fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
            return 0;
        }
    }
    else
    {
        fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
        return 0;
    }
}