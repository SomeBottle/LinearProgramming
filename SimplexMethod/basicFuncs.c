#include "public.h"

SplitResult SplitByChr(char *str, char chr) { // (字符串,字符) 按字符分隔字符串，会返回一个二维数组
    int i;
    int stringLen=strlen(str);
    int bufferSize = sizeof(char) * stringLen;
    char *buffer = (char *) malloc(bufferSize); // 字符串暂存区
    int bufferLen = 0; // 暂存区字符数组长度
    char **arr = (char **) malloc(sizeof(char *) * stringLen); // 数组第一维
    int arrLen = 0; // 返回二维数组第一维的大小
    for (i = 0; i < stringLen + 1; i++) {
        int currentChr;
        int lastOne = 0; // 最后一项单独处理
        if (i < stringLen) {
            currentChr = str[i];
        } else {
            lastOne = 1;
        }
        if (lastOne || currentChr == chr) {
            arr[arrLen] = (char *) malloc(sizeof(char) * (bufferLen + 1)); // 初始化第二维数组（需要多一位来存放\0）
            strncpy(arr[arrLen], buffer, bufferLen); // 将字符装入第二维数组
            arr[arrLen][bufferLen] = '\0'; // 手动构造成一个字符串
            memset(buffer, 0, bufferSize); // 清空字符串暂存区
            bufferLen = 0; // 暂存区长度归零
            arrLen++;
        } else {
            buffer[bufferLen++] = currentChr; // 存入字符串暂存区
        }
    }
    free(buffer); // 释放暂存区
    SplitResult result = {
            arr, // 一定要记得用完释放！！！
            arrLen
    };
    return result; // 返回结果
}

int freeSplitArr(SplitResult *rs) { // 门当对户地释放SplitByChr的返回结果中的字符二维数组
    int i;
    for (i = 0; i < rs->len; i++) {
        free(rs->split[i]);
    }
    free(rs->split);
    rs->split = NULL;
    return 1;
}
