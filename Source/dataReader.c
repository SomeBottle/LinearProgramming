/* 读取并处理初始目标函数及约束的模块 --> 化标准型
 * 非常感谢文章：https://www.bilibili.com/read/cv5287905
 */
#include "public.h"

#define BUFFER_SIZE_PER_ALLOC 100 // 每次分配给字符串暂存区的元素个数
#define RESET_BUFFER (char *) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char)) // 字符串暂存区，最开始分配100个
#define ST_SIZE_PER_ALLOC 10 // 每次分配给约束SubjectTo的元素个数

char *constants = NULL; // 常数项指针数组
int constArrLen = BUFFER_SIZE_PER_ALLOC; // 常量项数组长度，防止溢出用
int constantsNum = 0; // 常数项数量

// 正在读取哪个部分，为1代表在读目标函数LF，为2代表在读约束ST，3则代表在读取常量CONSTANTS，分开处理。
static int readFlag = 0;

int InterruptBuffer(char x);

LPModel Parser(FILE *fp);

ST FormulaParser(char *str);

int WriteIn(LF *linearFunc, ST *subjectTo, int *stPtr, int *stSize, char *str);


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
    ST *subjectTo = (ST *) calloc(ST_SIZE_PER_ALLOC, sizeof(ST)); // 初始化约束结构体数组
    int stPtr = 0; // 约束数组指针
    int stSize = ST_SIZE_PER_ALLOC; // 约束数组总长度
    int errorOccurs = 0; // 是否发生错误
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    int bufferLen = BUFFER_SIZE_PER_ALLOC; // 字符串暂存区长度，防止溢出
    char *buffer = RESET_BUFFER; // 字符串暂存区，最开始分配100个
    if (constants == NULL) { // 全局变量在外层声明时无法被赋值，只能在这里赋值了
        constants = (char *) calloc(BUFFER_SIZE_PER_ALLOC, sizeof(char));
    }
    while (!feof(fp)) {
        currentChar = (char) fgetc(fp);
        if (!InterruptBuffer(currentChar)) { // 字符不是空白符，推入buffer
            if (isspace(currentChar)) continue; // 跳过{ }中所有空格
            buffer[bufferPointer] = currentChar; // 推入buffer
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
        } else if (strlen(buffer) > 0) { // 遇到空白字符或大括号或分号, 暂存区中有内容就进行处理
            if (strcmp(buffer, "LF") == 0) {
                readFlag = 1; // 正在读取目标函数LF
            } else if (strcmp(buffer, "ST") == 0) {
                readFlag = 2; // 正在读取约束ST
            } else if (strcmp(buffer, "CONSTANTS") == 0) {
                readFlag = 3; // 正在读取常量列表
            } else if (readFlag != 0) { // 交给对应的函数将数据读入结构体
                int writeResult = WriteIn(&linearFunc, subjectTo, &stPtr, &stSize, buffer);
                if (!writeResult) {
                    errorOccurs = 1;
                    break; // 解析数据失败，中止
                }
            }
            // printf("len:%d, Str: %s\n", strlen(buffer), buffer);
            free(buffer); // 释放内存块，抛弃当前暂存区
            bufferPointer = 0; // 初始化暂存区指针
            bufferLen = BUFFER_SIZE_PER_ALLOC; // 初始化
            buffer = RESET_BUFFER; // 重设字符串暂存区
            if (currentChar == '}') // 遇到反大括号，当前部分读取完毕
                readFlag = 0; // 读取完毕
        }
    }
    free(buffer); // 释放暂存区
    buffer = NULL;
    if (errorOccurs) { // 发生了错误
        printf("WARNING: Error occurred when parsing the model.\n");
    }
    LPModel result = {
            .subjectTo=subjectTo,
            .objective=linearFunc,
            .stNum=stPtr
    };
    return result;
}

ST FormulaParser(char *str) { // 将方程字符串处理为对应结构体，返回结果是ST，记得free
    int i, len = strlen(str);
    char currentChar;
    int writeSide = 0; // 在写入哪边，0代表关系符号左边，1代表右边
    int cfcRead = 0; // 读取过系数的标记
    Monomial monoBuffer = {}; // 单项的暂存区
    ST result = { // 返回结果
            .leftNum=0,
            .rightNum=0,
            .left=(Monomial *) calloc(len, sizeof(Monomial)),
            .right=(Monomial *) calloc(len, sizeof(Monomial))
    };
    int bufferPointer = 0; // 字符串暂存区指针
    char *buffer = (char *) calloc(len, sizeof(char)); // 字符串暂存区
    for (i = 0; i < len + 1; i++) {
        currentChar = i < len ? str[i] : '+'; // 最后len+1特殊处理，保证所有项目都被读入
        if (strchr("+->=<", currentChar) != NULL) { // 是加号或减号或>=<，这是每一项的划分标志
            if (bufferPointer > 0 || monoBuffer.constant) {
                // 暂存区中有内容，这一段部分就是变量名，此时读取完毕了一项 / 或者有常量项
                if (bufferPointer > 0 && strspn(buffer, "0123456789+-./") == strlen(buffer)) {
                    // 目前的暂存区中是一个常数项（前提：暂存区中有内容）
                    monoBuffer.variable[0] = '\0'; // 该项没有变量名
                    monoBuffer.coefficient = Fractionize(buffer); // 存入系数
                } else {
                    strncpy(monoBuffer.variable, buffer, 2); // 变量名最多两个字符
                    monoBuffer.variable[2] = '\0'; // 手动构造字符串
                }
                cfcRead = 0; // 一项系数读取完毕，标记归位
                if (writeSide == 0) { // 写到左边
                    result.left[result.leftNum++] = monoBuffer; // 把一项存入数组，作为式子左端
                } else if (writeSide == 1) {
                    result.right[result.rightNum++] = monoBuffer; // 作为式子右端
                }
                memset(buffer, 0, sizeof(char) * bufferPointer); // 读取后清空buffer
                bufferPointer = 0; // 重置字符串缓冲区
                Monomial newMono = {};
                monoBuffer = newMono; // 重置单项缓冲区
            }
            if (currentChar == '+' || currentChar == '-') {
                buffer[bufferPointer++] = currentChar; // 储存+-符号
            } else if (currentChar == '<' || currentChar == '>') { // 是关系符号>或<，这是方程左右的划分标志
                int ptr = 0;
                char nextChar = str[i + 1]; // 查看下一项是不是还有符号
                result.relation[ptr++] = currentChar; // 记入符号
                if (nextChar == '=') { // 下一项是等号，那就是>=或<=
                    result.relation[ptr++] = nextChar; // 计入符号
                    i++; // 跳过下一项目
                }
                result.relation[ptr] = '\0'; // 手动构造成字符串
                writeSide = 1; // 左边读完了，开始读右边
            } else if (currentChar == '=') { // 是关系符号=，这是方程左右的划分标志
                result.relation[0] = currentChar; // 存入符号
                result.relation[1] = '\0'; // 构造成字符串
                writeSide = 1; // 读右边
            }
        } else {
            if (!cfcRead && !isdigit(currentChar) && currentChar != '.' && currentChar != '/') {
                // 如果不是数字（包括分数除号，小数点，整数数字digit），说明系数读取结束，清除一次buffer
                // 此前的部分作为系数中的数字项存入monoBuffer，如果buffer中没有字符串，也就是没有写系数，那就默认是1
                monoBuffer.coefficient = bufferPointer > 0 ? Fractionize(buffer) : Fractionize("1");
                if (strchr(constants, currentChar) != NULL) { // 当前字符属于常量
                    monoBuffer.constant = currentChar; // 把常量作为系数的一部分储存
                    currentChar = 0; // 字符使用后置0
                } else {
                    monoBuffer.constant = 0; // 没有常量就设为0
                }
                memset(buffer, 0, sizeof(char) * bufferPointer); // 读取后清空buffer
                bufferPointer = 0;
                cfcRead = 1;
            }
            if (currentChar) {
                buffer[bufferPointer] = currentChar; // 逐字符处理
                bufferPointer++; // 暂存区指针后移
            }
        }
    }
    free(buffer); // 释放暂存区
    return result;
}

int WriteIn(LF *linearFunc, ST *subjectTo, int *stPtr, int *stSize, char *str) { // 将数据(str)解析后写入LF或者ST
    int status = 1; // 返回码
    int stringLen = strlen(str);
    SplitResult colonSp; // 初始化分割字符串
    ST formulaResult;
    switch (readFlag) {
        case 1: // 写入LF
            // 根据冒号分割
            colonSp = SplitByChr(str, ':');
            if (colonSp.len < 2) { // 目标函数没有指定maximize or minimize，无效
                printf("Objective function invalid.\n");
                status = 0;
            } else if (strcmp(colonSp.split[0], "max") == 0 || strcmp(colonSp.split[0], "min") == 0) { // 必须要是max/min
                strcpy(linearFunc->type, colonSp.split[0]); // 写入max/min
                formulaResult = FormulaParser(colonSp.split[1]); // 将公式处理成结构体
                if (strcmp(formulaResult.relation, "=") == 0) {
                    linearFunc->left = formulaResult.left;
                    linearFunc->right = formulaResult.right;
                    linearFunc->leftNum = formulaResult.leftNum;
                    linearFunc->rightNum = formulaResult.rightNum; // 结果存入linearFunc
                    printf("Successfully parsed the Objective function.\n");
                } else {
                    printf("Wrong relational operator in Objective function!\n");
                    status = 0;
                }
            } else {
                printf("Objective function invalid.\n");
                status = 0;
            }
            freeSplitArr(&colonSp); // 用完后释放
            break;
        case 2: // 写入ST
            formulaResult = FormulaParser(str); // 解析约束
            subjectTo[(*stPtr)++] = formulaResult;
            if (*stPtr >= *stSize) { // 约束结构体数组放不下了，需要重分配
                (*stSize) += ST_SIZE_PER_ALLOC; // 扩充内存大小
                subjectTo = (ST *) realloc(subjectTo, (*stSize) * sizeof(ST));
                if (subjectTo != NULL) {
                    memset(subjectTo + *stPtr, 0, ST_SIZE_PER_ALLOC * sizeof(ST));
                } else { // 内存分配失败
                    status = 0;
                    printf("Memory re-allocation failed when parsing constraints.\n");
                }
            }
            break;
        case 3: // 写入常量项
            if (!isalpha(str[0])) { // 常量只能是a-zA-Z的一个字母
                printf("Only letters: [a-zA-Z] can be used in CONSTANTS.\n");
                status = 0;
                break;
            }
            constants[constantsNum] = str[0]; // 将常量(字符表示)放入常量数组
            constantsNum++;
            if (constantsNum >= constArrLen) { // 数组不够放了！需要分配更多
                constArrLen += BUFFER_SIZE_PER_ALLOC;
                constants = (char *) realloc(constants, sizeof(char) * constArrLen);
                if (constants != NULL) { // 分配成功
                    // 初始化新分配部分的内存为0
                    memset(constants + constantsNum, 0, sizeof(char) * BUFFER_SIZE_PER_ALLOC);
                } else {
                    printf("Memory re-allocation failed when writing CONSTANTS.");
                    status = 0;
                }
            }
            break;
    }
    return status;
}
