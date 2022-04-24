/* 读取并处理初始目标函数及约束的模块 --> 化标准型
 * 非常感谢文章：https://www.bilibili.com/read/cv5287905
 */
#include "public.h"

#define BUFFER_SIZE_PER_ALLOC 100 // 每次分配给字符串暂存区的内存大小
#define RESET_BUFFER (char *) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char)) // 字符串暂存区，最开始分配100个

LPModel Parser(FILE *fp) { // 传入读取文件操作指针用于读取文件
    int stopReading = 0; // 停止读取的标志
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    int bufferLen = BUFFER_SIZE_PER_ALLOC; // 字符串暂存区长度，防止溢出
    char *buffer = RESET_BUFFER; // 字符串暂存区，最开始分配100个
    while (!stopReading && !feof(fp)) {
        currentChar = (char) fgetc(fp);
        if (!isspace(currentChar)) { // 字符不是空白符，推入buffer
            buffer[bufferPointer] = currentChar;
            bufferPointer++;
            if (bufferPointer >= bufferLen) {// 字符串暂存数组长度不够用了
                bufferLen += BUFFER_SIZE_PER_ALLOC; // 长度续上100
                buffer = (char *) realloc(buffer, sizeof(char) * bufferLen); // 重新分配内存，增长区域
                if (buffer != NULL) { // 分配成功
                    // 初始化从当前指针到新分配长度的末尾的内存
                    memset(buffer + bufferPointer, 0, sizeof(char) * BUFFER_SIZE_PER_ALLOC);
                } else {
                    printf("Memory re-allocation failed when reading file.");
                    exit(0); // 内存重分配失败，退出
                }
            }
        } else { // 遇到空白字符
            printf("len:%d, STR: %s\n", strlen(buffer), buffer);
            free(buffer); // 释放内存块，抛弃当前暂存区
            bufferPointer = 0; // 初始化暂存区指针
            bufferLen = BUFFER_SIZE_PER_ALLOC; // 初始化
            buffer = RESET_BUFFER; // 重设字符串暂存区
        }
    }
    free(buffer); // 释放暂存区
    buffer = NULL;
    LPModel result = {

    };
    return result;
}


