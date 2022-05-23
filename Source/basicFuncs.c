#include "public.h"

static void PrintTerms(Term **item, size_t itemNum);

static void PrintRelation(short int code);

/**
 * 按字符分隔字符串
 * @param str 待分割字符串
 * @param chr 用于分割的字符
 * @return SplitResult结构体，包含分割结果数组和分割结果元素个数
 * @note 使用完结果后一定要记得用freeSplitArr函数进行释放
 */
SplitResult SplitByChr(char *str, char chr) {
    size_t i;
    size_t stringLen = strlen(str);
    size_t bufferSize = sizeof(char) * stringLen;
    char *buffer = (char *) malloc(bufferSize); // 字符串暂存区
    int bufferLen = 0; // 暂存区字符数组长度
    char **arr = (char **) malloc(sizeof(char *) * stringLen); // 数组第一维
    int arrLen = 0; // 返回二维数组第一维的大小
    for (i = 0; i < stringLen + 1; i++) {
        size_t currentChr;
        int lastOne = 0; // 最后一项单独处理
        if (i < stringLen) {
            currentChr = (size_t) str[i];
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
            buffer[bufferLen++] = (char) currentChr; // 存入字符串暂存区
        }
    }
    free(buffer); // 释放暂存区
    SplitResult result = {
            arr, // 一定要记得用完释放！！！
            arrLen
    };
    return result; // 返回结果
}

/**
 * 门当对户地释放SplitByChr的返回结果中的堆内存
 * @param rs 指向SplitByChr返回的结构体的指针
 */
void freeSplitArr(SplitResult *rs) {
    size_t i;
    for (i = 0; i < rs->len; i++) {
        free(rs->split[i]);
    }
    free(rs->split);
    rs->split = NULL;
}

/**
 * 将两段内存连接成一块（返回结果从堆中新分配）
 * @param prev 指向前一段内存起址的指针
 * @param prevLen 前一段内存的长度
 * @param next 指向后一段内存起址的指针
 * @param nextLen 后一段内存的长度
 * @param eachSize 每一个元素占字节数
 * @return 返回指向新分配的连接后内存开头的void指针
 */
void *MemJoin(void *prev, size_t prevLen, void *next, size_t nextLen, size_t eachSize) {
    //
    // (前一段内存的起址,前一段内存长度,后一段内存的起址,后一段内存长度,类型储存字节大小)
    void *joined = malloc(eachSize * (prevLen + nextLen));
    size_t prevSize = eachSize * prevLen;
    memcpy(joined, prev, prevSize); // 复制前一段
    memcpy(joined + prevSize, next, eachSize * nextLen); // 复制后一段
    // 返回void指针，记得free！
    return joined;
}

/**
 * 查找字符chr在常量数组内存中对应的地址，找不到返回NULL
 * @param chr 查找的常量（一个字符）
 * @return 一个地址
 */
Constant *InConstants(char chr) {
    Constant *ptr = NULL;
    if (constants != NULL) {
        size_t i;
        for (i = 0; i < constantsNum; i++) {
            if (constants[i].name == chr) {
                ptr = &constants[i];
            }
        }
    }
    return ptr;
}

/**
 * 判断整个字符串是不是一个常数项
 * @param str 待判断字符串
 * @return 1/0 代表 是/否 是常数项
 */
int IsConstTerm(char *str) {
    size_t i;
    unsigned long int len = strlen(str);
    for (i = 0; i < len; i++) {
        // 既不是数字，也不包含常量
        if (strchr("0123456789/+-.", str[i]) == NULL && InConstants(str[i]) == NULL) {
            return 0;
        }
    }
    return 1;
}

/**
 * 寻找两数的最大公约数(欧几里得算法)
 * @param num1 第一个数
 * @param num2 第二个数
 * @return 两个数的最大公约数（一定是正数）
 */
long int GCD(long int num1, long int num2) {
    long int temp;
    num1 = labs(num1); // GCD规定为正整数
    num2 = labs(num2);
    if (num2 > num1) {
        temp = num2;
        num2 = num1;
        num1 = temp;
    }
    while (num2) {
        temp = num2;
        num2 = num1 % num2;
        num1 = temp;
    }
    return num1;
}

/**
 * 寻找两数的最小公倍数
 * @param num1 第一个数
 * @param num2 第二个数
 * @return 两个数的最小公倍数，如果发生溢出就会返回-1
 */
long int LCM(long int num1, long int num2) {
    // 最大公约数*最小公倍数=两整数乘积
    // 溢出会返回-1
    long int divisor = GCD(num1, num2), divided, result;
    num1 = labs(num1); // 一般LCM也被限定为正整数
    num2 = labs(num2);
    divided = (num1 / divisor);
    result = divided * num2;
    if (divided != 0 && result / divided != num2) {
        return -1; // 如果溢出了就返回-1
    }
    return result;
}

/**
 * 将一个代表数字的字符串转化为分数结构体
 * @param str 待转化字符串
 * @return Number结构体
 */
Number Fractionize(char *str) { // 分数化一个字符串 3M/4 2.45M 5M 3/4M 3M/4M 3/4...
    size_t i;
    Number result = {.valid=1};
    Constant *cstPtr1 = NULL; // 常量临时指针1
    size_t len = strlen(str);
    size_t partLen = 0; // 字符串部分长度暂存
    // 创建一份字符串拷贝
    char *strCopy = (char *) calloc(len + 2, sizeof(char));
    char *convEndPtr; // 转换类型后所在位置的指针
    char *divPtr; // 分割用指针
    long int numerator;
    long int denominator;
    long int cmDivisor; // 最大公约数
    for (i = 0; i < len; i++) // 拷贝字符串
        strCopy[i] = str[i];
    if (strchr(strCopy, '/') != NULL) { // 分数表示
        Constant *cstPtr2 = NULL; // 常量临时指针2
        divPtr = strtok(strCopy, "/"); // 按'/‘分割
        if (divPtr != NULL) {
            partLen = strlen(divPtr);
            if ((cstPtr1 = InConstants(divPtr[partLen - 1])) != NULL) { // 分子最后一位是一个常量，对应3M/4分子3M的情况
                divPtr[partLen - 1] = '\0'; // 从字符串中去掉该项，防止下面转换为数字失败
            }
            // 分子转换为10进制long int类型
            numerator = strtol(divPtr, &convEndPtr, 10);
            if (*convEndPtr == '\0') { // 分子能完全转换为整数
                divPtr = strtok(NULL, "/"); // 继续再分割一次
                if (divPtr != NULL) {
                    partLen = strlen(divPtr);
                    if ((cstPtr2 = InConstants(divPtr[partLen - 1])) != NULL) { // 分母最后是一个常量
                        if (cstPtr1 != NULL) { // 分子已经有常量了
                            result.constant = NULL; // 两个常量消掉了
                        } else {
                            result.constant = cstPtr2; // 储存指向constants中一个元素的指针
                            result.constLies = 1; // 常量在分母
                        }
                        divPtr[partLen - 1] = '\0'; // 从字符串中去掉该项，防止下面转换为数字失败
                    } else if (cstPtr1 != NULL) { // 分子的最后存在常量
                        result.constant = cstPtr1; // 储存指向constants中一个元素的指针
                        result.constLies = 0; // 常量在分子
                    } else {
                        result.constant = NULL; // 无常量
                    }
                    cstPtr1 = cstPtr2 = NULL; // 解除两个指针的指向
                    denominator = strtol(divPtr, &convEndPtr, 10);
                    if (*convEndPtr == '\0' && numerator != 0) {
                        // 分母也能完全转换为整数，且分子不为0
                        cmDivisor = labs(GCD(numerator, denominator));
                        // 找出最大公约数（绝对值）
                        denominator = denominator / cmDivisor;
                        numerator = numerator / cmDivisor; // 约分操作
                        result.numerator = numerator;
                        result.denominator = denominator; // 存入结构体
                        if (denominator <= 0)  // 规定分母不可等于零，也不可小于0
                            result.valid = 0; // 数字无效
                    } else {
                        result.valid = 0; // 出错了，数字无效
                    }
                } else {
                    result.valid = 0; // 分母无效，数字无效
                }
            } else {
                result.valid = 0; // 出错了，数字无效
            }
        } else {
            result.valid = 0; // 分子无效，数字无效
        }
    } else {
        // 预先判断末尾有没有常量
        partLen = strlen(strCopy);
        if ((cstPtr1 = InConstants(strCopy[partLen - 1])) != NULL) { // 最后有一项常量
            result.constant = cstPtr1; // 存入常量指针
            result.constLies = 0; // 常量在分子上
            strCopy[--partLen] = '\0';
            // 从字符串中去掉该项，防止下面转换为数字失败(--partLen正好取到了字符串最后一位，也将partLen进行自减，缩短长度)
            cstPtr1 = NULL;
        } else {
            result.constant = NULL; // 无常量
        }
        if (strlen(strCopy) == 0 || (strlen(strCopy) == 1 && strchr("+-", strCopy[0]) != NULL)) {
            /* 存在这样一种情况: 在各种处理后到这里的字符串只剩一个+号或者一个-号了
             * 比如有一项是-x1，那么传进来的就只有一个负号；亦或是有一项是+M（M是常量），
             * 而上面处理常量后会把常量给移除掉，所以+M到这里也只会剩一个加号了
             * 因此在这里要做个额外处理，如果只有一个加号或减号，就在后面加个1，
             * 变成+1或者-1，这样下面的转换函数strtod就能正确处理
             * （甚者还有放在开头的常量，比如开头的M，被处理后字符串长度就为0了，这种情况也要考虑）
             */
            strCopy[partLen++] = '1';
            strCopy[partLen] = '\0';
        }
        if (strchr(strCopy, '.') != NULL) { // 表示为了小数
            double decimal = strtod(strCopy, &convEndPtr); // 先转换为double数
            if (*convEndPtr == '\0') { // 转换成功
                strtok(strCopy, "."); // 按小数点分割
                divPtr = strtok(NULL, "."); // 获得小数点后面的部分(NULL就会接着上一次的位置继续)
                /* 这里的原理就像这样：-2.45 -分数形式-> -245/100 -约分-> -49/20 */
                if (divPtr != NULL) {
                    size_t digitsLen = strlen(divPtr); // 小数位数
                    denominator = (long int) pow(10.0, (double) digitsLen); // 计算出分母
                    numerator = (long int) (decimal * denominator); // 计算出分子
                    cmDivisor = labs(GCD(numerator, denominator));
                    // 最大公约数约分，公约数规定为正数，防止符号问题
                    denominator = denominator / cmDivisor;
                    numerator = numerator / cmDivisor;
                    result.numerator = numerator;
                    result.denominator = denominator; // 存入结构体
                } else {
                    result.valid = 0;
                }
            } else {
                result.valid = 0;
            }
        } else { // 表示为了整数
            long int integer = strtol(strCopy, &convEndPtr, 10); // 转换为长整型
            if (*convEndPtr == '\0') { // 转换成功
                numerator = integer;
                denominator = 1; // 分母为1
                result.numerator = numerator;
                result.denominator = denominator; // 存入结构体
            } else {
                result.valid = 0;
            }
        }
    }
    free(strCopy); // 释放拷贝的字符串
    return result;
}

/**
 * 将Number结构体转换为double浮点数
 * @param num 待转换的Number结构体
 * @return 一个双精度浮点数
 */
double Decimalize(Number num) { // 将Number结构体转成double浮点数
    double result = 0;
    if (num.valid) {
        if (num.constant == NULL) {
            if (num.denominator) // 分母不能为0
                result = (double) num.numerator / num.denominator;
            else
                printf("Decimalize Failed: Numerator can't be divided by zero.");
        } else { // 不允许有常量
            printf("Decimalize Failed: Number with CONSTANT is not allowed here.");
        }
    } else { // 数字无效
        printf("Decimalize Failed: Number invalid.");
    }
    return result;
}

/**
 * 将整型数字转换为字符串
 * @param num 待转换数值(int)
 * @return 一个指向字符串开头的指针
 * @note 返回的字符串分配在堆中，记得free
 */
char *Int2Str(int num) {
    int original = num;
    size_t digits = 1; // 位数
    while ((num /= (10 * digits)) != 0)
        digits++;
    // 根据位数分配字符串长度
    char *str = (char *) calloc(digits + 1, sizeof(char));
    sprintf(str, "%d", original); // 数字打印进字符串
    return str;
}

/** 合并多项式中的同类项
 * @param terms 代表多项式的指针数组
 * @param len 指向指针数组长度变量的指针
 * @param forOF 是否用于合并目标函数的同类项
 * @return 返回合并是否成功
 */
int CmbSmlTerms(Term **terms, size_t *termsLen, int forOF) {
    size_t j, k;
    for (j = 0; j < *termsLen; j++) {
        for (k = j + 1; k < *termsLen; k++) {
            if (strcmp(terms[j]->variable, terms[k]->variable) == 0) { // 找到同类项
                // 系数相加
                terms[j]->coefficient = NAdd(terms[j]->coefficient, terms[k]->coefficient);
                *termsLen = RmvTerm(terms, *termsLen, k, 1); // 移除多余的项目
                k--; // 去除某一项后遍历的位置也要前移一位
            }
        }
        if (!terms[j]->coefficient.valid) { // 有项目的系数存在问题
            printf("CMB ERROR: Invalid coefficient appeared after combining.\n");
            return 0; // 合并中止
        }

        if (terms[j]->coefficient.numerator == 0) {
            // 剔除消除了的项（合并后系数为0）
            *termsLen = RmvTerm(terms, *termsLen, j, 1);
            j--; // 去除某一项后遍历的位置也要前移一位
        } else if (!forOF) { // 合并的是约束条件中的多项
            // 这一项有效，就把变量登记入哈希表
            // 创建键值对
            VarItem *newItem = CreateVarItem(terms[j]->variable, 0, 0);
            PutVarItem(newItem); // 存入哈希表（如果变量对应项目已经存在，就会被替换掉）
        }
    }
    return 1;
}

/**
 * 检查变量是否合规
 * @param str 待检查变量字符串
 * @return 1/0 代表 是/否 合规
 * @note 规定变量名第一位为字母，后面两位是数字
 */
short int ValidVar(char *str) {
    char digits[] = "0123456789";
    return (short int) (isalpha(str[0]) && strspn(str + 1, digits) == strlen(str + 1));
}

/**
 * 从多项式指针数组中移除某一项，比如从约束式子左边移去一项
 * @param terms 指向多项式指针数组的指针
 * @param len 多项式指针数组长度
 * @param pos 移除的位置（下标）
 * @param clean 是否free移除的堆项
 * @return 多项式指针数组的新长度
 * @note 如果clean=0，这里的Remove就只是形式上的，并没有对移除项进行free
 */
size_t RmvTerm(Term **terms, size_t len, size_t pos, short int clean) {
    int ptr1, ptr2 = 0; // 双指针法
    for (ptr1 = 0; ptr1 < len; ptr1++) {
        if (ptr1 != pos) {
            terms[ptr2++] = terms[ptr1];
        } else if (clean) {
            free(terms[ptr1]);
        }
    }
    return ptr2; // 数组的新长度
}

/**
 * 根据代号打印关系符号
 * @param code 关系代号
 * @note 代号 -2代表<= -1代表< 1代表> 2代表>= 3代表=
 */
void PrintRelation(short int code) {
    char symbols[][3] = {"<=", "<", "?", ">", ">=", "="};
    printf("%s", symbols[code + 2]);
}

/**
 * 打印多项式
 * @param item 指向多项式指针数组的指针
 * @param itemNum 多项式指针数组的元素数量
 * @return
 */
void PrintTerms(Term **item, size_t itemNum) { // 打印多项式
    size_t i;
    long int numeTemp, denoTemp, liesTemp;
    char constName = '\0';
    for (i = 0; i < itemNum; i++) {
        numeTemp = item[i]->coefficient.numerator;
        denoTemp = item[i]->coefficient.denominator;
        liesTemp = item[i]->coefficient.constLies;
        if (item[i]->coefficient.constant != NULL)
            constName = item[i]->coefficient.constant->name;
        // 先把分子打印出来
        if (i == 0) { // 是第一项
            printf("%ld", numeTemp);
        } else {
            printf(numeTemp >= 0 ? " + %ld" : " - %ld", labs(numeTemp));
        }
        // 如果常量在分子，打印一下
        if (constName != '\0' && liesTemp == 0)
            printf("%c", constName);
        // 有分母就打印分母
        if (denoTemp != 1) {
            printf("/%ld", denoTemp);
            if (constName != '\0' && liesTemp == 1)
                printf("%c", constName);
        }
        if (strlen(item[i]->variable) > 0) { // 有变量名的话
            printf("[%s", item[i]->variable); // 打印变量名
            if (item[i]->inverted) // 如果用x'代替了-x，就在输出的时候加一撇
                printf("'");
            printf("]");
        }
    }
}

/**
 * 在控制台打印LPModel
 * @param model LPModel结构体
 */
void PrintModel(LPModel model) { // 打印LP模型
    size_t i;
    OF oFunc = model.objective; // 临时拿到目标函数
    ST *subTo = model.subjectTo; // 取到约束数组指针
    printf("Objective Function:\n\t%s:", oFunc.type == 1 ? "max" : "min"); // 目标函数类型
    PrintTerms(oFunc.left, oFunc.leftLen); // 一项一项打印出来
    printf(" = "); // 打印等号
    PrintTerms(oFunc.right, oFunc.rightLen);
    printf("\nSubject to:\n");
    for (i = 0; i < model.stLen; i++) {
        printf("\t");
        PrintTerms(subTo[i].left, subTo[i].leftLen); // 一项一项打印出来
        printf(" ");
        PrintRelation(subTo[i].relation); // 打印关系符号
        printf(" ");
        PrintTerms(subTo[i].right, subTo[i].rightLen);
        printf("\n");
    }
    // 输出变量约束情况
    printf("\nVariables:\n\t");
    short int valid = 1;
    size_t varNum = 0;
    VarItem **allVars = GetVarItems(&varNum, NULL, &valid);
    if (!valid) {
        printf("Failed to get.\n");
    } else {
        VarItem *temp = NULL;
        for (i = 0; i < varNum; i++) {
            temp = allVars[i];
            if (temp->relation != 0) { // 该变量有约束
                printf("%s", temp->keyName);
                PrintRelation(temp->relation);
                printf("%d | ", temp->number);
            } else { // 该变量无约束(unr)
                printf("%s(unr)", temp->keyName);
                if (strlen(temp->formerX) > 0) // 如果已经用x''-x'代替了x(unr)
                    printf("=%s-%s", temp->formerX, temp->latterX);
                printf(" | ");
            }
        }
    }
    printf("\n");
    // 用完后释放，好习惯
    free(allVars);
}

/**
 * 深拷贝整个LP模型
 * @param model 指向待拷贝模型的指针
 * @return 拷贝出来的模型（LPModel结构体）
 * @note 使用完后记得用FreeModel进行销毁
 */
LPModel CopyModel(LPModel *model) {
    size_t i;
    OF *oFunc = &model->objective;
    ST *subTo = model->subjectTo;
    // 建议使用memcpy以便维护
    OF oFuncCopy; // 创建一个目标函数副本
    // 复制目标函数的内存
    memcpy(&oFuncCopy, oFunc, sizeof(OF));
    // 复制目标函数左边和右边的多项式
    oFuncCopy.left = TermsCopy(oFunc->left, oFunc->maxLeftLen, NULL);
    oFuncCopy.right = TermsCopy(oFunc->right, oFunc->maxRightLen, NULL);
    ST *subToCopy = (ST *) calloc(model->maxStLen, sizeof(ST));
    for (i = 0; i < model->stLen; i++) { // 遍历约束
        // 复制每个约束对应的内存
        memcpy(&subToCopy[i], &subTo[i], sizeof(ST));
        // 复制每个约束的左右两边的多项式
        subToCopy[i].left = TermsCopy(subTo[i].left, subTo[i].maxLeftLen, NULL);
        subToCopy[i].right = TermsCopy(subTo[i].right, subTo[i].maxRightLen, NULL);
    }
    LPModel newModel;
    // 复制LPModel本体
    memcpy(&newModel, model, sizeof(LPModel));
    newModel.objective = oFuncCopy;
    newModel.subjectTo = subToCopy;
    return newModel;
}

/**
 * 复制多项式的一个项
 * @param origin 指向一个项(Term)的指针
 * @return 指向复制出来的项的指针
 * @note 记得free
 */
Term *TermCopy(Term *origin) {
    Term *new = (Term *) calloc(1, sizeof(Term));
    memcpy(new, origin, sizeof(Term));
    return new;
}

/**
 * 复制多项式
 * @param origin 指向原多项式的二级指针（Term**）
 * @param maxLen 多项式指针数组最多能容纳多少元素
 * @param copyLen 指向一个变量，这个变量储存复制后的数组长度
 * @return 指向复制出来的多项式的二级指针(Term**)
 * @note copyLen参数可以传入NULL
 */
Term **TermsCopy(Term **origin, size_t maxLen, size_t *copyLen) {
    size_t i;
    Term **new = (Term **) calloc(maxLen, sizeof(Term *));
    for (i = 0; i < maxLen; i++) {
        if (origin[i] != NULL) { // 保证不会访问到无效内存
            new[i] = TermCopy(origin[i]); // 复制每一项
        } else { // 一旦访问到了NULL就说明数组只有这么多内容了
            break;
        }
    }
    if (copyLen != NULL)
        *copyLen = i;
    return new;
}

/**
 * 释放LP模型中的堆内存（销毁模型）
 * @param model 指向LPModel模型的指针
 */
void FreeModel(LPModel *model) {
    size_t i, j;
    // 先处理目标函数
    OF *oFunc = &model->objective; // 地址引用目标函数结构体
    ST *subTo = model->subjectTo;
    for (i = 0; i < oFunc->leftLen; i++) // 释放所有的项
        free(oFunc->left[i]);
    for (i = 0; i < oFunc->rightLen; i++)
        free(oFunc->right[i]);
    oFunc->leftLen = 0;
    oFunc->rightLen = 0;
    free(oFunc->left); // 释放目标函数中的项集
    free(oFunc->right);
    for (i = 0; i < model->stLen; i++) { // 遍历释放约束条件内存
        ST *stTemp = subTo + i;
        for (j = 0; j < stTemp->leftLen; j++) { // 释放所有的项
            free(stTemp->left[j]);
        }
        for (j = 0; j < stTemp->rightLen; j++) {
            free(stTemp->right[j]);
        }
        stTemp->leftLen = 0;
        stTemp->rightLen = 0;
        free(stTemp->left); // 释放该约束中的项集
        free(stTemp->right);
    }
    free(subTo); // 释放约束指针数组占用的内存
    model->stLen = 0;
}