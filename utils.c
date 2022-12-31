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
