/*
 * 单纯形法部分
 * 包括化为标准型，单纯形法相关的部分
 * SomeBottle - 20220518
 */

#include "public.h"

static int VarCmp(char *str1, char *str2);

/**
 * 将LP模型标准化，用于单纯形算法
 * @param model 指向LPModel的一个指针
 */
void LPStandardize(LPModel *model) {
    // 感谢文章：https://www.bilibili.com/read/cv5287905
    
}

/**
 * 根据变量名对多项式进行排序
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen 多项式指针数组的长度
 * @param rplNeg 1/0 代表 是/否 替换非正决策变量（处理x<=0这种情况）
 * @note 采用选择排序算法
 */
void TermsSort(Term **terms, size_t termsLen, int rplNeg) {
    unsigned int i, j, minIndex;
    for (i = 0; i < termsLen; i++) {
        minIndex = i;
        // 找到后方最小的变量名
        for (j = i + 1; j < termsLen; j++) {
            // 后方有更小的变量名
            if (VarCmp(terms[minIndex]->variable, terms[j]->variable) > 0) {
                minIndex = j;
            }
        }
        if (minIndex != i) { // 进行交换
            Term *temp = terms[i];
            terms[i] = terms[minIndex];
            terms[minIndex] = temp;
        }
    }
}

/**
 * 变量字符串排序函数
 * @param str1 前一个代表变量的字符串
 * @param str2 后一个代表变量的字符串
 * @return 比较结果
 * @note >0 代表 str1>str2
 * @note <0 代表 str1<str2
 * @note =0 代表 两者权重相同
 */
int VarCmp(char *str1, char *str2) {
    long int serial1, serial2;
    if (str1[0] != str2[0]) {
        return str1[0] > str2[0] ? 1 : -1;
    } else {
        serial1 = strlen(str1) > 1 ? atoi(str1 + 1) : 0;
        serial2 = strlen(str2) > 1 ? atoi(str2 + 1) : 0;
    }
    return serial1 == serial2 ? 0 : serial1 - serial2;
}