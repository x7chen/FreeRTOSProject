/********************************************************************
*  File Name: asciicvt.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdio.h>
#include <string.h>

void Hex2Str(const char *sSrc, char *sDest, int nSrcLen)
{
    int i;
    char szTmp[3];

    for (i = 0; i < nSrcLen; i++)
    {
        sprintf(szTmp, "%02X", (unsigned char)sSrc[i]);
        memcpy(&sDest[i * 2], szTmp, 2);
    }
    return;
}

int IsHexChar(char ch)
{
    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
        return 1;
    else
        return 0;
}
unsigned char CharToValue(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return c;
}

unsigned char TwoCharToByte(char h, char l)
{
    return (unsigned char)(CharToValue(h) * 16 + CharToValue(l));
}

void Str2Hex(const char *sSrc, char *bDest, int nSrcLen)
{
    int cnt = 0;
    int i = 0;
    int n;
    for (n = 0; n < nSrcLen; n += 2)
    {
        if (IsHexChar(sSrc[i]) && IsHexChar(sSrc[i + 1]))
        {
            bDest[cnt] = TwoCharToByte(sSrc[i], sSrc[i + 1]);
            cnt++;
            i += 2;
        }
        else
            break;
    }
}
