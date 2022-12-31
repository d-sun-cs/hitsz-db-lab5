#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

int buildIndex(int beginIdx)
{
    Buffer buf; /* A buffer */
    unsigned char *dataBlk; // 数据块
    unsigned char *idxBlk; // 索引块
    int idxCnt = 0; // 记录当前正在创建索引块的第几个指针
    int idxOutAddr = 350; // 记录索引块的要输出到的磁盘块号
    int idxData = -1, Y = -1;
    int preIdxData = -1;

    /* Initialize the buffer */
    // 1个内存块用作索引块，为7个磁盘块建立索引
    // 1个内存块用于暂存数据
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    idxBlk = getNewBlockInBuffer(&buf);
    for (int i = beginIdx; i;)
    {
        if ((dataBlk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
        // 读取数据块第一条数据，用于创建指针
        record2XY(dataBlk, 0, &idxData, &Y);
        // 在索引块中创建一个指针（索引键值不重复）
        if (idxData != preIdxData)
        {
            preIdxData = idxData;
            XY2record(idxBlk, idxCnt, idxData, i);
            // 如果索引块中已经包含7个指针，则输出索引块
            if (++idxCnt == 7)
            {
                XY2record(idxBlk, idxCnt, idxOutAddr + 1, -1);
                if (writeBlockToDisk(idxBlk, idxOutAddr++, &buf) != 0)
                {
                    perror("Writing Block Failed!\n");
                    return -1;
                }
                idxBlk = getNewBlockInBuffer(&buf);
                idxCnt = 0;
            }
        }
        i = nextAddr(dataBlk);
        freeBlockInBuffer(dataBlk, &buf);
    }
    // 输出最后一个不满7个指针的索引块
    if (idxCnt != 0)
    {
        if (writeBlockToDisk(idxBlk, idxOutAddr, &buf) != 0)
        {
            perror("Writing Block Failed!\n");
            return -1;
        }
    }

    freeBuffer(&buf);

    return 350;
}

void indexSelect()
{
    int C = 128;
    printf("------------------------------\n");
    printf("基于索引的选择算法 S.C = %d: \n", C);
    printf("------------------------------\n");

    // 为S.C建立索引
    int idxBeginAddr = buildIndex(317);

    Buffer buf; /* A buffer */
    unsigned char *dataBlk; // 数据块
    unsigned char *idxBlk; // 索引块
    unsigned char *resBlk; // 用来存放满足条件的、待输出的选择结果
    int rowCount = 0; // 满足条件的记录数
    int idxData = -1, idxPtr = -1;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    for (int i = idxBeginAddr; i;)
    {
        if ((idxBlk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
        printf("读入索引块%d\n", i);
        int flag = 0;
        int j = 0;
        for (j = 0; j < 7; j++)
        {
            record2XY(idxBlk, j, &idxData, &idxPtr);
            // 第一次找到索引值大于目标值的索引项，则回退索引项
            if (idxData > C)
            {
                flag = 1;
                break;
            }
        }
        // 第一次找到索引值大于目标值的索引项，则回退索引项
        if (flag)
        {
            int preIdxData = -1, preIdxPtr = -1;
            int X = -1, Y = -1;

            if (j)
            {
                record2XY(idxBlk, j - 1, &preIdxData, &preIdxPtr);

            }
            // 如果上一条索引项不在当前索引块中，则需要再次读入上一索引块
            else
            {
                if (i)
                {
                    if ((idxBlk = readBlockFromDisk(i - 1, &buf)) == NULL)
                    {
                        perror("Reading Block Failed!\n");
                        return -1;
                    }
                    printf("读入索引块%d\n", i - 1);
                    record2XY(idxBlk, 6, &preIdxData, &preIdxPtr);
                }
                // 如果当前的就是第1个索引块，说明整个关系中没有满足条件的记录
                else
                {
                    break;
                }
            }
            int noMore = 0;
            for (int k = preIdxPtr; k && k < idxPtr;)
            {
                if ((dataBlk = readBlockFromDisk(k, &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                printf("读入数据块%d\n", k);
                for (int l = 0; l < 7; l++)
                {
                    record2XY(dataBlk, l, &X, &Y);
                    if (X == C)
                    {
                        printf("(X=%d, Y=%d)\n", X, Y);
                        // 每超过7条，则输出一次结果
                        if ((++rowCount - 1) % 7 == 0)
                        {
                            // 需要排除一下刚得到一条结果的情况
                            if (rowCount != 1) {
                                // 将下一块地址写入内存块
                                XY2record(resBlk, 7, 120 + (rowCount / 7), -1);
                                // 将内存块写入磁盘块
                                if (writeBlockToDisk(resBlk, 120 + (rowCount / 7) - 1, &buf) != 0)
                                {
                                    perror("Writing Block Failed!\n");
                                    return -1;
                                }
                            }
                            resBlk = getNewBlockInBuffer(&buf);
                        }
                        XY2record(resBlk, (rowCount - 1) % 7, X, Y);
                    }
                    // 记录是有序的，
                    // 如果当前记录已经大于目标值，则后面不可能再有满足条件的记录
                    else if (X > C)
                    {
                        noMore = 1;
                        break;
                    }
                }
                k = nextAddr(dataBlk);
                freeBlockInBuffer(dataBlk, &buf);
                if (noMore)
                {
                    break;
                }
            }
            break;
        }
        else
        {
            printf("没有满足条件的元组\n");
        }
    }

    // 将最后一个不超过7条记录的块写入磁盘
    if (resBlk && writeBlockToDisk(resBlk, 120 + (rowCount - 1) / 7, &buf) != 0)
    {
        perror("Writing Block Failed!\n");
        return -1;
    }

    // 输出写入磁盘的信息
    for (int i = 1; i <= rowCount; i += 7)
    {
        printf("注：结果写入磁盘：%d\n", 120 + (i - 1) / 7);
    }
    printf("\n");
    printf("满足选择条件的元组一共%d个\n", rowCount);
    printf("\n");
    printf("IO读写一共%d次\n", buf.numIO); /* Check the number of IO's */
    printf("\n");

    freeBuffer(&buf);

    return 0;
}
