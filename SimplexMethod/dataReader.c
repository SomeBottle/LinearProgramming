/* 读取并处理初始目标函数及约束的模块 --> 化标准型
 * 非常感谢文章：https://www.bilibili.com/read/cv5287905
 */
#include "public.h"

#define BUFFER_SIZE_PER_ALLOC 100 // 每次分配给字符串暂存区的内存大小
#define RESET_BUFFER (char *) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char)) // 字符串暂存区，最开始分配100个

static int readFlag = 0; // 正在读取哪个部分，为1代表在读目标函数LF，为2代表在读约束ST，分开处理。

int InterruptBuffer(char x);

LPModel Parser(FILE *fp);

int writeIn(LF *linearFunc, ST *subjectTo, char *str);


int InterruptBuffer(char x) { // 什么时候截断字符串暂存
    static int bracket = 0; // 是否进入{ }括起来的区域，该变量放在静态储存区
    if (x == '{') { // 进入{区域
        bracket = 1;
        return 1;
    } else if (x == '}') { // 离开}区域
        bracket = 0;
        readFlag = 0; // 离开大括号说明读取完毕了，设置读取标记为0
        return 1;
    }
    // 进入括号区域后就接受空格，但是遇到分号还是会截断字符串，起到分隔的作用
    return (!bracket && isspace(x)) || x == ';';
}

LPModel Parser(FILE *fp) { // 传入读取文件操作指针用于读取文件
    LF linearFunc; // 初始化目标函数结构体
    ST subjectTo; // 初始化约束结构体
    int errorOccurs = 0; // 是否发生错误
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    int bufferLen = BUFFER_SIZE_PER_ALLOC; // 字符串暂存区长度，防止溢出
    char *buffer = RESET_BUFFER; // 字符串暂存区，最开始分配100个
    while (!feof(fp)) {
        currentChar = (char) fgetc(fp);
        if (!InterruptBuffer(currentChar)) { // 字符不是空白符，推入buffer
            if (isspace(currentChar)) continue; // 跳过{ }中所有空格
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
                    errorOccurs = 1;
                    break; // 内存重分配失败，退出
                }
            }
        } else if (strlen(buffer) > 0) { // 遇到空白字符, 暂存区中有内容就进行处理
            if (strncmp(buffer, "LF", 2) == 0) {
                readFlag = 1; // 正在读取目标函数LF
            } else if (strncmp(buffer, "ST", 2) == 0) {
                readFlag = 2; // 正在读取约束ST
            } else if (readFlag != 0) { // 交给对应的函数将数据读入结构体
                int writeResult = writeIn(&linearFunc, &subjectTo, buffer);
                if (!writeResult) {
                    errorOccurs = 1;
                    break; // 解析数据失败，中止
                }
            }
            printf("len:%d, Str: %s\n", strlen(buffer), buffer);
            free(buffer); // 释放内存块，抛弃当前暂存区
            bufferPointer = 0; // 初始化暂存区指针
            bufferLen = BUFFER_SIZE_PER_ALLOC; // 初始化
            buffer = RESET_BUFFER; // 重设字符串暂存区
        }
    }
    free(buffer); // 释放暂存区
    buffer = NULL;
    if (errorOccurs) { // 发生了错误

    }
    LPModel result = {

    };
    return result;
}

int writeIn(LF *linearFunc, ST *subjectTo, char *str) { // 将数据(str)解析后写入LF或者ST
    switch (readFlag) {
        SplitResult colonSp;
        case 1: // 写入LF
            colonSp = SplitByChr(str, strlen(str), ':');
            break;
        case 2: // 写入ST

            break;
    }
}
