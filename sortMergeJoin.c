#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"



void sortMergeJoin()
{
    printf("--------------------\n");
    printf("基于排序的连接算法: \n");
    printf("--------------------\n");

    Buffer buf; /* A buffer */
    // 每个关系分配1个内存块用于归并，用1个内存块缓存归并结果并输出至磁盘
    int groups = 2;
    unsigned char *blk;
    // 存储每个关系调入内存的块指针
    unsigned char *blks[groups];
    // 记录输出缓存写到了第几条记录（从0开始写，0到5，相邻两条记录算一条连接结果）
    int recordCnt = 0;
    // 记录连接结果要输出到的磁盘块
    int outAddr = 401;
    // 记录当前正在处理每个关系的第几条记录
    // 从 0 开始，关系 R 共 112 条，关系 S 共 224 条
    int recordCnts[groups];
    int RmaxRecordCnt = 112;
    int SmaxRecordCnt = 224;
    int curRBlk;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    // 先将每个关系的第0块读入内存
    recordCnts[0] = 0;
    curRBlk = 0;
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
    // 遍历每个关系每块的当前记录，找到可以连接的记录
    int A = -1;
    int B = -1;
    int C = -1;
    int D = -1;
    while(recordCnts[0] < RmaxRecordCnt && recordCnts[1] < SmaxRecordCnt)
    {
        record2XY(blks[0], recordCnts[0] % 7, &A, &B);
        record2XY(blks[1], recordCnts[1] % 7, &C, &D);
        // printf("读取了关系R的第%d条，关系S的第%d条\n", recordCnts[0], recordCnts[1]);
        // printf("R.A=%d，S.C=%d\n", A, C);
        // 可以连接
        if (A == C)
        {
            int eqlVal = A;
            // 固定关系R的指针
            int fixR = recordCnts[0];
            int fixBlk = curRBlk;
            // 遍历关系S连续相等的记录，分别连接关系R中连续相等的记录
            int RnoMore = 0;
            int noMore = 0;
            while (C == eqlVal)
            {
                // 连接关系R中连续相等的记录
                recordCnts[0] = fixR;
                if (curRBlk != fixBlk)
                {
                    freeBlockInBuffer(blks[0], &buf);
                    if ((*blks[0] = readBlockFromDisk(fixBlk, &buf)) == NULL)
                    {
                        return -1;
                    }
                    curRBlk = fixBlk;
                }
                record2XY(blks[0], recordCnts[0] % 7, &A, &B);
                // printf("读取了关系R的第%d条\n", recordCnts[0]);
                // printf("R.A=%d\n", A);
                while (A == eqlVal)
                {
                    // 如果已经写满一块，则输出至磁盘
                    // 要排除是第一条记录的情况
                    if (recordCnt % 6 == 0 && recordCnt != 0)
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
                    XY2record(blk, recordCnt++ % 6, A, B);
                    XY2record(blk, recordCnt++ % 6, C, D);
                    // printf("连接了关系R的第%d条，关系S的第%d条\n", recordCnts[0], recordCnts[1]);
                    // 关系R后移一条记录
                    int res;
                    if ((res = shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt)) == -1)
                    {
                        RnoMore = 1;
                        break;
                    }
                    if (res)
                    {
                        curRBlk = res;
                    }
                    record2XY(blks[0], recordCnts[0] % 7, &A, &B);
                    // printf("读取了关系R的第%d条\n", recordCnts[0]);
                    // printf("R.A=%d\n", A);
                }
                // 如果在连接过程中R已经达到最后一条记录，则本轮连续连接结束后，再结束连接
                if (RnoMore)
                {
                    noMore = 1;
                }
                // 关系S后移一条记录
                if (shiftRecord(&buf, &blks[1], &recordCnts[1], SmaxRecordCnt) == -1)
                {
                    noMore = 1;
                    break;
                }
                record2XY(blks[1], recordCnts[1] % 7, &C, &D);
                // printf("读取了关系S的第%d条\n", recordCnts[1]);
                // printf("S.C=%d\n", C);
            }
            // 如果在连接过程中S已经达到最后一条记录，则结束连接
            if (noMore)
            {
                break;
            }
        }
        // 关系R后移一条记录
        else if (A < C)
        {
            int res;
            if ((res = shiftRecord(&buf, &blks[0], &recordCnts[0], RmaxRecordCnt)) == -1)
            {
                break;
            }
            // 需要记录关系R读到了哪一块
            if (res)
            {
                curRBlk = res;
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
    printf("总共连接%d次。\n", recordCnt / 2);
    printf("\n");
    freeBuffer(&buf);
}
