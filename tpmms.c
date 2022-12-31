#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

#define SUBSET_BLK_NUM 8 // 8块为一个子集/一组
#define INTERNAL_R_BLK 601
#define INTERNAL_S_BLK 617

void internalSort(Buffer *buf, int beginBlk, int endBlk)
{
    unsigned char *blks[SUBSET_BLK_NUM]; // 存储每组内存块的指针
    for (int i = beginBlk, nxtAddr = beginBlk; nxtAddr && i <= endBlk; i++)
    {
        if ((blks[(i - 1) % SUBSET_BLK_NUM] = readBlockFromDisk(nxtAddr, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
        nxtAddr = nextAddr(blks[(i - 1) % SUBSET_BLK_NUM]);
        // printf("读入数据块%d\n", i);
        // 每读满SUBSET_BLK_NUM个块，进行一次内排序
        // 假设一组块数可以整除总块数，不单独处理最后一组未满的情况
        if (i % SUBSET_BLK_NUM == 0)
        {
            printf("开始内排序第%d块到第%d块\n", i - 7, i);
            // 冒泡排序
            // X和Y用于存储待比较的两条记录
            int X1 = -1;
            int Y1 = -1;
            int X2 = -1;
            int Y2 = -1;
            for (int j = 0; j < SUBSET_BLK_NUM * 7 - 1; j++)
            {
                for (int k = 0; k < SUBSET_BLK_NUM * 7 - (j + 1); k++)
                {
                    record2XY(blks[k / 7], k % 7, &X1, &Y1);
                    record2XY(blks[(k + 1) / 7], (k + 1) % 7, &X2, &Y2);
                    if (X1 > X2 || (X1 == X2 && Y1 > Y2))
                    {
                        XY2record(blks[k / 7], k % 7, X2, Y2);
                        XY2record(blks[(k + 1) / 7], (k + 1) % 7, X1, Y1);
                    }

                }
            }

            for (int j = 0; j < SUBSET_BLK_NUM; j++)
            {
                // 除了整个关系的最后一块，其他块都要设置后继块地址
                if (i == endBlk && j == SUBSET_BLK_NUM - 1)
                {
                    XY2record(blks[j], 7, -1, -1);
                }
                else
                {
                    XY2record(blks[j], 7, INTERNAL_R_BLK + i - SUBSET_BLK_NUM + j + 1, -1);
                }
                if (writeBlockToDisk(blks[j],
                                 INTERNAL_R_BLK + i - SUBSET_BLK_NUM + j,
                                 buf) != 0)
                {
                    perror("Writing Block Failed!\n");
                    return -1;
                }

            }
            printf("内排序结束，已输出至第%d块至第%d块\n",
                   INTERNAL_R_BLK + i - SUBSET_BLK_NUM,
                   INTERNAL_R_BLK + i - 1);
        }
    }
}

void externalSort(Buffer *buf, int beginBlk, int endBlk)
{
    // 每组分配1个内存块用于归并，用1个内存块缓存归并结果并输出至磁盘
    // 假设可以整除，且不超过7组，内存块够用
    int groups = (endBlk - beginBlk + 1) / SUBSET_BLK_NUM;
    unsigned char *blk;
    // 存储每组调入内存的块指针
    unsigned char *blks[groups];
    // 记录输出缓存写到了第几条记录（从0开始写，0到6）
    int recordCnt = 0;
    // 记录当前正在处理每组的第几条记录（从0开始，每组共 SUBSET_BLK_NUM * 7 条）
    int recordCnts[groups];

    // 先将每组的第0块读入内存
    for (int i = 0; i < groups; i++)
    {
        recordCnts[i] = 0;
        if ((blks[i] = readBlockFromDisk(beginBlk + i * SUBSET_BLK_NUM, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
    }
    // 申请1块作为输出缓存
    blk = getNewBlockInBuffer(buf);
    // 遍历每组每块的当前记录，找到最小记录
    int Xpre;
    int Ypre;
    int Xcur;
    int Ycur;
    // 记录每次遍历选择归并记录的组
    int select;
    // 每次归并一条记录至输出缓存，共 groups * SUBSET_BLK_NUM * 7 条
    for (int k = 0; k < groups * SUBSET_BLK_NUM * 7; k++)
    {
        Xpre = -1;
        Ypre = -1;
        Xcur = -1;
        Ycur = -1;
        select = -1;
        for (int i = 0; i < groups; i++)
        {
            // 如果该组已经处理完了最后一条记录，则跳过
            if (recordCnts[i] == -1)
            {
                continue;
            }
            record2XY(blks[i], recordCnts[i] % 7, &Xcur, &Ycur);
            if (Xcur < Xpre || (Xcur == Xpre && Ycur < Ypre) || Xpre == -1)
            {
                Xpre = Xcur;
                Ypre = Ycur;
                select = i;
            }
        }
        // 将最小记录写入输出缓存块，并后移该组的当前处理记录
        XY2record(blk, recordCnt++, Xpre, Ypre);
        recordCnts[select]++;
        // 如果输出缓存块写到了第7条，则输出至磁盘
        if (recordCnt == 7)
        {
            // 如果不是最后一次输出归并，则标记后继地址
            if (k != groups * SUBSET_BLK_NUM * 7 - 1)
            {
                XY2record(blk, 7, 301 + beginBlk - INTERNAL_R_BLK + k / 7 + 1, -1);
            }
            if (writeBlockToDisk(blk, 301 + beginBlk - INTERNAL_R_BLK + k / 7, buf) != 0)
            {
                perror("Writing Block Failed!\n");
                return -1;
            }
            blk = getNewBlockInBuffer(buf);
            recordCnt = 0;
            printf("输出归并结果至第%d块\n", 301 + beginBlk - INTERNAL_R_BLK + k / 7);
        }
        // 如果本次被选中的组已经处理完最后一条记录，则标记其为-1
        if (recordCnts[select] == SUBSET_BLK_NUM * 7)
        {
            printf("处理完第%d组的磁盘块%d，该组处理完毕\n",
                   select,
                   beginBlk + select * SUBSET_BLK_NUM + recordCnts[select] / 7 - 1);
            recordCnts[select] = -1;
        }
        // 如果不是最后一条记录，且已经处理完一个块，则读入下一块
        else if (recordCnts[select] % 7 == 0)
        {
            int nxtAddr = nextAddr(blks[select]);
            freeBlockInBuffer(blks[select], buf);
            if ((blks[select] = readBlockFromDisk(nxtAddr, buf)) == NULL)
            {
                perror("Reading Block Failed!\n");
                return -1;
            }
            printf("处理完第%d组的磁盘块%d，读入下一块\n",
                   select,
                   beginBlk + select * SUBSET_BLK_NUM + recordCnts[select] / 7 - 1);
        }
    }
}

void tpmms()
{
    printf("------------------------\n");
    printf("两阶段多路归并排序算法: \n");
    printf("------------------------\n");

    Buffer buf; /* A buffer */

    int i = 0;


    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    // 分别内排序关系R和关系S
    // 内排序中间结果暂存至501-548块
    internalSort(&buf, 1, 16);
    internalSort(&buf, 17, 48);

    // 分别外排序关系R和关系S
    // 将外排序结果存储至301-348块
    externalSort(&buf, INTERNAL_R_BLK, INTERNAL_R_BLK + 15);
    externalSort(&buf, INTERNAL_S_BLK, INTERNAL_S_BLK + 31);

    // 擦除内排序中间结果
    for (int i = INTERNAL_R_BLK; i <= INTERNAL_S_BLK + 31; i++)
    {
        dropBlockOnDisk(i);
    }
    printf("已擦除磁盘块%d至磁盘块%d上的中间结果\n", INTERNAL_R_BLK, INTERNAL_S_BLK + 31);
    printf("\n");

    freeBuffer(&buf);

}
