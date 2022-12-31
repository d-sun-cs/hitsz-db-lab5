#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

void sortUnion()
{
    printf("------------------------\n");
    printf("基于排序的集合的并算法: \n");
    printf("------------------------\n");

    Buffer buf; /* A buffer */
    // 每个关系分配1个内存块用于归并，用1个内存块缓存归并结果并输出至磁盘
    int groups = 2;
    unsigned char *blk;
    // 存储每个关系调入内存的块指针
    unsigned char *blks[groups];
    // 记录输出缓存写到了第几条记录（从0开始写，0到6）
    int recordCnt = 0;
    // 记录并结果要输出到的磁盘块
    int outAddr = 801;
    // 记录当前正在处理每个关系的第几条记录
    // 从 0 开始，关系 R 共 112 条，关系 S 共 224 条
    int recordCnts[groups];
    int RmaxRecordCnt = 112;
    int SmaxRecordCnt = 224;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    // 先将每个关系的第0块读入内存
    recordCnts[0] = 0;
    if ((blks[0] = readBlockFromDisk(301, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }
    recordCnts[1] = 0;
    if ((blks[1] = readBlockFromDisk(317, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }

    // 申请1块作为输出缓存
    blk = getNewBlockInBuffer(&buf);
    // 遍历每个关系每块的当前记录，相等的去重，保证结果有序
    int A = -1;
    int B = -1;
    int C = -1;
    int D = -1;
    int noMoreR = 0, noMoreS = 0;
    while(!noMoreR || !noMoreS)
    {
        record2XY(blks[0], recordCnts[0] % 7, &A, &B);
        record2XY(blks[1], recordCnts[1] % 7, &C, &D);
        // 需要去重，只输出1条
        if (A == C && B == D && !noMoreR && !noMoreS)
        {
            if (writeToOutBlk(&buf, &blk, &recordCnt, &outAddr, A, B) == -1)
            {
                return -1;
            }
            if (shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt) == -1)
            {
                noMoreR = 1;
            }
            if (shiftRecord(&buf, &blks[1], &recordCnts[1], SmaxRecordCnt) == -1)
            {
                noMoreS = 1;
            }
        }
        // 关系R先输出
        else if ((noMoreS || A < C || (A == C && B < D)) && !noMoreR)
        {
            if (writeToOutBlk(&buf, &blk, &recordCnt, &outAddr, A, B) == -1)
            {
                return -1;
            }
            if (shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt) == -1)
            {
                noMoreR = 1;
            }
        }
        // 关系S先输出
        else if (!noMoreS)
        {
            if (writeToOutBlk(&buf, &blk, &recordCnt, &outAddr, C, D) == -1)
            {
                return -1;
            }
            if (shiftRecord(&buf, &blks[1], &recordCnts[1], SmaxRecordCnt) == -1)
            {
                noMoreS = 1;
            }
        }
    }
    // 如果还有剩余连接记录，则输出最后一块
    if (recordCnt)
    {
        if (writeBlockToDisk(blk, outAddr, &buf) != 0)
        {
            perror("Writing Block Failed!\n");
            return -1;
        }
        printf("注：结果写入磁盘：%d\n", outAddr);
    }
    printf("\n");
    printf("S和R的并集有%d个元素。\n", recordCnt);
    printf("\n");
    freeBuffer(&buf);
    return 0;
}
