#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32 // 根据不同系统编译环境定义清屏和暂停的宏
#define CLEAR system("cls")
#define PAUSE system("pause")
#else
#define CLEAR system("clear")
#define PAUSE printf("Press Enter to continue.\n");\
    getchar()
#endif

#define BUFFER_LEN_PER_ALLOC 50 /** 每次分配给字符串暂存区的元素个数*/
#define CONSTANTS_LEN_PER_ALLOC 10 /** 每次分配给常数项暂存区的元素个数*/
#define RESET_BUFFER (char *) calloc(BUFFER_LEN_PER_ALLOC, sizeof(char)) /** 字符串暂存区，最开始分配50个*/
#define ST_LEN_PER_ALLOC 5 /** 每次分配给约束SubjectTo的元素个数*/
#define TERMS_LEN_PER_ALLOC 5 /** 每次分配给方程中多项式的元素个数*/

struct inc_constant; // 因为Number和Constant是有互相包含的
struct inc_number; // 需要用到结构体不完全声明(incomplete)

typedef struct inc_constant Constant;
typedef struct inc_number Number;

typedef struct {
    // 附在Number中的子数字，比如人工变量法可能出现的2+3M中的2
    // 因为式中只可能出现一个常量，所以全都可以归纳成 SubNum + Number这种情况
    long int numerator;
    long int denominator;
    short int valid;
} SubNum;

struct inc_number { // 数字结构体（用于表示分数，小数，整数）（为了方便，整数/小数都转换为分数储存）
    long int numerator; // 分子
    long int denominator; // 分母
    SubNum sub; // 子数值
    short int constLies; // 常量在分子还是分母，0代表分子,1代表分母
    Constant *constant; // 常量指针（这里只能写成指针，不然编译器不认，恰巧我们正好用到指针，一~拍即合）
    short int valid; // 这个结构体是否有效（如果转换失败了valid=0）
};

struct inc_constant { // 常量结构体
    char name; // 常量名（一个字符）
    // 关系符号 -2代表<= -1代表< 1代表> 2代表>= 3代表=
    short int relation; // 常量关系
    Number val; // 值
};

typedef struct { // 方程中的一项，包括系数，变量名
    Number coefficient; // 系数
    char variable[8]; // 变量名
    short int inverted; // 是否取了相反数（用于x<=0的情况，需要用x'=-x代替）
} Term;

typedef struct { // 目标线性函数
    // 分为等号左边和等号右边两个Term数组，加上一个求最大值还是最小值
    Term **left;
    size_t leftLen; // 左侧数量（目标函数左边是z，所以这里只能是1）
    Term **right;
    size_t rightLen; // 右侧数量
    size_t maxLeftLen; // 左侧最多容纳的项数
    size_t maxRightLen; // 右侧最多容纳的项数
    short int type; // 最大值(max用1代表)还是最小值(min用-1代表)
} OF;

typedef struct { // 约束条件
    // 分为方程左边和右边两个Term数组,加上一个符号字符数组
    Term **left;
    size_t leftLen; // 左侧数量
    Term **right; // 方程右边
    size_t rightLen; // 右侧数量
    size_t maxLeftLen; // 左侧最多容纳的项数
    size_t maxRightLen; // 右侧最多容纳的项数
    // 关系符号 -2代表<= -1代表< 1代表> 2代表>= 3代表=
    short int relation;
} ST;

typedef struct { // 线性规划数学模型，包括线性函数数组和约束数组
    OF objective; // 目标函数
    ST *subjectTo; // 约束
    size_t stLen; // 约束数量
    size_t maxStLen; // 最多能容纳多少约束
    short int valid; // 是否有效
} LPModel;

typedef struct { // 分隔字符串返回结果
    char **split;
    int len;
} SplitResult;

extern Constant *constants;
extern int constArrLen;
extern int constantsNum;

// DataReader Funcs below:

extern LPModel Parser(FILE *fp); // 外部变量，定义于dataParser
extern void LPTrans(LPModel *model);

extern void PushTerm(Term ***terms, Term *toPut, size_t pos, size_t *ptr, size_t *maxLen, short int *valid);

extern void PushST(ST **subjectTo, size_t *ptr, size_t *maxLen, ST target, short int *valid);

// Basic Funcs below:

extern SplitResult SplitByChr(char *str, char chr);

extern void freeSplitArr(SplitResult *rs);

extern Number Fractionize(char *str);

extern double Decimalize(Number num);

extern char *Int2Str(int num);

extern int CmbSmlTerms(Term **terms, size_t *termsLen, int forOF);

extern LPModel CopyModel(LPModel *model);

extern Term *TermCopy(Term *origin);

extern Term **TermsCopy(Term **origin, size_t maxLen, size_t *copyLen);

extern void FreeModel(LPModel *model);

extern Constant *InConstants(char chr);

extern int IsConstTerm(char *str);

extern short int ValidVar(char *str);

extern size_t RmvTerm(Term **terms, size_t len, size_t pos, short int clean);

extern void PrintModel(LPModel model);

extern long int GCD(long int num1, long int num2);

extern long int LCM(long int num1, long int num2);

extern void *MemJoin(void *prev, size_t prevLen, void *next, size_t nextLen, size_t eachSize);

// Number Operations Funcs below:

extern int OFAdd(long prev, long after);

extern Number NAdd(Number prev, Number next);

extern Number NSub(Number prev, Number next);

extern Number NMul(Number prev, Number next);

extern Number NDiv(Number prev, Number next);

extern Number NInv(Number num);

// hashTable Funcs below:

#include "hashTable.h"

// Router

extern void Entry(LPModel *model);
