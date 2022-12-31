#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

void linearSelect()
{
    printf("----------------------------------\n");
    printf("基于线性搜索的选择算法 S.C = 128: \n");
    printf("----------------------------------\n");
    Buffer buf; /* A buffer */
    unsigned char *blk; // 用来不断读取新数据
    int rowCount = 0; // 满足条件的记录数
    unsigned char *resBlk; // 用来存放满足条件的、待输出的选择结果

    /* Initialize the buffer */
    // 1个内存块用来不断读取新数据
    // 1个内存块用来存放可能的选择结果
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    /* Read the block from the hard disk */
    for (int i = 17; i && i <= 48;)
    {
        if ((blk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }

        /* Process the data in the block */
        int X = -1;
        int Y = -1;
        int addr = -1;
        for (int j = 0; j < 7; j++) //一个blk存7个元组加一个地址
        {
            record2XY(blk, j, &X, &Y);
            if (X == 128)
            {
                printf("(X=%d, Y=%d)\n", X, Y);
                // 每超过7条，则输出一次结果
                if ((++rowCount - 1) % 7 == 0)
                {
                    // 需要排除一下刚得到一条结果的情况
                    if (rowCount != 1) {
                        // 将下一块地址写入内存块
                        XY2record(resBlk, 7, 100 + (rowCount / 7), -1);
                        // 将内存块写入磁盘块
                        if (writeBlockToDisk(resBlk, 100 + (rowCount / 7) - 1, &buf) != 0)
                        {
                            perror("Writing Block Failed!\n");
                            return -1;
                        }
                    }
                    resBlk = getNewBlockInBuffer(&buf);
                }
                XY2record(resBlk, (rowCount - 1) % 7, X, Y);
            }
        }
        i = nextAddr(blk);
        freeBlockInBuffer(blk, &buf);
    }


    // 将最后一个不超过7条记录的块写入磁盘
    if (resBlk && writeBlockToDisk(resBlk, 100 + (rowCount - 1) / 7, &buf) != 0)
    {
        perror("Writing Block Failed!\n");
        return -1;
    }

    // 输出写入磁盘的信息
    for (int i = 1; i <= rowCount; i += 7)
    {
        printf("注：结果写入磁盘：%d\n", 100 + (i - 1) / 7);
    }
    printf("\n");
    printf("满足选择条件的元组一共%d个\n", rowCount);
    printf("\n");
    printf("IO读写一共%d次\n", buf.numIO); /* Check the number of IO's */
    printf("\n");

    freeBuffer(&buf);

    return 0;
}



