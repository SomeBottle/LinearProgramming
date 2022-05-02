/* 针对Number结构体进行数字计算的模块
 * SomeBottle
 */
#include "public.h"

int OFAdd(long prev, long after) { // 加运算溢出判断，返回1则代表溢出
    if (prev > 0 && after > LONG_MAX - prev) { // 溢出上界
        return 1;
    } else if (prev < 0 && after < LONG_MIN - prev) { // 溢出下界
        return 1;
    }
    return 0;
}

SubNum FractionMul(long prevNume, long prevDeno, long nextNume, long nextDeno) {
    // 运算分数乘法
    SubNum result = {.valid=1};
    long int numeMult; // 相乘后的分子
    long int denoMult; // 相乘后的分母
    long int divisor1 = GCD(prevNume, nextDeno);
    long int divisor2 = GCD(prevDeno, nextNume);
    prevNume /= divisor1;
    nextDeno /= divisor1;
    prevDeno /= divisor2;
    nextNume /= divisor2;
    // 比如 2/3 × 5/4，这里2和4可以约分，就先找出来给约了再乘
    numeMult = prevNume * nextNume;
    denoMult = prevDeno * nextDeno;
    if ((prevNume != 0 && numeMult / prevNume != nextNume) ||
        (prevDeno != 0 && denoMult / prevDeno != nextDeno)) {
        // 发生了溢出
        result.valid = 0;
        printf("WARNING: MULTIPLY Operation overflowed between %ld/%ld and %ld/%ld.\n", prevNume, prevDeno, nextNume,
               nextDeno);
    }
    return result;
}

SubNum FractionAdd(long prevNume, long prevDeno, long nextNume, long nextDeno) {
    // 运算分数加法，专门写出来是为了防止运算溢出
    long int prevAdded; // 通分后前一项的分子
    long int nextAdded; // 通分后后一项的分子
    long int numeAdded; // 相加后的分子
    long int prevNumeFactor; // 通分时前一项分子要乘的因数
    long int nextNumeFactor; // 通分时下一项分子要乘的因数
    long int denoExpanded; // 通分后的分母
    long int commonMul; // 最小公倍数
    long int divisor; // 相加后约分用的最大公约数
    SubNum result = {.valid=1};
    commonMul = LCM(prevDeno, nextDeno); // 算出最小公倍数
    denoExpanded = commonMul; // 通分后用作分母
    if (prevDeno != 0 && denoExpanded / prevDeno != nextDeno) { // 相乘运算溢出判断
        result.valid = 0;
    }
    /* 比如 1/3 + 1/2
     *    prevAdded   nextAdded
     *      _|_       _|_
     * 通分：1*2/3*2 + 1*3/2*3
     *        ↑         ↑
     * prevNumeFactor nextNumeFactor
     */
    prevNumeFactor = commonMul / prevDeno;
    prevAdded = prevNume * prevNumeFactor;
    nextNumeFactor = commonMul / nextDeno;
    nextAdded = nextNume * nextNumeFactor;
    if ((prevNume != 0 && prevAdded / prevNume != prevNumeFactor) ||
        (nextNume != 0 && nextAdded / nextNume != nextNumeFactor)) {
        // 相乘发生溢出
        result.valid = 0;
    }
    if (OFAdd(prevAdded, nextAdded)) { // 相加溢出判断
        result.valid = 0;
    } else {
        numeAdded = prevAdded + nextAdded; // 分子相加
        divisor = GCD(numeAdded, denoExpanded); // 找出分子分母最大公约数
        numeAdded /= divisor;
        denoExpanded /= divisor; // 约分
        result.numerator = numeAdded;
        result.denominator = denoExpanded;
    }
    if (result.valid == 0) { // 发生了溢出错误，运算出来的数字无效，提示一下
        printf("WARNING: ADD Operation overflowed between %ld/%ld and %ld/%ld.\n", prevNume, prevDeno, nextNume,
               nextDeno);
    }
    return result;
}

Number NAdd(Number prev, Number next) { // Number加法
    Number result = {.valid=1};
    long int prevNumerator = prev.numerator;
    long int prevDenominator = prev.denominator;
    long int nextNumerator = next.numerator;
    long int nextDenominator = next.denominator;
    SubNum prevSub = prev.sub;
    SubNum nextSub = next.sub;
    char prevConstName = '\0', nextConstName = '\0';
    if (prev.constant != NULL)
        prevConstName = prev.constant->name;
    if (next.constant != NULL)
        nextConstName = next.constant->name;
    if (prevConstName == '\0' && nextConstName != '\0') {
        // 前一项没有常量，后一项有常量M，比如3和(1+2M)相加，就是将3和1相加变成4+2M
        result.numerator = nextNumerator; // 以有常量M的项为主
        result.denominator = nextDenominator;
        result.constant = next.constant;
        result.constLies = next.constLies;
        // 运算分数加法
        result.sub = FractionAdd(nextSub.numerator, nextSub.denominator, prevNumerator, prevDenominator);
    } else if (prevConstName != '\0' && nextConstName == '\0') {
        // 前一项有常量，后一项没有常量，比如(1+3M)和4相加，那么要做的当然是1和4相加变成5+3M
        result.numerator = prevNumerator; // 以有常量M的项为主
        result.denominator = prevDenominator;
        result.constant = prev.constant;
        result.constLies = prev.constLies;
        // 运算分数加法
        result.sub = FractionAdd(prevSub.numerator, prevSub.denominator, nextNumerator, nextDenominator);
    } else if ((prevConstName == '\0' && nextConstName == '\0') ||
               prevConstName == nextConstName && prev.constLies == next.constLies) {
        // 是同类项，比如 1 + 1 = 2；(1+3M)+(2+4M)=3+7M
        // 先把主项目进行相加
        SubNum fracResult = FractionAdd(prevNumerator, prevDenominator, nextNumerator, nextDenominator);
        result.numerator = fracResult.numerator;
        result.denominator = fracResult.denominator;
        result.valid = fracResult.valid;
        result.constant = prev.constant;
        result.constLies = prev.constLies;
        // 再把子项目进行相加
        result.sub = FractionAdd(prevSub.numerator, prevSub.denominator, nextSub.numerator, nextSub.denominator);
    } else { // 不是同类项，运算无效
        result.valid = 0;
    }
    if (result.constant != NULL && result.numerator == 0) {
        // 如果有常量，但是运算出来分子结果为0，那么常量这一项就可以去除了
        // 比如1+3M和2-3M相加，1+2+3M-3M就只剩3了，常量消失，这个时候就要把这个3从SubNum提到Number内变成3
        result.numerator = result.sub.numerator;
        result.denominator = result.sub.denominator;
        result.constant = NULL; // 常量消失
        result.constLies = 0;
        result.sub.numerator = 0; // 子项目变成0
    }
    if (result.sub.numerator != 0) {
        // 有子项目的时候，必须要子项目和主项目的数字valid值都为1才算有效
        result.valid = result.sub.valid && result.valid;
    }
    return result;
}

Number NSub(Number prev, Number next) { // 减法：prev-next
    next.numerator = -next.numerator;
    next.sub.numerator = -next.sub.numerator; // 取成相反数，1+3M就转换成-1-3M
    return NAdd(prev, next); // 借加法函数一用
}

Number NMul(Number prev, Number next) { // 乘法
    Number result = {.valid=1};
    long int prevNumerator = prev.numerator;
    long int prevDenominator = prev.denominator;
    long int nextNumerator = next.numerator;
    long int nextDenominator = next.denominator;
    SubNum prevSub = prev.sub;
    SubNum nextSub = next.sub;
    SubNum fracResult; // 分数运算结果
    char prevConstName = '\0', nextConstName = '\0';
    if (prev.constant != NULL)
        prevConstName = prev.constant->name;
    if (next.constant != NULL)
        nextConstName = next.constant->name;
    if (prevConstName != '\0' && nextConstName != '\0') { // 不允许有常量M的相乘，比如3M*(1+3M)是绝对不允许的
        result.valid = 0;
    } else {
        // 计算主项目，形如3M × 2 或 2 × 3M 或 4 × 5
        fracResult = FractionMul(prevNumerator, prevDenominator, nextNumerator, nextDenominator);
        result.numerator = fracResult.numerator;
        result.denominator = fracResult.denominator;
        result.valid = fracResult.valid;
        // 下面处理有常量的情况
        if (prevConstName != '\0' && nextConstName == '\0') {
            // 前一项有常量M，后一项没有，形如(3M+1) × 2这样
            // 先算子项目，1 × 2
            result.sub = FractionMul(prevSub.numerator, prevSub.denominator, nextNumerator, nextDenominator);
            result.constant = prev.constant;
            result.constLies = prev.constLies;
        } else if (prevConstName == '\0' && nextConstName != '\0') {
            // 前一项没有常量，后一项有M，形如2 × (3M+1)这样
            // 先算子项目，2 × 1
            result.sub = FractionMul(nextSub.numerator, nextSub.denominator, prevNumerator, prevDenominator);
            result.constant = next.constant;
            result.constLies = next.constLies;
        }
    }
    if (result.sub.numerator != 0) {
        // 有子项目的时候，必须要子项目和主项目的数字valid值都为1才算有效
        result.valid = result.sub.valid && result.valid;
    }
    return result;
}

Number NDiv(Number prev, Number next) { // 除法
    // 除法其实就是乘倒数
    // 除法主要允许(4M+2)/2 4/2 这种，也就是除数最好不要有常量M，NMul无法运算数字就会导致valid=0
    long int temp, subTemp;
    temp = next.numerator; // 转换为倒数
    next.numerator = next.denominator;
    next.denominator = temp;
    if (next.constant != NULL) { // 有常量
        next.constLies = next.constLies == 1 ? 0 : 1; // 把M位置倒过来
        // 有常量就可能有子项目
        subTemp = next.sub.numerator;
        next.sub.numerator = next.sub.denominator;
        next.sub.denominator = subTemp;
        printf("WARNING:Dividing by a Number with CONSTANT is not suggested here.\n"); // 提示不建议
    }
    return NMul(prev, next);
}