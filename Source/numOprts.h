/**
 * Number结构体操作部分
 * SomeBottle 20220523
 */

#ifndef LPSIMPLEX_NUMOPRTS_H
#define LPSIMPLEX_NUMOPRTS_H

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

extern int OFAdd(long prev, long after);

extern Number NAdd(Number prev, Number next);

extern Number NSub(Number prev, Number next);

extern Number NMul(Number prev, Number next);

extern Number NDiv(Number prev, Number next);

extern Number NInv(Number num);

#endif //LPSIMPLEX_NUMOPRTS_H
