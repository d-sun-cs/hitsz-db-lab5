#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

int sortIntersection()
{
    printf("------------------------\n");
    printf("基于排序的集合的交算法: \n");
    printf("------------------------\n");

    Buffer buf; /* A buffer */
    // 每个关系分配1个内存块用于归并，用1个内存块缓存归并结果并输出至磁盘
    int groups = 2;
    unsigned char *blk;
    // 存储每个关系调入内存的块指针
    unsigned char *blks[groups];
    // 记录输出缓存写到了第几条记录（从0开始写，0到6）
    int recordCnt = 0;
    // 记录交结果要输出到的磁盘块
    int outAddr = 140;
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
    // 遍历每个关系每块的当前记录，找到相等的记录
    int A = -1;
    int B = -1;
    int C = -1;
    int D = -1;
    while(recordCnts[0] < RmaxRecordCnt && recordCnts[1] < SmaxRecordCnt)
    {
        record2XY(blks[0], recordCnts[0] % 7, &A, &B);
        record2XY(blks[1], recordCnts[1] % 7, &C, &D);
        // 交成功
        if (A == C && B == D)
        {
            // 如果已经写满一块，则输出至磁盘
            // 要排除是第一条记录的情况
            /* if (recordCnt % 7 == 0 && recordCnt != 0)
            {
                XY2record(blk, 7, outAddr + 1, -1);
                if (writeBlockToDisk(blk, outAddr++, &buf) != 0)
                {
                    perror("Writing Block Failed!\n");
                    return -1;
                }
                blk = getNewBlockInBuffer(&buf);
                printf("注：结果写入磁盘：%d\n", outAddr - 1);
            }
            // 存储至输出缓存块
            XY2record(blk, recordCnt++ % 7, A, B); */
            if (writeToOutBlk(&buf, &blk, &recordCnt, &outAddr, A, B) == -1)
            {
                return -1;
            }
            printf("(X=%d, Y=%d)\n", A, B);
            if (shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt) == -1)
            {
                break;
            }
            if (shiftRecord(&buf, &blks[1], &recordCnts[1], SmaxRecordCnt) == -1)
            {
                break;
            }
        }
        // 关系R后移一条记录
        else if (A < C || (A == C && B < D))
        {
            if (shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt) == -1)
            {
                break;
            }
        }
        // 关系S后移一条记录
        else
        {
            if (shiftRecord(&buf, &blks[1], &recordCnts[1], SmaxRecordCnt) == -1)
            {
                break;
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
    printf("S和R的交集有%d个元素。\n", recordCnt);
    printf("\n");
    freeBuffer(&buf);

    return 0;

}
