#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct inc_constant; // 因为Number和Constant是有互相包含的
struct inc_number; // 需要用到结构体不完全声明(incomplete)

typedef struct inc_constant Constant;
typedef struct inc_number Number;

struct inc_number { // 数字结构体（用于表示分数，小数，整数）（为了方便，整数/小数都转换为分数储存）
    long int numerator; // 分子
    long int denominator; // 分母
    int constLies; // 常量在分子还是分母，0代表分子,1代表分母
    Constant *constant; // 常量指针（这里只能写成指针，不然编译器不认，恰巧我们正好用到指针，一~拍即合）
    int valid; // 这个结构体是否有效（如果转换失败了valid=0）
};

struct inc_constant { // 常量结构体
    char name; // 常量名（一个字符）
    int relation; // 常量关系，数字表示: < 1 ; <= 2 ;3 = ; > 4 ; >= 5
    Number val; // 值
};

typedef struct { // 方程中的一项，包括系数，变量名
    Number coefficient; // 系数
    char variable[4]; // 变量名
} Monomial;

typedef struct { // 目标线性函数
    // 分为等号左边和等号右边两个Monomial数组，加上一个求最大值还是最小值
    Monomial *left;
    int leftNum; // 左侧数量（目标函数左边是z，所以这里只能是1）
    Monomial *right;
    int rightNum; // 右侧数量
    char type[4]; // 最大值(max)还是最小值(min)
} LF;

typedef struct { // 约束条件
    // 分为方程左边和右边两个Monomial数组,加上一个符号字符数组
    Monomial *left;
    int leftNum; // 左侧数量
    Monomial *right; // 方程右边
    int rightNum; // 右侧数量
    char relation[3]; // 关系符号
} ST;

typedef struct { // 线性规划数学模型，包括线性函数数组和约束数组
    LF objective; // 目标函数
    ST *subjectTo; // 约束
    int stNum; // 约束数量
} LPModel;

typedef struct { // 分隔字符串返回结果
    char **split;
    int len;
} SplitResult;


extern SplitResult SplitByChr(char *str, char chr);

extern Number Fractionize(char *str);

extern int FreeModel(LPModel *model);

extern int freeSplitArr(SplitResult *rs);

extern Constant *InConstants(char chr);

extern int IsConstItem(char *str);

extern int PrintModel(LPModel model);

extern Constant *constants;
extern int constantsNum;