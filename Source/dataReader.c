/* 读取并化简处理初始目标函数及约束的模块
 * SomeBottle
 */
#include "public.h"

Constant *constants = NULL; // 常数项指针数组
int constArrLen = CONSTANTS_SIZE_PER_ALLOC; // 常量项数组长度，防止溢出用
int constantsNum = 0; // 常数项数量

// 正在读取哪个部分，为1代表在读目标函数OF，为2代表在读约束ST，3则代表在读取常量CONSTANTS，分开处理。
static int readFlag = 0;

int InterruptBuffer(char x);

ST FormulaParser(char *str, int *valid);

ST FormulaSimplify(ST formula, int *valid);

int WriteIn(OF *linearFunc, ST **subjectTo, int *stPtr, int *stSize, char *str);

/**
 * 判断是否停止读入当前buffer字符串
 * @param x 当前读到的字符(char)
 * @return 1/0 代表 是/否 停止读入当前buffer字符串
 */
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

/**
 * 对LP模型中的约束进行移项处理，保证关系符号左边都是变量，而右边是常数项；
 * 同时合并多项式中的同类项；
 * 另外再次检查LP模型是否有效
 * @param model 指向LP模型的指针
 */
void LPTrans(LPModel *model) {
    int i, j, k;
    ST *stTemp = NULL;
    Term *termTemp = NULL;

    // 合并目标函数右边的同类项
    if (!CmbSmlTerms(model->objective.right, &model->objective.rightLen, 1)) {
        // 出错
        printf("ERROR: Invalid term appeared in the right hand side of Objective Function.\n");
        model->valid = 0; // 无效
    }

    for (i = 0; i < model->stLen; i++) {
        stTemp = model->subjectTo + i;
        for (j = 0; j < stTemp->leftLen; j++) { // 检查约束左边有没有常数项
            termTemp = stTemp->left[j];
            if (strlen(termTemp->variable) == 0) { // 没有变量名，是常数项，往右边移动
                // 移除式子左边的对应项目
                stTemp->leftLen = RmvTerm(stTemp->left, stTemp->leftLen, j, 0);
                termTemp->coefficient = NInv(termTemp->coefficient); // 移项后取相反数
                stTemp->right[stTemp->rightLen++] = termTemp; // 把该项移动到式子右边
            }
        }
        for (j = 0; j < stTemp->rightLen; j++) {
            termTemp = stTemp->right[j];
            if (strlen(termTemp->variable) != 0) { // 有变量名，非常数项，往左边移动
                stTemp->rightLen = RmvTerm(stTemp->right, stTemp->rightLen, j, 0);
                termTemp->coefficient = NInv(termTemp->coefficient); // 取相反数
                stTemp->left[stTemp->leftLen++] = termTemp; // 移动到式子左边
            }
        }
        if (stTemp->leftLen > 0 && stTemp->rightLen > 0) { // 式子左右两侧必须要有项目，保证约束完整性
            // 合并左边同类项：全都是带变量的
            if (!CmbSmlTerms(stTemp->left, &stTemp->leftLen, 0)) {
                // 出错
                printf("ERROR: Invalid term appeared in the left hand side of the CONSTRAINT (ST Line: %d)\n",
                       i + 1);
                model->valid = 0; // 无效
            }
            // 合并右边同类项：全都是常数项
            for (j = stTemp->rightLen - 1; j > 0; j--) { // 倒序遍历
                stTemp->right[0]->coefficient = NAdd(stTemp->right[0]->coefficient, stTemp->right[j]->coefficient);
                free(stTemp->right[j]); // 释放参与合并了的项
                stTemp->rightLen--;
            }
            // 检查约束左边在合并同类项后是否还有项目
            if (stTemp->leftLen <= 0) {
                printf("ERROR: No term left in the left hand side of the CONSTRAINT (ST Line: %d) after combining similar terms.\n",
                       i + 1);
                model->valid = 0; // 无效
            }
            // 检查约束右边的常数项是否有效(valid)
            if (!stTemp->right[0]->coefficient.valid) {
                // 运算错误时(比如分母出现0)，数字就会无效
                printf("ERROR: Division by zero appeared in the right hand side of the CONSTRAINT (ST Line: %d).\n",
                       i + 1);
                model->valid = 0;
            }

            // 抽取出变量的取值方向，仅限 x >= 0 x <= 0 这种，否则会留在约束条件中
            if (stTemp->leftLen == 1 && stTemp->rightLen == 1 && // 左右只有一项
                Decimalize(stTemp->left[0]->coefficient) == 1 && // 左边系数必须是1
                Decimalize(stTemp->right[0]->coefficient) == 0 && // 右边为0
                (abs(stTemp->relation) == 2)) { // 关系必须是 >= 或 <=
                int ptr;
                VarItem *newItem = CreateVarItem(stTemp->left[0]->variable, stTemp->relation, 0);
                PutVarItem(newItem); // 将变量取值存入哈希表
                for (ptr = i; ptr < model->stLen - 1; ptr++) { // 移除指定的约束
                    model->subjectTo[ptr] = model->subjectTo[ptr + 1];
                }
                // 更新约束数量
                model->stLen--;
                i--; // 约束移除后遍历位置前移
            }
        } else { // 有约束不完整
            printf("ERROR: LPModel invalid due to the incomplete CONSTRAINT (ST Line: %d).\n", i + 1);
            model->valid = 0;
        }
    }
    // 最后检查目标函数中的决策变量是否都正好存在于约束中，不能多也不能少
    size_t tableVarNum = 0;
    free(GetVarItems(&tableVarNum)); // 获得哈希表中变量的数量
    // 因为在合并约束的同类项时会将变量写入哈希表，
    // 如果 哈希表存放的变量数量 和 合并后的目标函数右边变量的数量 不匹配就肯定少写了或多写了约束
    if (tableVarNum != model->objective.rightLen) {
        printf("ERROR: Mismatch in the number of variables in the Objective Function and Constraints.\n");
        model->valid = 0;
    }
}

/**
 * 读取文件中的原始数据并转换为线性规划模型
 * @param fp 文件指针
 * @return 解析生成的线性规划模型
 * @note 记得使用FreeModel进行释放
 */
LPModel Parser(FILE *fp) { // 传入读取文件操作指针用于读取文件
    OF linearFunc = {
            .left=NULL,
            .right=NULL,
            .leftLen=0,
            .rightLen=0
    }; // 初始化目标函数结构体
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
                readFlag = 1; // 正在读取目标函数OF
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
        }
        if (currentChar == '}') { // 遇到反大括号，当前部分读取完毕
            readFlag = 0; // 读取完毕
        }
    }
    free(buffer); // 释放暂存区
    buffer = NULL;
    if (linearFunc.leftLen == 0) { // 没有读到目标函数
        valid = 0;
        printf("MISSING DATA: Objective Function not found.\n");
    }
    if (stPtr == 0) {
        valid = 0;
        printf("MISSING DATA: Constraints not found.\n");
    }
    if (!valid) { // 发生了错误
        printf("Error occurred when parsing the model.\n");
    }
    LPModel result = {
            .subjectTo=subjectTo,
            .objective=linearFunc,
            .stLen=stPtr,
            .valid=valid
    };
    return result;
}

/**
 * 将方程字符串解析为一个约束方程结构体(ST)
 * @param str 待处理字符串
 * @param valid 指向一个变量，用于储存 1/0 代表当前字符串 是/否 解析成功
 * @return 一个约束方程结构体ST
 * @note 通常这个和WriteIn函数结合使用，单独使用时请记得free
 */
ST FormulaParser(char *str, int *valid) {
    int i, len = strlen(str);
    char currentChar, nextChar;
    int writeSide = 0; // 在写入哪边，0 代表关系符号左边，1 代表右边
    int cfcRead = 0; // 读取过系数的标记
    Term *termBuffer = (Term *) calloc(1, sizeof(Term)); // 单项的暂存区
    ST result = { // 返回结果
            .leftLen=0,
            .rightLen=0,
            .left=(Term **) calloc(len, sizeof(Term *)),
            .right=(Term **) calloc(len, sizeof(Term *))
    };
    short int thanMark; // 小于号thanMark=-1，大于号thanMark=1
    int bufferPointer = 0; // 字符串暂存区指针
    char *buffer = (char *) calloc(len, sizeof(char)); // 字符串暂存区
    for (i = 0; i < len + 1; i++) {
        currentChar = i < len ? str[i] : '+'; // 最后len+1特殊处理，保证所有项目都被读入
        if (strchr("+->=<", currentChar) != NULL) { // 是加号或减号或>=<，这是每一项的划分标志
            if (bufferPointer > 0 || termBuffer->coefficient.valid) {
                // 暂存区中有内容，这一段部分就是变量名，此时读取完毕了一项，适用于 3M x1这种情况
                // 或者有纯常数项(termBuffer.coefficient)，适用于3M这种只有常数项的情况
                if (bufferPointer == 0 && termBuffer->coefficient.valid) {
                    // 目前的暂存区中是一个常数项（前提：暂存区中没有内容）
                    termBuffer->variable[0] = '\0'; // 该项没有变量名
                } else if (IsConstTerm(buffer)) { // buffer中储存的是一个常数项，针对-1.35这种情况
                    termBuffer->coefficient = Fractionize(buffer); // 转换为分数存入系数
                    termBuffer->variable[0] = '\0'; // 该项没有变量名
                } else {
                    strncpy(termBuffer->variable, buffer, 3); // 变量名最多3个字符
                    termBuffer->variable[3] = '\0'; // 手动构造字符串
                }
                cfcRead = 0; // 一项系数读取完毕，标记归位
                if (writeSide == 0) { // 写到左边
                    result.left[result.leftLen++] = termBuffer; // 把一项存入数组，作为式子左端
                } else if (writeSide == 1) {
                    result.right[result.rightLen++] = termBuffer; // 作为式子右端
                }
                memset(buffer, 0, sizeof(char) * bufferPointer); // 读取后清空buffer
                bufferPointer = 0; // 重置字符串缓冲区
                termBuffer = calloc(1, sizeof(Term)); // 重置单项缓冲区
            }
            if (currentChar == '+' || currentChar == '-') {
                buffer[bufferPointer++] = currentChar; // 储存+-符号
            } else if (currentChar == '<' && (thanMark = -1) ||
                       currentChar == '>' && (thanMark = 1)) { // 是关系符号>或<，这是方程左右的划分标志
                nextChar = str[i + 1]; // 查看下一项是不是还有符号
                if (nextChar == '=') { // 下一项是等号，那就是>=或<=
                    thanMark *= 2; // 小于号-1*2=小于等于号-2; 大于号1*2=大于等于号2
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
                // 此前的部分作为系数中的数字项存入termBuffer，如果buffer中没有字符串，也就是没有写系数，那就默认是1
                termBuffer->coefficient = bufferPointer > 0 ? Fractionize(buffer) : Fractionize("1");
                memset(buffer, 0, sizeof(char) * bufferPointer); // 读取后清空buffer
                bufferPointer = 0;
                cfcRead = 1; // 系数读取完毕
            }
            buffer[bufferPointer] = currentChar; // 逐字符处理
            bufferPointer++; // 暂存区指针后移
        }
    }
    free(buffer); // 释放暂存区
    free(termBuffer);
    // Formula校验部分
    result = FormulaSimplify(result, valid);
    return result;
}

/**
 * 对约束方程(ST)进行校验和化简处理
 * @param formula 待处理约束方程(ST结构体)
 * @param valid 指向一个变量，用于储存 1/0 代表当前方程 是/否 处理成功
 * @return 处理后的约束方程结构体ST
 * @note 这个函数主要的工作是化简，找出式子两边的最大公约数
 */
ST FormulaSimplify(ST formula, int *valid) {
    // 校验，化简处理
    if ((formula.leftLen < 1 || formula.rightLen < 1) || // 左边和右边都至少要有一项
        !formula.relation) // 缺少关系符号，方程无效
    {
        printf("Simplification Failed: Formula invalid.\n");
        *valid = 0; // 该方程无效
    } else {
        int i;
        // 临时把左右两边连接起来，便于遍历
        Term **joined = (Term **) MemJoin(formula.left, formula.leftLen, formula.right, formula.rightLen,
                                          sizeof(Term *));
        // 连接后的数组长度
        unsigned int joinedLen = formula.leftLen + formula.rightLen;
        long int numeCommonDiv = joined[0]->coefficient.numerator; // 所有分子的最大公约数
        long int denoCommonDiv = joined[0]->coefficient.denominator; // 所有分母的最大公约数
        for (i = 1; i < joinedLen; i++) {
            // 找所有分子的最大公约数和所有分母的最大公约数
            if (joined[i]->coefficient.constant != NULL) { // 用户输入的方程不允许有常量
                printf("Simplification Failed: Manual added CONSTANTs are not allowed.\n");
                *valid = 0;
                break;
            }
            numeCommonDiv = GCD(numeCommonDiv, joined[i]->coefficient.numerator);
            denoCommonDiv = GCD(denoCommonDiv, joined[i]->coefficient.denominator);
        }
        for (i = 0; i < formula.leftLen; i++) { // 处理式左边
            formula.left[i]->coefficient.numerator /= numeCommonDiv; // 约分子
            formula.left[i]->coefficient.denominator /= denoCommonDiv; // 约分母
        }
        for (i = 0; i < formula.rightLen; i++) { // 处理式右边
            formula.right[i]->coefficient.numerator /= numeCommonDiv; // 约分子
            formula.right[i]->coefficient.denominator /= denoCommonDiv; // 约分母
        }
        free(joined); // 用完后释放内存是好习惯
    }
    return formula;
}

/**
 * 将字符串解析为方程后将其写入LPModel特定的部分
 * @param linearFunc 指向存放目标函数的变量的指针
 * @param subjectTo 指向 指向约束方程的数组的指针 的指针
 * @param stPtr 指向一个变量的指针，代表当前写入到约束方程数组中的哪个位置（下标）
 * @param stSize 指向一个变量的指针，代表当前约束方程数组的容量
 * @param str 待解析字符串
 * @return 1/0 代表 是/否 写入成功
 * @note 本函数根据readFlag决定是写入模型的目标函数还是约束方程，1代表写入目标函数，2代表写入约束方程
 */
int WriteIn(OF *linearFunc, ST **subjectTo, int *stPtr, int *stSize, char *str) {
    int status = 1; // 返回码
    SplitResult colonSp; // 初始化分割字符串
    ST formulaResult;
    switch (readFlag) {
        case 1: // 写入OF
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
                    if (formulaResult.leftLen == 1 && // OF左边只能有z一项
                        Decimalize(formulaResult.left[0]->coefficient) == 1) { // 左边z系数必须为1
                        linearFunc->left = formulaResult.left;
                        linearFunc->right = formulaResult.right;
                        linearFunc->leftLen = formulaResult.leftLen;
                        linearFunc->rightLen = formulaResult.rightLen; // 结果存入linearFunc
                    } else {
                        printf("Non-standard Objective function!\n");
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
