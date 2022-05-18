#include "public.h"

SplitResult SplitByChr(char *str, char chr) { // (字符串,字符) 按字符分隔字符串，会返回一个二维数组
    int i;
    size_t stringLen = strlen(str);
    size_t bufferSize = sizeof(char) * stringLen;
    char *buffer = (char *) malloc(bufferSize); // 字符串暂存区
    int bufferLen = 0; // 暂存区字符数组长度
    char **arr = (char **) malloc(sizeof(char *) * stringLen); // 数组第一维
    int arrLen = 0; // 返回二维数组第一维的大小
    for (i = 0; i < stringLen + 1; i++) {
        unsigned int currentChr;
        int lastOne = 0; // 最后一项单独处理
        if (i < stringLen) {
            currentChr = (unsigned int) str[i];
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

int freeSplitArr(SplitResult *rs) { // 门当对户地释放SplitByChr的返回结果中的字符二维数组
    int i;
    for (i = 0; i < rs->len; i++) {
        free(rs->split[i]);
    }
    free(rs->split);
    rs->split = NULL;
    return 1;
}

void *MemJoin(void *prev, size_t prevLen, void *next, size_t nextLen, size_t eachSize) {
    // 将两段内存连接成一块（重分配），返回指向新分配内存开头的指针
    // (前一段内存的起址,前一段内存长度,后一段内存的起址,后一段内存长度,类型储存字节大小)
    void *joined = malloc(eachSize * (prevLen + nextLen));
    size_t prevSize = eachSize * prevLen;
    memcpy(joined, prev, prevSize); // 复制前一段
    memcpy(joined + prevSize, next, eachSize * nextLen); // 复制后一段
    // 返回void指针，记得free！
    return joined;
}

Constant *InConstants(char chr) { // 查找字符chr在常量数组中对应的地址，找不到返回NULL
    Constant *ptr = NULL;
    if (constants != NULL) {
        int i;
        for (i = 0; i < constantsNum; i++) {
            if (constants[i].name == chr) {
                ptr = &constants[i];
            }
        }
    }
    return ptr;
}

int IsConstItem(char *str) { // 判断整个字符串是不是一个常数项
    int i;
    unsigned long int len = strlen(str);
    for (i = 0; i < len; i++) {
        // 既不是数字，也不包含常量
        if (strchr("0123456789/+-.", str[i]) == NULL && InConstants(str[i]) == NULL) {
            return 0;
        }
    }
    return 1;
}

long int GCD(long int num1, long int num2) {
    // 寻找两数最大公约数(欧几里得算法)
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

Number Fractionize(char *str) { // 分数化一个字符串 3M/4 2.45M 5M 3/4M 3M/4M 3/4...
    Number result = {.valid=1};
    Constant *cstPtr1 = NULL; // 常量临时指针1
    int i;
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

size_t RmvTerm(Term **terms, size_t len, int pos, int clean) {
    // (Term指针, 数组长度, 移除位置, 是否free)
    // 从Term数组中移除某一项，比如从约束式子左边移去一项
    // 注：这里的Remove只是形式上的，并没有对项目进行free
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

int PrintTerms(Term **item, size_t itemNum) { // 打印多项式
    int i;
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
            printf(numeTemp > 0 ? " + %ld" : " - %ld", labs(numeTemp));
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
        if (strlen(item[i]->variable) > 0) // 有变量名的话
            printf("[%s]", item[i]->variable); // 打印变量名
    }
    return 1;
}

int PrintModel(LPModel model) { // 打印LP模型
    int i;
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
        switch (subTo[i].relation) {
            case -2:
                printf(" %s ", "<="); // 打印关系符号
                break;
            case -1:
                printf(" %s ", "<");
                break;
            case 1:
                printf(" %s ", ">");
                break;
            case 2:
                printf(" %s ", ">=");
                break;
            case 3:
                printf(" %s ", "=");
                break;
        }
        PrintTerms(subTo[i].right, subTo[i].rightLen);
        printf("\n");
    }
    return 1;
}

int FreeModel(LPModel *model) { // 释放LP模型中分配的内存
    int i, j;
    // 先处理目标函数
    OF *oFunc = &model->objective; // 地址引用目标函数结构体
    ST *subTo = model->subjectTo;
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
    return 1;
}