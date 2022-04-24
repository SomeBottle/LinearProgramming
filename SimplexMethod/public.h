#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


typedef struct { // 方程中的一项，包括系数，变量名
    char coefficient[4]; // 系数
    char variable[3]; // 变量名
} Monomial;

typedef struct { // 目标线性函数
    // 分为等号左边和等号右边两个Monomial数组，加上一个求最大值还是最小值
    Monomial *left;
    Monomial *right;
    char type[4]; // 最大值(max)还是最小值(min)
} LF;

typedef struct { // 约束条件
    // 分为方程左边和右边两个Monomial数组,加上一个符号字符数组
    Monomial *left;
    Monomial *right; // 方程右边
    char relation[3]; // 关系符号
} ST;

typedef struct { // 线性规划数学模型，包括线性函数数组和约束数组
    LF goal; // 目标函数
    ST *constraints; // 约束
} LPModel;

typedef struct { // 分隔字符串返回结果
    char **split;
    int len;
} SplitResult;

extern SplitResult SplitByChr(char *str, int strLen, char chr);
extern int freeSplitArr(SplitResult *rs);
