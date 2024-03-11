// https://www.includehelp.com/c/convert-ascii-string-to-hexadecimal-string-in-c.aspx#google_vignette
//https://stackoverflow.com/questions/7537791/understanding-recursion-to-generate-permutations
//https://www.geeksforgeeks.org/write-a-c-program-to-print-all-permutations-of-a-given-string/
//https://stackoverflow.com/questions/9284420/how-to-use-sha1-hashing-in-c-programming

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <openssl/sha.h>

#define SHA_DIGEST_LENGTH 20

#pragma pack(push, 1)
typedef struct BootEntry
{
    unsigned char BS_jmpBoot[3];    // Assembly instruction to jump to boot code
    unsigned char BS_OEMName[8];    // OEM Name in ASCII
    unsigned short BPB_BytsPerSec;  // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
    unsigned char BPB_SecPerClus;   // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
    unsigned short BPB_RsvdSecCnt;  // Size in sectors of the reserved area
    unsigned char BPB_NumFATs;      // Number of FATs
    unsigned short BPB_RootEntCnt;  // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
    unsigned short BPB_TotSec16;    // 16-bit value of number of sectors in file system
    unsigned char BPB_Media;        // Media type
    unsigned short BPB_FATSz16;     // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
    unsigned short BPB_SecPerTrk;   // Sectors per track of storage device
    unsigned short BPB_NumHeads;    // Number of heads in storage device
    unsigned int BPB_HiddSec;       // Number of sectors before the start of partition
    unsigned int BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
    unsigned int BPB_FATSz32;       // 32-bit size in sectors of one FAT
    unsigned short BPB_ExtFlags;    // A flag for FAT
    unsigned short BPB_FSVer;       // The major and minor version number
    unsigned int BPB_RootClus;      // Cluster where the root directory can be found
    unsigned short BPB_FSInfo;      // Sector where FSINFO structure can be found
    unsigned short BPB_BkBootSec;   // Sector where backup copy of boot sector is located
    unsigned char BPB_Reserved[12]; // Reserved
    unsigned char BS_DrvNum;        // BIOS INT13h drive number
    unsigned char BS_Reserved1;     // Not used
    unsigned char BS_BootSig;       // Extended boot signature to identify if the next three values are valid
    unsigned int BS_VolID;          // Volume serial number
    unsigned char BS_VolLab[11];    // Volume label in ASCII. User defines when creating the file system
    unsigned char BS_FilSysType[8]; // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct DirEntry
{
    unsigned char DIR_Name[11];     // File name
    unsigned char DIR_Attr;         // File attributes
    unsigned char DIR_NTRes;        // Reserved
    unsigned char DIR_CrtTimeTenth; // Created time (tenths of second)
    unsigned short DIR_CrtTime;     // Created time (hours, minutes, seconds)
    unsigned short DIR_CrtDate;     // Created day
    unsigned short DIR_LstAccDate;  // Accessed day
    unsigned short DIR_FstClusHI;   // High 2 bytes of the first cluster address
    unsigned short DIR_WrtTime;     // Written time (hours, minutes, seconds
    unsigned short DIR_WrtDate;     // Written day
    unsigned short DIR_FstClusLO;   // Low 2 bytes of the first cluster address
    unsigned int DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

void printFSInfo(const char *diskName)
{
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
    if (addr == MAP_FAILED)
    {
        perror("err");
        close(fd);
        return;
    }

    BootEntry *be = (BootEntry *)(addr);
    fprintf(stdout, "Number of FATs = %u\n", be->BPB_NumFATs);
    fprintf(stdout, "Number of bytes per sector = %u\n", be->BPB_BytsPerSec);
    fprintf(stdout, "Number of sectors per cluster = %u\n", be->BPB_SecPerClus);
    fprintf(stdout, "Number of reserved sectors = %u\n", be->BPB_RsvdSecCnt);
}

void lsRootDir(const char *diskName)
{
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
    if (addr == MAP_FAILED)
    {
        perror("err");
        close(fd);
        return;
    }

    BootEntry *be = (BootEntry *)(addr);
    // Reserved sectors and FATs
    unsigned long offset = be->BPB_BytsPerSec * be->BPB_FATSz32 * be->BPB_NumFATs + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec;
    // fprintf(stderr, "fat + reserved: %d\n", offset);
    // fprintf(stderr, "bytes per sec: %u\n", be->BPB_BytsPerSec);
    // fprintf(stderr, "reserved sec: %u\n", be->BPB_RsvdSecCnt);
    // fprintf(stderr, "fatnum: %u\n", be->BPB_NumFATs);
    // fprintf(stderr, "fat in sec: %u\n", be->BPB_FATSz32);
    // fprintf(stderr, "fat starts at %u\n", be->BPB_RsvdSecCnt * be->BPB_BytsPerSec);
    int *fat = (int *)(addr + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec);
    int maxDE = be->BPB_SecPerClus * be->BPB_BytsPerSec / sizeof(DirEntry);
    // fprintf(stderr, "maxde: %d\n", maxDE);
    int total = 0;
    // int index = 0;
    // int cnt = 0;
    // char *name = malloc(13 * sizeof(char));
    int curClust = be->BPB_RootClus;

    while (curClust < 0x0ffffff8)
    {

        unsigned long curClustOffset = (curClust - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec;
        DirEntry *de = (DirEntry *)(addr + offset + curClustOffset);
        // int cnt = 0;
        int index = 0;

        while (de->DIR_Name[0] != 0x00 && index < maxDE)
        {
            // fprintf(stderr, "here");
            unsigned int combinedAddr = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);
            if (de->DIR_Name[0] == 0xe5)
            {
                index++;
                de = (DirEntry *)(addr + offset + curClustOffset + index * sizeof(DirEntry));
                // continue;
            }
            else
            {
                char *name = malloc(13 * sizeof(char));
                bool hasDot = false;
                if (de->DIR_Name[8] != 0x20)
                {
                    hasDot = true;
                }
                int ind = 0;
                for (int i = 0; i < 11; i++)
                {
                    if (de->DIR_Name[i] != 0x20)
                    {
                        if (i == 8 && hasDot && name[ind - 1] != '.')
                        {
                            name[ind] = '.';
                            ind++;
                        }
                        name[ind] = de->DIR_Name[i];
                        ind++;
                    }
                    else
                    {
                        if (hasDot && i < 8)
                        {
                            name[ind] = '.';
                            ind++;
                            // increment to 8, which will be file ext
                            i = 7;
                        }
                        else
                        {

                            // ind++;
                            break;
                        }
                    }
                }
                name[ind] = '\0';

                if (de->DIR_Attr == 0x10)
                {
                    fprintf(stdout, "%s/ (starting cluster = %d)\n", name, combinedAddr);
                }
                else if (de->DIR_FileSize == 0)
                {

                    fprintf(stdout, "%s (size = 0)\n", name);
                }
                else
                {
                    fprintf(stdout, "%s (size = %u, starting cluster = %u)\n", name, de->DIR_FileSize, combinedAddr);
                }
                index++;
                total++;
                free(name);
                de = (DirEntry *)(addr + offset + curClustOffset + index * sizeof(DirEntry));
            }
        }
        // total += cnt;
        curClust = fat[curClust];
        // fprintf(stderr, "next cluster: %d\n", curClust);
    }

    // free(name);
    fprintf(stdout, "Total number of entries = %d\n", total);
}

void rSmallFile(char *diskName, char *fileName)
{
    // Open file
    int fd = open(diskName, O_RDWR);
    if (fd == -1)
        perror("err");

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("err");

    // Map file into memory
    char *addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("err");
        close(fd);
        return;
    }
    char *ck = ".";
    char *ext = strchr(fileName, '.');
    char *reformed = calloc(12, 1);
    if (ext != NULL)
    {
        int d = strcspn(fileName, ck);
        for (int j = 0; j < d; j++)
        {
            reformed[j] = fileName[j];
        }
        if (8 - d > 0)
        {
            // fprintf(stderr, "has space\n");
            for (int j = d; j < 8; j++)
            {
                reformed[j] = 0x20;
            }
        }
        int end = strlen(fileName) - d - 1;
        // fprintf(stderr, ". at : %d, has %d ext\n", d, end);
        for (int j = 8; j < 8 + end; j++)
        {
            reformed[j] = fileName[d + 1];
            // fprintf(stderr, "%c", reformed[j]);
            d++;
        }
        while (end < 3)
        {
            reformed[8 + end] = 0x20;
            end++;
        }
        // fprintf(stderr, "\n");
        // fprintf(stderr, "reformed: %s\n", reformed);
    }
    else
    {
        // reformed = fileName;
        for (int j = 0; j < (int)strlen(fileName); j++)
        {
            reformed[j] = fileName[j];
        }
        for (int j = strlen(fileName); j < 11; j++)
        {
            reformed[j] = 0x20;
        }
    }
    reformed[11] = '\0';
    BootEntry *be = (BootEntry *)(addr);
    unsigned int offset = be->BPB_FATSz32 * be->BPB_BytsPerSec * be->BPB_NumFATs + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec;
    int curRoot = be->BPB_RootClus;

    unsigned int *fat = (unsigned int *)(addr + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec);
    int maxDE = be->BPB_BytsPerSec * be->BPB_SecPerClus / sizeof(DirEntry);
    int foundNum = 0;
    unsigned int recovAddr;
    DirEntry *actual;
    // fprintf(stderr, "max de: %d\n", maxDE);
    while (curRoot < 0xffffff8)
    {

        // int c = 0;

        DirEntry *de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec);
        int i = 0;
        while (i < maxDE)
        {
            // fprintf(stderr, "cur: %d\n", i);
            // fprintf(stderr, "full: %s\n", de->DIR_Name);
            if (de->DIR_Name[0] == 0x00)
            {
                // fprintf(stderr, "empty after: %d\n", i);
                break;
            }
            else if (de->DIR_Name[0] == 0xE5 && de->DIR_Attr != 0x10)
            {
                bool found = true;

                for (int j = 1; j < 11; j++)
                {
                    if (de->DIR_Name[j] != reformed[j])
                    {
                        // fprintf(stderr, "err at %d\n", j);
                        // fprintf(stderr, "dirname: %u\n", de->DIR_Name[j]);
                        // fprintf(stderr, "reformed: %u\n", reformed[j]);
                        found = false;
                        break;
                    }
                }
                // fprintf(stderr, "found is %d\n", found);
                if (found)
                {
                    foundNum++;
                    if (foundNum > 1)
                    {
                        fprintf(stdout, "%s: multiple candidates found\n", fileName);
                        return;
                    }
                    recovAddr = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);
                    actual = de;
                }
            }
            i++;
            de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + i * sizeof(DirEntry));
        }
        curRoot = fat[curRoot];
    }
    if (foundNum == 1)
    {
        if (actual->DIR_FileSize > 0)
        {
            unsigned int clusterSpan = actual->DIR_FileSize / (be->BPB_SecPerClus * be->BPB_BytsPerSec);
            if (clusterSpan * (be->BPB_SecPerClus * be->BPB_BytsPerSec) < actual->DIR_FileSize)
            {
                clusterSpan++;
            }
            for (unsigned int k = 0; k < clusterSpan; k++)
            {
                if (k == clusterSpan - 1)
                {
                    fat[recovAddr] = 0x0ffffff8;
                }
                else
                {
                    fat[recovAddr] = recovAddr + 1;
                    recovAddr += 1;
                }
            }
        }

        actual->DIR_Name[0] = fileName[0];

        fprintf(stdout, "%s: successfully recovered\n", fileName);
        return;
    }
    fprintf(stdout, "%s: file not found\n", fileName);
}

void recoverSha(char *diskName, char *fileName, char *sha)
{
    // Open file
    int fd = open(diskName, O_RDWR);
    if (fd == -1)
        perror("err");

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("err");

    // Map file into memory
    char *addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("err");
        close(fd);
        return;
    }
    char *ck = ".";
    char *ext = strchr(fileName, '.');
    char *reformed = calloc(12, 1);
    if (ext != NULL)
    {
        int d = strcspn(fileName, ck);
        for (int j = 0; j < d; j++)
        {
            reformed[j] = fileName[j];
        }
        if (8 - d > 0)
        {
            // fprintf(stderr, "has space\n");
            for (int j = d; j < 8; j++)
            {
                reformed[j] = 0x20;
            }
        }
        int end = strlen(fileName) - d - 1;
        // fprintf(stderr, ". at : %d, has %d ext\n", d, end);
        for (int j = 8; j < 8 + end; j++)
        {
            reformed[j] = fileName[d + 1];
            // fprintf(stderr, "%c", reformed[j]);
            d++;
        }
        while (end < 3)
        {
            reformed[8 + end] = 0x20;
            end++;
        }
        // fprintf(stderr, "\n");
        // fprintf(stderr, "reformed: %s\n", reformed);
    }
    else
    {
        // reformed = fileName;
        for (int j = 0; j < (int)strlen(fileName); j++)
        {
            reformed[j] = fileName[j];
        }
        for (int j = strlen(fileName); j < 11; j++)
        {
            reformed[j] = 0x20;
        }
    }
    reformed[11] = '\0';
    BootEntry *be = (BootEntry *)(addr);
    unsigned int offset = be->BPB_FATSz32 * be->BPB_BytsPerSec * be->BPB_NumFATs + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec;
    int curRoot = be->BPB_RootClus;

    unsigned int *fat = (unsigned int *)(addr + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec);
    int maxDE = be->BPB_BytsPerSec * be->BPB_SecPerClus / sizeof(DirEntry);
    int foundNum = 0;
    unsigned int recovAddr;
    DirEntry *actual;
    // fprintf(stderr, "max de: %d\n", maxDE);
    while (curRoot < 0x0ffffff8)
    {

        // int c = 0;

        DirEntry *de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec);
        int i = 0;
        while (i < maxDE)
        {
            // fprintf(stderr, "cur: %d\n", i);
            // fprintf(stderr, "full: %s\n", de->DIR_Name);
            if (de->DIR_Name[0] == 0x00)
            {
                // fprintf(stderr, "empty after: %d\n", i);
                break;
            }
            else if (de->DIR_Name[0] == 0xE5 && de->DIR_Attr != 0x10)
            {
                bool found = true;

                for (int j = 1; j < 11; j++)
                {
                    if (de->DIR_Name[j] != reformed[j])
                    {
                        // fprintf(stderr, "err at %d\n", j);
                        // fprintf(stderr, "dirname: %u\n", de->DIR_Name[j]);
                        // fprintf(stderr, "reformed: %u\n", reformed[j]);
                        found = false;
                        break;
                    }
                }
                // fprintf(stderr, "found is %d\n", found);
                if (found)
                {
                    // Check for Sha
                    int startClus = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);

                    if (de->DIR_FileSize == 0)
                    {
                        if (memcmp(sha, "da39a3ee5e6b4b0d3255bfef95601890afd80709", SHA_DIGEST_LENGTH) == 0)
                        {
                            // fprintf(stderr, "zerolength file!\n");
                            foundNum++;
                        }
                    }
                    else
                    {
                        // fprintf(stderr, "starting clus: %d\n", startClus);
                        // fprintf(stderr, "filesize is : %u\n", de->DIR_FileSize);
                        // fprintf(stderr, "start byte: %s\n", (addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset));
                        if (de->DIR_FileSize <= be->BPB_BytsPerSec * be->BPB_SecPerClus)
                        {
                            unsigned char *ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                            unsigned char md[SHA_DIGEST_LENGTH + 1];
                            SHA1(ptrFile, de->DIR_FileSize, md);
                            md[SHA_DIGEST_LENGTH] = '\0';
                            // fprintf(stderr, "sha provided is %s\n", (unsigned char*)sha);
                            // fprintf(stderr, "sha computed is %s\n", md);
                            // fprintf(stderr, "size is : %lu\n", sizeof(sha));

                            char *hex = calloc(2 * SHA_DIGEST_LENGTH + 1, 1);
                            int k = 0;
                            int l = 0;
                            while (md[k] != '\0')
                            {
                                sprintf((char *)(hex + l), "%02x", md[k]);
                                k++;
                                l += 2;
                            }
                            hex[l++] = '\0';
                            // char * hex = (char*)strtol(sha, NULL, 16);
                            // fprintf(stderr, "hex: %s\n", hex);
                            if (strcmp(hex, sha) == 0)
                            {
                                // fprintf(stderr, "actually compared!\n");
                                foundNum++;
                            }
                            free(hex);
                        }
                        else
                        {
                            unsigned char *ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                            unsigned char md[SHA_DIGEST_LENGTH + 1];
                            unsigned int span = de->DIR_FileSize / (be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            unsigned long remainByte = de->DIR_FileSize - span * (be->BPB_SecPerClus * be->BPB_BytsPerSec);
                            // fprintf(stderr, "remain: %lu\n", remainByte);
                            if (remainByte > 0)
                            {
                                span++;
                            }
                            SHA_CTX ctx;
                            SHA1_Init(&ctx);
                            // First cluster
                            SHA1_Update(&ctx, ptrFile, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            unsigned int cur = 1;
                            while (cur < span)
                            {
                                ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset + cur * be->BPB_BytsPerSec * be->BPB_SecPerClus;
                                if (cur == span - 1)
                                {
                                    SHA1_Update(&ctx, ptrFile, remainByte);
                                }
                                else
                                {
                                    SHA1_Update(&ctx, ptrFile, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                }
                                cur++;
                            }
                            SHA1_Final(md, &ctx);
                            md[SHA_DIGEST_LENGTH] = '\0';
                            // fprintf(stderr, "sha provided is %s\n", (unsigned char*)sha);
                            // fprintf(stderr, "sha computed is %s\n", md);
                            // fprintf(stderr, "size is : %lu\n", sizeof(sha));

                            char *hex = calloc(2 * SHA_DIGEST_LENGTH + 1, 1);
                            int k = 0;
                            int l = 0;
                            while (md[k] != '\0')
                            {
                                sprintf((char *)(hex + l), "%02x", md[k]);
                                k++;
                                l += 2;
                            }
                            hex[l++] = '\0';
                            // char * hex = (char*)strtol(sha, NULL, 16);
                            // fprintf(stderr, "hex: %s\n", hex);
                            if (strcmp(hex, sha) == 0)
                            {
                                // fprintf(stderr, "actually compared!\n");
                                foundNum++;
                            }
                            free(hex);
                        }
                    }
                    if (foundNum > 1)
                    {
                        // fprintf(stderr, "number found %d\n", foundNum);
                        fprintf(stdout, "%s: multiple candidates found\n", fileName);
                        return;
                    }
                    recovAddr = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);
                    actual = de;
                }
            }
            i++;
            de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + i * sizeof(DirEntry));
        }
        curRoot = fat[curRoot];
    }
    if (foundNum == 1)
    {
        if (actual->DIR_FileSize > 0)
        {
            // recover fat table
            unsigned int clusterSpan = actual->DIR_FileSize / (be->BPB_SecPerClus * be->BPB_BytsPerSec);
            if (clusterSpan * (be->BPB_SecPerClus * be->BPB_BytsPerSec) < actual->DIR_FileSize)
            {
                clusterSpan++;
            }
            for (unsigned int k = 0; k < clusterSpan; k++)
            {
                if (k == clusterSpan - 1)
                {
                    fat[recovAddr] = 0x0ffffff8;
                }
                else
                {
                    fat[recovAddr] = recovAddr + 1;
                }
                recovAddr += 1;
            }
        }

        // Recover filename
        actual->DIR_Name[0] = fileName[0];

        fprintf(stdout, "%s: successfully recovered with SHA-1\n", fileName);
        return;
    }
    fprintf(stdout, "%s: file not found\n", fileName);
}

bool checkContains(int *arr, int target, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (target == arr[i])
        {
            return true;
        }
    }
    return false;
}

// int countPermutations(int *arr, int n, int r, int exclude)
// {
//     if (r == 0)
//         return 1;
//     int count = 0;
//     for (int i = 0; i < n; i++)
//     {
//         if (arr[i] != exclude)
//         {
//             count += countPermutations(arr, n, r - 1, exclude);
//         }
//     }
//     return count;
// }
// void combination(int* arr, int **res, int *r, int start, int end, int index, int span, int total){
//     if(index == span){
//         res[total] = r;
//         total++;
//         return;
//     }
//     else{
//         for (int i = start; i <= end && end - i + 1 >= span - index; i++){
//             r[index] = arr[i];
//             combination(arr, res, r, start + 1, end, index + 1, span, total);
//         }
//     }
// }
int *reform(int *arr, int len, int exclude)
{
    int *res = malloc((len - 1) * sizeof(int));
    for (int i = 0; i < len; i++)
    {
        if (arr[i] == exclude)
        {
            for (int j = i; j < len - 1; j++)
            {
                res[j] = arr[j + 1];
            }
            break;
        }
        else
        {
            res[i] = arr[i];
        }
    }
    return res;
}
void combination(int *arr, int **res, int *r, int start, int end, int index, int span)
{
    if (index == span)
    {
        for (int i = 0; i < span; i++)
        {
            // fprintf(stdout, "%d,", r[i]);
        }
        // fprintf(stdout, "\n");
        int k = 0;
        while (res[k] != 0)
        {
            k++;
        }
        // fprintf(stdout, "k is : %d\n", k);
        res[k] = malloc(sizeof(int) * span);
        for (int i = 0; i < span; i++)
        {
            res[k][i] = r[i];
        }
        return;
    }

    for (int i = start; i <= end && end - i + 1 >= span - index; i++)
    {
        r[index] = arr[i];
        combination(arr, res, r, i + 1, end, index + 1, span);

        // fprintf(stdout, "r[%d] is %d\n", index, arr[i]);

        // index = 0;
    }
}
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
void permutation(int *arr, int **res, int l, int r, int span)
{
    int i;
    if (l == r)
    {
        // int *r = malloc(sizeof(int) * span);
        //  for (int j = 0; j< span; j++){
        //      r[j] = arr[j];
        //  }
        int k = 0;
        while (res[k] != 0)
        {
            k++;
        }
        res[k] = malloc(sizeof(int) * span);
        for (int j = 0; j < span; j++)
        {
            res[k][j] = arr[j];
        }

        return;
    }
    else
    {
        for (i = l; i <= r; i++)
        {
            swap((arr + l), (arr + i));
            permutation(arr, res, l + 1, r, span);
            swap((arr + l), (arr + i));
        }
    }
}

void finale(char *diskName, char *fileName, char *sha)
{
    // fprintf(stderr, "diskname: %s\n", diskName);
    // fprintf(stderr, "filename: %s\n", fileName);
    // fprintf(stderr, "sha: %s\n", sha);

    // Open file
    int fd = open(diskName, O_RDWR);
    if (fd == -1)
        perror("err");

    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("err");

    // Map file into memory
    char *addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("err");
        close(fd);
        return;
    }
    char *ck = ".";
    char *ext = strchr(fileName, '.');
    char *reformed = calloc(12, 1);
    if (ext != NULL)
    {
        int d = strcspn(fileName, ck);
        for (int j = 0; j < d; j++)
        {
            reformed[j] = fileName[j];
        }
        if (8 - d > 0)
        {
            // fprintf(stderr, "has space\n");
            for (int j = d; j < 8; j++)
            {
                reformed[j] = ' ';
            }
        }
        int end = strlen(fileName) - d - 1;
        // fprintf(stderr, ". at : %d, has %d ext\n", d, end);
        for (int j = 8; j < 8 + end; j++)
        {
            reformed[j] = fileName[d + 1];
            // fprintf(stderr, "%c", reformed[j]);
            d++;
        }

        while (end < 3)
        {
            reformed[8 + end] = 0x20;
            end++;
        }
        // fprintf(stderr, "\n");
        // fprintf(stderr, "reformed: %s\n", reformed);
    }
    else
    {
        //reformed = fileName;
        for (int j = 0; j < (int)strlen(fileName); j++)
        {
            reformed[j] = fileName[j];
        }
        for (int j = strlen(fileName); j < 11; j++)
        {
            reformed[j] = 0x20;
        }
    }
    reformed[11] = '\0';
    BootEntry *be = (BootEntry *)(addr);
    unsigned int offset = be->BPB_FATSz32 * be->BPB_BytsPerSec * be->BPB_NumFATs + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec;
    int curRoot = be->BPB_RootClus;

    unsigned int *fat = (unsigned int *)(addr + be->BPB_RsvdSecCnt * be->BPB_BytsPerSec);
    int maxDE = be->BPB_BytsPerSec * be->BPB_SecPerClus / sizeof(DirEntry);
    int foundNum = 0;
    unsigned int recovAddr;
    DirEntry *actual;
    // fprintf(stderr, "max de: %d\n", maxDE);
    int *victory;
    while (curRoot < 0x0ffffff8)
    {

        // int c = 0;

        DirEntry *de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec);
        int i = 0;
        while (i < maxDE)
        {
            // fprintf(stderr, "cur: %d\n", i);
            // fprintf(stderr, "full: %s\n", de->DIR_Name);
            if (de->DIR_Name[0] == 0x00)
            {
                // fprintf(stderr, "empty after: %d\n", i);
                break;
            }
            else if (de->DIR_Name[0] == 0xE5 && de->DIR_Attr != 0x10)
            {
                bool found = true;

                for (int j = 1; j < 11; j++)
                {
                    if (de->DIR_Name[j] != reformed[j])
                    {
                        // fprintf(stderr, "err at %d\n", j);
                        // fprintf(stderr, "dirname: %u\n", de->DIR_Name[j]);
                        // fprintf(stderr, "reformed: %u\n", reformed[j]);
                        found = false;
                        break;
                    }
                }
                // fprintf(stderr, "found is %d\n", found);
                if (found)
                {
                    // fprintf(stderr, "m8 found: %d\n", i);
                    //  Check for Sha
                    unsigned int startClus = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);
                    // fprintf(stderr, "start clus is : %u\n", startClus);
                    //  file is empty
                    if (de->DIR_FileSize == 0)
                    {
                        if (memcmp(sha, "da39a3ee5e6b4b0d3255bfef95601890afd80709", SHA_DIGEST_LENGTH) == 0)
                        {
                            // fprintf(stderr, "zerolength file!\n");
                            foundNum++;
                        }
                    }
                    else
                    {

                        if (de->DIR_FileSize <= be->BPB_BytsPerSec * be->BPB_SecPerClus)
                        {
                            // fprintf(stderr, "small file\n");
                            unsigned char *ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                            unsigned char md[SHA_DIGEST_LENGTH + 1];
                            SHA1(ptrFile, de->DIR_FileSize, md);
                            md[SHA_DIGEST_LENGTH] = '\0';
                            // fprintf(stderr, "sha provided is %s\n", (unsigned char*)sha);
                            // fprintf(stderr, "sha computed is %s\n", md);
                            // fprintf(stderr, "size is : %lu\n", sizeof(sha));

                            char *hex = calloc(2 * SHA_DIGEST_LENGTH + 1, 1);
                            int k = 0;
                            int l = 0;
                            while (md[k] != '\0')
                            {
                                sprintf((char *)(hex + l), "%02x", md[k]);
                                k++;
                                l += 2;
                            }
                            hex[l++] = '\0';
                            // char * hex = (char*)strtol(sha, NULL, 16);
                            // fprintf(stderr, "hex: %s\n", hex);
                            if (strcmp(hex, sha) == 0)
                            {
                                // fprintf(stderr, "actually compared!\n");
                                foundNum++;
                            }
                            free(hex);
                        }
                        else
                        {
                            // fprintf(stderr, "bigfile\n");
                            //  brute force search

                            unsigned char *ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                            unsigned int clusterSpan = de->DIR_FileSize / (be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            unsigned int remainByte = de->DIR_FileSize - clusterSpan * (be->BPB_SecPerClus * be->BPB_BytsPerSec);
                            unsigned char md[SHA_DIGEST_LENGTH + 1];
                            victory = malloc(sizeof(int) * (int)(clusterSpan - 1));

                            md[SHA_DIGEST_LENGTH] = '\0';
                            // fprintf(stderr, "filesize: %u\n", de->DIR_FileSize);
                            // fprintf(stderr, "remainbyte: %u\n", remainByte);
                            if (remainByte > 0)
                            {
                                clusterSpan++;
                            }
                            // SHA_CTX ctx;
                            // SHA1_Init(&ctx);
                            // // First cluster
                            // SHA1_Update(&ctx, ptrFile, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            // unsigned int clustCount = 1;
                            unsigned int emp = 0;
                            int *emptyClust = calloc(18, sizeof(int));

                            for (int m = 2; m <= 20; m++)
                            {
                                if (fat[m] == 0x00000000)
                                {
                                    emptyClust[emp] = m;
                                    emp++;
                                }
                            }

                            unsigned int try = 1;
                            for (unsigned int n = 0; n < (clusterSpan - 1); n++)
                            {
                                try *= (emp - 1 - n);
                            }
                            // fprintf(stderr, "span is : %u\n", clusterSpan);
                            // fprintf(stderr, "empty clus count: %u\n", emp);
                            // fprintf(stderr, "total try: %u\n", try);

                            int *reformed2 = malloc(sizeof(int) * (emp - 1));
                            int comb = 1;
                            int div = 1;
                            for (unsigned int n = 1; n <= (clusterSpan - 1); n++)
                            {
                                div *= n;
                            }
                            comb = try / div;
                            int **combres = malloc(sizeof(int *) * comb);
                            int *combr = malloc(sizeof(int) * (clusterSpan - 1));
                            reformed2 = reform(emptyClust, emp, (int)startClus);

                            combination(reformed2, combres, combr, 0, emp - 2, 0, (int)clusterSpan - 1);

                            int **permres = malloc(sizeof(int *) * try);
                            for (int i = 0; i < comb; i++)
                            {
                                permutation(combres[i], permres, 0, (int)(clusterSpan - 2), (int)clusterSpan - 1);
                            }

                            for (int i = 0; i < (int)try; i++)
                            {
                                SHA_CTX ctx;
                                SHA1_Init(&ctx);
                                // First cluster
                                ptrFile = (unsigned char *)addr + (startClus - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                                SHA1_Update(&ctx, ptrFile, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int j = 0; j < (int)(clusterSpan - 1); j++)
                                {
                                    victory[j] = permres[i][j];
                                    // fprintf(stderr, "curcluster has %d, " , permres[i][j]);
                                    ptrFile = (unsigned char *)addr + (permres[i][j] - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + offset;
                                    if (j == (int)(clusterSpan - 1 - 1) && remainByte > 0)
                                    {
                                        SHA1_Update(&ctx, ptrFile, remainByte);
                                    }
                                    else
                                    {
                                        SHA1_Update(&ctx, ptrFile, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    }
                                }
                                // fprintf(stderr, "\n");
                                SHA1_Final(md, &ctx);
                                char *hex = calloc(2 * SHA_DIGEST_LENGTH + 1, 1);
                                int k = 0;
                                int l = 0;
                                while (md[k] != '\0')
                                {
                                    sprintf((char *)(hex + l), "%02x", md[k]);
                                    k++;
                                    l += 2;
                                }
                                hex[l++] = '\0';
                                // char * hex = (char*)strtol(sha, NULL, 16);
                                // fprintf(stderr, "hex: %s\n", hex);
                                if (strcmp(hex, sha) == 0)
                                {

                                    // fprintf(stderr, "actually compared!\n");
                                    foundNum++;
                                    break;
                                }
                                free(hex);
                            }

                            free(emptyClust);
                            free(combres);
                            free(permres);
                            free(combr);
                            free(reformed);
                            // free(hex);
                        }
                    }
                    if (foundNum > 1)
                    {
                        // fprintf(stderr, "number found %d\n", foundNum);
                        fprintf(stdout, "%s: multiple candidates found\n", fileName);
                        return;
                    }
                    recovAddr = (de->DIR_FstClusHI >> 8 | de->DIR_FstClusLO);
                    actual = de;
                }
            }
            i++;
            de = (DirEntry *)(addr + offset + (curRoot - 2) * be->BPB_SecPerClus * be->BPB_BytsPerSec + i * sizeof(DirEntry));
        }
        curRoot = fat[curRoot];
    }
    if (foundNum == 1)
    {
        if (actual->DIR_FileSize > 0)
        {
            unsigned int clusterSpan = actual->DIR_FileSize / (be->BPB_SecPerClus * be->BPB_BytsPerSec);
            if (clusterSpan * (be->BPB_SecPerClus * be->BPB_BytsPerSec) < actual->DIR_FileSize)
            {
                clusterSpan++;
            }
            // int i = 0;

            // fat = (unsigned int*) addr + be->BPB_RsvdSecCnt*be->BPB_BytsPerSec + i * be->BPB_FATSz32 * be->BPB_BytsPerSec;
            // fprintf(stderr, "fat[%d] starts at %u\n", i, be->BPB_RsvdSecCnt*be->BPB_BytsPerSec + i * be->BPB_FATSz32 * be->BPB_BytsPerSec);
            //  recover fat table
            for (unsigned int k = 0; k < clusterSpan; k++)
            {
                if (k == clusterSpan - 1)
                {
                    fat[recovAddr] = 0x0ffffff8;
                }
                else
                {
                    fat[recovAddr] = victory[k];
                    recovAddr = victory[k];
                }
                //recovAddr += 1;
            }
        }

        // Recover filename
        actual->DIR_Name[0] = fileName[0];

        fprintf(stdout, "%s: successfully recovered with SHA-1\n", fileName);
        return;
    }
    fprintf(stdout, "%s: file not found\n", fileName);
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

                rSmallFile((argv + 1)[0], optarg);
                // fprintf(stderr, "r then nothing\n");
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
                char *fileName = optarg;
                char *diskName = (argv + 1)[0];
                if ((opt = getopt(argc, argv, "s:") != -1))
                {

                    recoverSha(diskName, fileName, optarg);
                }
                else
                {
                    fprintf(stdout, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
                    return 0;
                }
            }
            else if (opt == 'R' && optarg)
            {
                char *fileName = optarg;
                char *diskName = (argv + 1)[0];
                if ((opt = getopt(argc, argv, "s:") != -1))
                {
                    finale(diskName, fileName, optarg);
                    // fprintf(stderr, "R then s\n");
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