/*
 * 单纯形法部分
 * 包括化为标准型，单纯形法相关的部分
 * SomeBottle - 20220518
 */

#include "public.h"

static int VarCmp(char *str1, char *str2);

static void TermsInvert(Term **terms, size_t termsLen);

static void InvertNegVars(Term **terms, size_t termsLen);

static Term *CreateSlack(int *currentSub, short int *valid);

/**
 * 将LP模型化为标准型，用于单纯形算法
 * @param model 指向LPModel的一个指针
 */
void LPStandardize(LPModel *model) {
    // 感谢文章：https://www.bilibili.com/read/cv5287905
    int i;
    size_t varNum;
    // 当前x的下标
    int currentSub;
    short int valid = 1;
    // 这一步主要是为了获得当前的x的最大下标
    // 比如最大下标是x67中的67，那么松弛变量就从68开始，也就是x68
    // 值得注意的是，松弛变量并没有变量名只能三位的限制
    free(GetVarItems(&varNum, &currentSub, &valid));
    if (model->objective.type != 1) { // 目标函数不是求max
        Term *temp = NULL;
        model->objective.type = 1;
        temp = model->objective.left[0];
        temp->coefficient = NInv(temp->coefficient); // 变成max(-z)
        // 等式两边同时取反
        TermsInvert(model->objective.right, model->objective.rightLen);
    }

    ST *stTemp; // 约束暂存temp
    Term *termBuffer = NULL;
    for (i = 0; i < model->stLen; i++) {
        stTemp = model->subjectTo + i;
        if (stTemp->relation != 3) {// 不是等式
            // 此时termBuffer指向待加入目标函数的项
            // 构造一个松弛/剩余变量项
            termBuffer = CreateSlack(&currentSub, &valid);
            // 这一个松弛变量先加到目标函数右边，系数为0
            termBuffer->coefficient = Fractionize("0");
            // 推入目标函数
            PushTerm(&model->objective.right, termBuffer, &model->objective.rightLen, &model->objective.maxRightLen,
                     &valid);
            // 接下来准备将松弛/剩余变量加入到约束中
            // 此时termBuffer指向待加入约束方程的项
            termBuffer = TermCopy(termBuffer); // 复制一份，作为加入约束的剩余变量
        }
        switch (stTemp->relation) {
            case 1: // >
            case 2: // >=
                termBuffer->coefficient = Fractionize("-1"); // 减去剩余变量
                break;
            case -1: // <
            case -2: // <=
                termBuffer->coefficient = Fractionize("1"); // 加上松弛变量
                break;
            default:
                break;
        }
        if (termBuffer != NULL) { // termBuffer不为空，说明有松弛/剩余变量加入
            // 将松弛/剩余变量推入当前约束的左边
            stTemp->relation = 3; // 弱约束变强约束
            PushTerm(&stTemp->left, termBuffer, &stTemp->leftLen, &stTemp->maxLeftLen, &valid);
            termBuffer = NULL;
        }
        // 接着检查当前约束右边是不是正数
        // 此时termBuffer指向当前约束右边的常数项
        termBuffer = stTemp->right[0];
        if (Decimalize(termBuffer->coefficient) < 0) {
            // 当前约束右边不是正数，整个方程需要乘个-1
            // 右侧取相反数
            termBuffer->coefficient = NInv(termBuffer->coefficient);
            // 约束左侧取反
            TermsInvert(stTemp->left, stTemp->leftLen);
        }
        // 对当前约束左边的项目按变量名进行排序
        TermsSort(stTemp->left, stTemp->leftLen);
        // 临时将x<=0的项转换为-x>=0
        InvertNegVars(stTemp->left, stTemp->leftLen);
    }
    // 对目标函数右边的项目按变量名进行排序
    TermsSort(model->objective.right, model->objective.rightLen);
    // 临时将x<=0的项转换为-x>=0
    InvertNegVars(model->objective.right, model->objective.rightLen);
    // 处理没有限制的变量

}

/**
 * 复制多项式的一个项
 * @param origin 指向一个项(Term)的指针
 * @return 指向复制出来的项的指针
 * @note 记得free
 */
Term *TermCopy(Term *origin) {
    Term *new = (Term *) calloc(1, sizeof(Term));
    strcpy(new->variable, origin->variable); // 拷贝变量名
    new->coefficient = origin->coefficient; // 拷贝系数
    return new;
}

/**
 * 创建一个松弛/剩余变量
 * @param currentSub 当前x开头变量的下标，比如现在是x6，松弛变量就从x7开始
 * @param valid 指向一个变量的指针，这个变量的值 1/0 代表 是/否 成功创建
 * @return 一个指向创建的松弛变量的项(Term)的指针
 * @note 这个函数会顺带把创建的变量存入哈希表，松弛/剩余变量全都是>=0
 */
Term *CreateSlack(int *currentSub, short int *valid) {
    char *strTemp = Int2Str(++(*currentSub)); // 临时字符串指针
    Term *termBuff = (Term *) calloc(1, sizeof(Term));
    termBuff->variable[0] = 'x'; // 剩余变量和松弛变量都是x开头
    strcat(termBuff->variable, strTemp);
    // 为松弛变量创建新的哈希表项目并存入哈希表
    VarItem *slack = CreateVarItem(termBuff->variable, 2, 0);
    *valid = *valid && PutVarItem(slack);
    // 用完后释放，是好习惯哦
    free(strTemp);
    return termBuff;
}

/**
 * 替换约束中无约束(unr)变量为x'' - x'的形式
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen  多项式指针数组长度
 * @param currentSub 指向一个变量的指针，该变量代表当前x变量下标大小
 */
void RplUnrVars(Term **terms, size_t termsLen, int *currentSub) {
    unsigned int i;
    VarItem *varTemp = NULL; // 变量约束暂存
    for (i = 0; i < termsLen; i++) {
        varTemp = GetVarItem(terms[i]->variable);
        if (varTemp->relation == 0) { // 是一个无约束(unr)变量
            // PushTerm函数需要新增一个pos参数，用于在多项式指定位置插入
            // 另外用到RmvTerm方法（原型定义在basicFuncs中）
        }
    }
}

/**
 * 将多项式中所有项的系数取相反数
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen  多项式指针数组长度
 */
void TermsInvert(Term **terms, size_t termsLen) {
    unsigned int i;
    for (i = 0; i < termsLen; i++)
        terms[i]->coefficient = NInv(terms[i]->coefficient);
}

/**
 * 将多项式中的非正变量x暂时转换为-x
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen 多项式指针数组的长度
 */
void InvertNegVars(Term **terms, size_t termsLen) {
    unsigned int i;
    for (i = 0; i < termsLen; i++) {
        // 从哈希表中取出变量自身的约束
        VarItem *get = GetVarItem(terms[i]->variable);
        if (get->relation < 0 && get->number == 0)
            // 取相反数，比如x1<=0，这里相当于令x1=-x1'，那么x1'=-x1，就有 x1' >= 0
            terms[i]->coefficient = NInv(terms[i]->coefficient);
    }
}

/**
 * 根据变量名对多项式进行排序
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen 多项式指针数组的长度
 * @note 采用选择排序算法
 */
void TermsSort(Term **terms, size_t termsLen) {
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