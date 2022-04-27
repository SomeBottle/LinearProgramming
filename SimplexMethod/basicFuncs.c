#include "public.h"

SplitResult SplitByChr(char *str, char chr) { // (字符串,字符) 按字符分隔字符串，会返回一个二维数组
    int i;
    int stringLen = strlen(str);
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

long int CommonDiv(long int num1, long int num2) {
    // 寻找两数最大公约数(欧几里得算法)
    long int dividend; // 被除数
    long int divisor; // 除数
    long int remainder; // 余数
    if (num1 > num2) {
        dividend = num1;
        divisor = num2;
    } else {
        dividend = num2;
        divisor = num1;
    }
    do {
        remainder = dividend % divisor;
        dividend = divisor;
        divisor = remainder ? remainder : divisor;
    } while (remainder != 0);
    return divisor;
}

Number Fractionize(char *str) { // 分数化一个字符串
    Number result = {.valid=1};
    int i;
    int len = strlen(str);
    // 创建一份字符串拷贝
    char *strCopy = (char *) calloc(len + 1, sizeof(char));
    char *convEndPtr; // 转换类型后所在位置的指针
    char *divPtr; // 分割用指针
    long int numerator;
    long int denominator;
    long int divisor; // 最大公约数
    for (i = 0; i < len; i++) // 拷贝字符串
        strCopy[i] = str[i];
    if (strchr(strCopy, '/') != NULL) { // 分数表示
        divPtr = strtok(strCopy, "/"); // 按'/‘分割
        // 分子转换为10进制long int类型
        numerator = strtol(divPtr, &convEndPtr, 10);
        if (*convEndPtr == '\0') { // 分子能完全转换为整数
            divPtr = strtok(NULL, "/"); // 继续再分割一次
            denominator = strtol(divPtr, &convEndPtr, 10);
            if (*convEndPtr == '\0') { // 分母也能完全转换为整数
                divisor = labs(CommonDiv(numerator, denominator));
                // 找出最大公约数（绝对值）
                denominator = denominator / divisor;
                numerator = numerator / divisor; // 约分操作
                result.numerator = numerator;
                result.denominator = denominator; // 存入结构体
            } else {
                result.valid = 0; // 出错了，数字无效
            }
        } else {
            result.valid = 0; // 出错了，数字无效
        }
    } else if (strchr(strCopy, '.') != NULL) { // 表示为了小数
        double decimal = strtod(strCopy, &convEndPtr); // 先转换为double数
        if (*convEndPtr == '\0') { // 转换成功
            strtok(strCopy, "."); // 按小数点分割
            divPtr = strtok(NULL, "."); // 获得小数点后面的部分(NULL就会接着上一次的位置继续)
            /* 这里的原理就像这样：-2.45 -分数形式-> -245/100 -约分-> -49/20 */
            if (divPtr != NULL) {
                int digits = strlen(divPtr); // 小数位数
                denominator = (long int) pow(10, digits); // 计算出分母
                numerator = (long int) (decimal * denominator); // 计算出分子
                divisor = labs(CommonDiv(numerator, denominator));
                // 最大公约数约分，公约数规定为正数，防止符号问题
                denominator = denominator / divisor;
                numerator = numerator / divisor;
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
    free(strCopy); // 释放拷贝的字符串
    return result;
}

int printModel(LPModel model) { // 打印
    LF oFunc = model.objective; // 临时拿到目标函数
    ST *subTo = model.subjectTo; // 取到约束数组指针
    return 1;
}
