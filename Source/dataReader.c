/* 读取并化简处理初始目标函数及约束的模块
 * SomeBottle
 */
#include "public.h"

Constant *constants = NULL; // 常数项指针数组
int constArrLen = CONSTANTS_SIZE_PER_ALLOC; // 常量项数组长度，防止溢出用
int constantsNum = 0; // 常数项数量

// 正在读取哪个部分，为1代表在读目标函数LF，为2代表在读约束ST，3则代表在读取常量CONSTANTS，分开处理。
static int readFlag = 0;

int InterruptBuffer(char x);

LPModel Parser(FILE *fp);

ST FormulaParser(char *str, int *valid);

ST FormulaSimplify(ST formula, int *valid);

int WriteIn(OF *linearFunc, ST **subjectTo, int *stPtr, int *stSize, char *str);


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
    OF linearFunc; // 初始化目标函数结构体
    ST *subjectTo = (ST *) calloc(ST_SIZE_PER_ALLOC, sizeof(ST)); // 初始化约束结构体数组
    int stPtr = 0; // 约束数组指针
    int stSize = ST_SIZE_PER_ALLOC; // 约束数组总长度
    int valid = 1; // 模型是否有效
    char currentChar;
    int bufferPointer = 0; // 字符串暂存区指针
    int bufferLen = BUFFER_SIZE_PER_ALLOC; // 字符串暂存区长度，防止溢出
    char *buffer = RESET_BUFFER; // 字符串暂存区，最开始分配100个
    if (constants == NULL) { // 全局变量在外层声明时无法被赋值，只能在这里赋值了
        constants = (Constant *) calloc(CONSTANTS_SIZE_PER_ALLOC, sizeof(Constant));
        Constant cstTemp = {
                .relation=4,
                .name='M',
                .val=Fractionize("0") // 大M法专用M>0
        };
        constants[constantsNum++] = cstTemp; // 默认创造一个常量大M
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
                    valid = 0;
                    break; // 内存重分配失败，退出
                }
            }
        } else if (strlen(buffer) > 0) { // 遇到空白字符或大括号或分号, 暂存区中有内容就进行处理
            if (strcmp(buffer, "OF") == 0) {
                readFlag = 1; // 正在读取目标函数LF
            } else if (strcmp(buffer, "ST") == 0) {
                readFlag = 2; // 正在读取约束ST
            } else if (readFlag != 0) { // 交给对应的函数将数据读入结构体
                int writeResult = WriteIn(&linearFunc, &subjectTo, &stPtr, &stSize, buffer);
                if (!writeResult) {
                    valid = 0;
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
    if (!valid) { // 发生了错误
        printf("WARNING: Error occurred when parsing the model.\n");
    }
    LPModel result = {
            .subjectTo=subjectTo,
            .objective=linearFunc,
            .stNum=stPtr,
            .valid=valid
    };
    return result;
}

ST FormulaParser(char *str, int *valid) { // 将方程字符串处理为对应结构体，返回结果是ST，记得free
    int i, len = strlen(str);
    char currentChar, nextChar;
    int writeSide = 0; // 在写入哪边，0 代表关系符号左边，1 代表右边
    int cfcRead = 0; // 读取过系数的标记
    Monomial monoBuffer = {}; // 单项的暂存区
    ST result = { // 返回结果
            .leftNum=0,
            .rightNum=0,
            .left=(Monomial *) calloc(len, sizeof(Monomial)),
            .right=(Monomial *) calloc(len, sizeof(Monomial))
    };
    short int thanMark; // 小于号thanMark=-1，大于号thanMark=1
    int bufferPointer = 0; // 字符串暂存区指针
    char *buffer = (char *) calloc(len, sizeof(char)); // 字符串暂存区
    for (i = 0; i < len + 1; i++) {
        currentChar = i < len ? str[i] : '+'; // 最后len+1特殊处理，保证所有项目都被读入
        if (strchr("+->=<", currentChar) != NULL) { // 是加号或减号或>=<，这是每一项的划分标志
            if (bufferPointer > 0 || monoBuffer.coefficient.valid) {
                // 暂存区中有内容，这一段部分就是变量名，此时读取完毕了一项，适用于 3M x1这种情况
                // 或者有纯常数项(monoBuffer.coefficient)，适用于3M这种只有常数项的情况
                if (bufferPointer == 0 && monoBuffer.coefficient.valid) {
                    // 目前的暂存区中是一个常数项（前提：暂存区中没有内容）
                    monoBuffer.variable[0] = '\0'; // 该项没有变量名
                } else if (IsConstItem(buffer)) { // buffer中储存的是一个常数项，针对-1.35这种情况
                    monoBuffer.coefficient = Fractionize(buffer); // 转换为分数存入系数
                    monoBuffer.variable[0] = '\0'; // 该项没有变量名
                } else {
                    strncpy(monoBuffer.variable, buffer, 3); // 变量名最多3个字符
                    monoBuffer.variable[3] = '\0'; // 手动构造字符串
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
            } else if (currentChar == '<' && (thanMark = -1) ||
                       currentChar == '>' && (thanMark = 1)) { // 是关系符号>或<，这是方程左右的划分标志
                int ptr = 0;
                nextChar = str[i + 1]; // 查看下一项是不是还有符号
                if (nextChar == '=') { // 下一项是等号，那就是>=或<=
                    thanMark *= 2; // 小于号-1*2=小于等于号-2; 大于号1*2=大于等于号2
                    nextChar = '\0';
                    i++; // 跳过下一个项目
                }
                result.relation = thanMark; // 记入符号
                writeSide = 1; // 左边读完了，开始读右边
            } else if (currentChar == '=') { // 是关系符号=，这是方程左右的划分标志
                result.relation = 3; // 存入符号
                writeSide = 1; // 读右边
            }
        } else {
            if (!cfcRead && !isdigit(currentChar)
                && currentChar != '.' && currentChar != '/') {
                /* 前面几个表达式判断如果正在读取系数(cfcRead=0)，不是数字部分（包括分数除号，小数点，整数数字digit），
                 说明系数读取结束(cfcRead=1)，计入系数，清除一次buffer*/
                // 此前的部分作为系数中的数字项存入monoBuffer，如果buffer中没有字符串，也就是没有写系数，那就默认是1
                monoBuffer.coefficient = bufferPointer > 0 ? Fractionize(buffer) : Fractionize("1");
                memset(buffer, 0, sizeof(char) * bufferPointer); // 读取后清空buffer
                bufferPointer = 0;
                cfcRead = 1; // 系数读取完毕
            }
            buffer[bufferPointer] = currentChar; // 逐字符处理
            bufferPointer++; // 暂存区指针后移
        }
    }
    free(buffer); // 释放暂存区
    // Formula校验部分
    result = FormulaSimplify(result, valid);
    return result;
}

ST FormulaSimplify(ST formula, int *valid) {
    // 校验，化简处理
    if ((formula.leftNum < 1 || formula.rightNum < 1) || // 左边和右边都至少要有一项
        !formula.relation) // 缺少关系符号，方程无效
    {
        printf("Simplification Failed: Formula invalid.\n");
        *valid = 0; // 该方程无效
    } else {
        int i;
        Number numTemp; // 临时存放数字
        // 临时把左右两边连接起来，便于遍历
        Monomial *joined = MemJoin(formula.left, formula.leftNum, formula.right, formula.rightNum, sizeof(Monomial));
        // 连接后的数组长度
        unsigned int joinedLen = formula.leftNum + formula.rightNum;
        long int numeCommonDiv = joined[0].coefficient.numerator; // 所有分子的最大公约数
        long int denoCommonDiv = joined[0].coefficient.denominator; // 所有分母的最大公约数
        for (i = 1; i < joinedLen; i++) {
            // 找所有分子的最大公约数和所有分母的最大公约数
            if (joined[i].coefficient.constant != NULL) { // 用户输入的方程不允许有常量
                printf("Simplification Failed: Manual added CONSTANTs are not allowed.\n");
                *valid = 0;
                break;
            }
            numeCommonDiv = GCD(numeCommonDiv, joined[i].coefficient.numerator);
            denoCommonDiv = GCD(denoCommonDiv, joined[i].coefficient.denominator);
        }
        for (i = 0; i < formula.leftNum; i++) { // 处理式左边
            formula.left[i].coefficient.numerator /= numeCommonDiv; // 约分子
            formula.left[i].coefficient.denominator /= denoCommonDiv; // 约分母
        }
        for (i = 0; i < formula.rightNum; i++) { // 处理式右边
            formula.right[i].coefficient.numerator /= numeCommonDiv; // 约分子
            formula.right[i].coefficient.denominator /= denoCommonDiv; // 约分母
        }
        free(joined); // 用完后释放内存是好习惯
    }
    return formula;
}

int WriteIn(OF *linearFunc, ST **subjectTo, int *stPtr, int *stSize, char *str) { // 将数据(str)解析后写入LF或者ST
    int status = 1; // 返回码
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
                if (strcmp(colonSp.split[0], "max") == 0) {
                    linearFunc->type = 1; // 1 代表max
                } else if (strcmp(colonSp.split[0], "min") == 0) {
                    linearFunc->type = -1; // -1 代表min
                }
                formulaResult = FormulaParser(colonSp.split[1], &status); // 将公式处理成结构体
                if (formulaResult.relation == 3) {
                    if (formulaResult.leftNum == 1 && // OF左边只能有z一项
                        Decimalize(formulaResult.left[0].coefficient) == 1) { // 左边z系数必须为1
                        linearFunc->left = formulaResult.left;
                        linearFunc->right = formulaResult.right;
                        linearFunc->leftNum = formulaResult.leftNum;
                        linearFunc->rightNum = formulaResult.rightNum; // 结果存入linearFunc
                    } else {
                        linearFunc->left = NULL;
                        linearFunc->right = NULL;
                        linearFunc->leftNum = 0;
                        linearFunc->rightNum = 0;
                        printf("Nonstandard Objective function!\n");
                        status = 0;
                    }
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
            formulaResult = FormulaParser(str, &status); // 解析约束
            (*subjectTo)[(*stPtr)++] = formulaResult;
            if (*stPtr >= *stSize) { // 约束结构体数组放不下了，需要重分配
                (*stSize) += ST_SIZE_PER_ALLOC; // 扩充内存大小
                *subjectTo = (ST *) realloc(*subjectTo, (*stSize) * sizeof(ST));
                if (*subjectTo != NULL) {
                    memset(*subjectTo + *stPtr, 0, ST_SIZE_PER_ALLOC * sizeof(ST));
                } else { // 内存分配失败
                    status = 0;
                    printf("Memory re-allocation failed when parsing constraints.\n");
                }
            }
            break;
    }
    return status;
}
