#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

void record2XY(unsigned char *blk, int recordNum, int *X, int *Y)
{
    char str[5];
    for (int k = 0; k < 4; k++)
    {
        str[k] = blk[recordNum * 8 + k];
    }
    *X = atoi(str);
    for (int k = 0; k < 4; k++)
    {
        str[k] = blk[recordNum * 8 + 4 + k];
    }
    *Y = atoi(str);
}

void XY2record(unsigned char *blk, int recordNum, int X, int Y)
{
    unsigned char fourBytes[4];
    if (X != -1)
    {
        sprintf(fourBytes, "%d", X);
        for (int k = 0; k < 4; k++)
        {
            blk[recordNum * 8 + k] = fourBytes[k];
        }
    }
    else
    {
        for (int k = 0; k < 4; k++)
        {
            blk[recordNum * 8 + k] = 0;
        }
    }
    if (Y != -1)
    {
        sprintf(fourBytes, "%d", Y);
        for (int k = 0; k < 4; k++)
        {
            blk[recordNum * 8 + 4 + k] = fourBytes[k];
        }
    }
    else
    {
        for (int k = 0; k < 4; k++)
        {
            blk[recordNum * 8 + 4 + k] = 0;
        }
    }
}

int nextAddr(unsigned char *blk)
{
    char str[5];
    for (int k = 0; k < 4; k++)
    {
        str[k] = blk[7 * 8 + k];
    }
    return atoi(str);
}

/**
* 返回值：
*   -1代表已经读完了所有记录
*   0代表只是后移记录，并没有读入新块
*   非0非-1代表读入了新块，返回的是新块号
*/
int shiftRecord(Buffer *buf, unsigned char **blk, int *recordCnt, int maxRecordCnt)
{
    ++(*recordCnt);
    // 如果已经达到最后一条记录，则结束连接
    if (*recordCnt == maxRecordCnt)
    {
        return -1;
    }
    // 如果下一条记录在下一个块中，则读入下一块
    else if (*recordCnt % 7 == 0)
    {
        int nxtAddr = nextAddr(*blk);
        freeBlockInBuffer(*blk, buf);
        if ((*blk = readBlockFromDisk(nxtAddr, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
        return nxtAddr;
    }
    return 0;
}

int writeToOutBlk(Buffer *buf, unsigned char **outBlk, int *recordCnt, int *outAddr, int X, int Y)
{
    // 如果已经写满一块，则输出至磁盘
    // 要排除是第一条记录的情况
    if (*recordCnt % 7 == 0 && *recordCnt != 0)
    {
        XY2record(*outBlk, 7, *outAddr + 1, -1);
        if (writeBlockToDisk(*outBlk, (*outAddr)++, buf) != 0)
        {
            perror("Writing Block Failed!\n");
            return -1;
        }
        *outBlk = getNewBlockInBuffer(buf);
        printf("注：结果写入磁盘：%d\n", *outAddr - 1);
    }
    // 存储至输出缓存块
    XY2record(*outBlk, (*recordCnt)++ % 7, X, Y);
}
