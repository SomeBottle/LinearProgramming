/* 读取并处理初始目标函数及约束的模块 --> 化标准型
 * 非常感谢文章：https://www.bilibili.com/read/cv5287905
 */
#include "public.h"

#define BUFFER_SIZE_PER_ALLOC 100 // 每次分配给字符串暂存区的内存大小
#define RESET_BUFFER (char *) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char)) // 字符串暂存区，最开始分配100个

char **constants = NULL; // 常数项指针数组
int constArrLen = BUFFER_SIZE_PER_ALLOC; // 常量项数组长度，防止溢出用
int constantsNum = 0; // 常数项数量

// 正在读取哪个部分，为1代表在读目标函数LF，为2代表在读约束ST，3则代表在读取常量CONSTANTS，分开处理。
static int readFlag = 0;

int InterruptBuffer(char x);

LPModel Parser(FILE *fp);

int FormulaParser(LF *linearFunc, ST *subjectTo, char *str);

int WriteIn(LF *linearFunc, ST *subjectTo, char *str);


int InterruptBuffer(char x) { // 什么时候截断字符串暂存
    static int bracket = 0; // 是否进入{ }括起来的区域，该变量放在静态储存区
    if (x == '{') { // 进入{区域
        bracket = 1;
        return 1;
    } else if (x == '}') { // 离开}区域
        bracket = 0;
        return 1;
    }
    // 进入括号区域后就接受空格，但是遇到分号还是会截断字符串，起到分隔的作用
    return (!bracket && isspace(x)) || x == ';';
}

LPModel Parser(FILE *fp) { // 传入读取文件操作指针用于读取文件
    LF linearFunc; // 初始化目标函数结构体
    ST *subjectTo = NULL; // 初始化约束结构体
    int errorOccurs = 0; // 是否发生错误
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    int bufferLen = BUFFER_SIZE_PER_ALLOC; // 字符串暂存区长度，防止溢出
    char *buffer = RESET_BUFFER; // 字符串暂存区，最开始分配100个
    if (constants == NULL) { // 全局变量在外层声明时无法被赋值，只能在这里赋值了
        constants = (char **) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char *));
    }
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
            if (strcmp(buffer, "LF") == 0) {
                readFlag = 1; // 正在读取目标函数LF
            } else if (strcmp(buffer, "ST") == 0) {
                readFlag = 2; // 正在读取约束ST
            } else if (strcmp(buffer, "CONSTANTS") == 0) {
                readFlag = 3; // 正在读取常量列表
            } else if (readFlag != 0) { // 交给对应的函数将数据读入结构体
                int writeResult = WriteIn(&linearFunc, subjectTo, buffer);
                if (!writeResult) {
                    errorOccurs = 1;
                    break; // 解析数据失败，中止
                }
                readFlag = 0; // 读取完毕
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

int FormulaParser(LF *linearFunc, ST *subjectTo, char *str) { // 将方程字符串处理为对应结构体
    int status = 1; // 返回码
    int i, len = strlen(str);
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    char *buffer = (char *) calloc(len, sizeof(char)); // 字符串暂存区，最开始分配100个
    for (i = 0; i < len; i++) {
        currentChar = str[i];
        if (currentChar == '+' || currentChar == '-') { // 是加号或减号，这是每一项的划分标志

        } else if (strchr(">=<", currentChar) != NULL) { // 是关系符号，这是方程左右的划分标志

        } else {
            buffer[bufferPointer] = currentChar; // 逐字符处理
        }
    }
    if (readFlag == 1) { // 处理目标函数
    } else if (readFlag == 2) {

    }
    free(buffer); // 释放暂存区
    return status;
}

int WriteIn(LF *linearFunc, ST *subjectTo, char *str) { // 将数据(str)解析后写入LF或者ST
    int status = 1; // 返回码
    int stringLen = strlen(str);
    SplitResult colonSp; // 初始化分割字符串
    switch (readFlag) {
        case 1: // 写入LF
            // 根据冒号分割
            colonSp = SplitByChr(str, ':');
            if (colonSp.len < 2) { // 目标函数没有指定maximize or minimize，无效
                printf("Objective function invalid.\n");
                status = 0;
            } else if (strcmp(colonSp.split[0], "max") || strcmp(colonSp.split[0], "min")) { // 必须要是max/min
                strcpy(linearFunc->type, colonSp.split[0]); // 写入max/min
                FormulaParser(linearFunc, subjectTo, colonSp.split[1]); // 将公式处理成结构体
            } else {
                printf("Objective function invalid.\n");
                status = 0;
            }
            freeSplitArr(&colonSp); // 用完后释放
            break;
        case 2: // 写入ST

            break;
        case 3: // 写入常数项
            constants[constantsNum] = (char *) calloc(stringLen + 1, sizeof(char));
            strcpy(constants[constantsNum], str); // 将常量(字符串表示)放入常量数组
            constantsNum++;
            if(constantsNum>=constArrLen){ // 数组不够放了！需要分配更多
                constArrLen+=BUFFER_SIZE_PER_ALLOC;
                constants=(char**) realloc(constants,sizeof(char*)*constArrLen);
                if(constants!=NULL){ // 分配成功
                    // 初始化新分配部分的内存为0
                    memset(constants+constantsNum,0,sizeof(char*)*BUFFER_SIZE_PER_ALLOC);
                }else{
                    printf("Memory re-allocation failed when writing CONSTANTS.");
                    status=0;
                }
            }
            break;
    }
    return status;
}
