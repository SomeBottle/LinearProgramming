/*
 * 单纯形法部分
 * 包括化为标准型，单纯形法相关的部分
 * SomeBottle - 20220518
 */

#include "public.h"
#include "simplex.h"
#include "matrix.h"

static void TermsInvert(Term **terms, size_t termsLen);

static void InvertNegVars(Term **terms, size_t termsLen);

static Term *CreateSlack(long int *currentSub, short int *valid);

static void TermsSort(Term **terms, size_t termsLen);

static int VarCmp(char *str1, char *str2);

/**
 * 单纯形表运算入口
 * @param model 指向LP模型的指针
 */
void newSimplex(LPModel *model) {
    LPStandardize(model, 0); // 按单纯形法需求化为标准型
    printf("\n---------------\n> Standard Form\n\n");
    PrintModel(*model); // 打印一下模型
    PAUSE;
    LPAlign(model); // 对模型约束进行对齐操作
    if (model->valid) {

    } else { // 模型变得invalid了
        printf("ERROR occurred during the Standardization and the Alignment :( \n");
    }
}

/**
 * 将LP模型化为标准型，用于单纯形算法
 * @param model 指向LPModel的一个指针
 * @param dual 1/0 代表 是/否 以对偶单纯形法的标准转化
 * @note 很惭愧这一部分我写的非常暴力，感觉我化身为了for循环战士
 * @note 暂时咱也想不到什么好的优化方法了，以后了解了一些算法我再看看能不能改写
 */
void LPStandardize(LPModel *model, short int dual) {
    // 感谢文章：https://www.bilibili.com/read/cv5287905
    size_t i, j, k;
    // 当前x的下标
    long int currentSub;
    short int valid = model->valid;
    // 这一步主要是为了获得当前的x的最大下标
    // 比如最大下标是x67中的67，那么松弛变量就从68开始，也就是x68
    // 值得注意的是，松弛变量并没有变量名只能三位的限制
    free(GetVarItems(NULL, &currentSub, &valid));
    if (model->objective.type != 1) { // 目标函数不是求max
        Term *temp = NULL;
        model->objective.type = 1;
        temp = model->objective.left[0];
        temp->coefficient = NInv(temp->coefficient); // 变成max(-z)
        // 等式两边同时取反
        TermsInvert(model->objective.right, model->objective.rightLen);
    }

    Term *termBuffer = NULL; // 单项暂存区
    VarItem *varTemp = NULL; // 变量约束暂存区
    for (i = 0; i < model->objective.rightLen; i++) {
        // 扫描处理没有限制的变量(替换模型中无约束(unr)变量为x'' - x'的形式)
        // 此时termBuffer指向目标函数右边的某一项
        char targetVar[8]; // 暂存变量名
        termBuffer = model->objective.right[i];
        varTemp = GetVarItem(termBuffer->variable); // 获得变量约束项
        if (!varTemp->relation) { // 这一项x没有约束(unr)
            strcpy(targetVar, termBuffer->variable); // 复制变量名，方便后面在约束中寻找
            Number originC = termBuffer->coefficient; // 备份unr项的系数
            // 创建前一个变量x''>=0
            Term *formerX = CreateSlack(&currentSub, &valid);
            Number formerC = Fractionize("1"); // 前一项的系数
            formerC = NMul(formerC, originC); // 和原来的系数相乘（乘法分配律）
            formerX->coefficient = formerC; // 存入系数
            // 将目标函数右侧这个unr项给free掉（没有用了）
            free(termBuffer);
            termBuffer = NULL; // termBuffer置空，防止野指针
            model->objective.right[i] = formerX; // 这个位置被替代成前一个变量x''
            // 创建后一个变量x'
            Term *latterX = CreateSlack(&currentSub, &valid);
            Number latterC = Fractionize("-1"); // 后一项的系数 (要形成x''-x')
            latterC = NMul(latterC, originC); // 仍然是乘法分配律
            latterX->coefficient = latterC; // 存入系数
            // 加入到目标函数中
            PushTerm(&model->objective.right, latterX, (long long int) i + 1, &model->objective.rightLen,
                     &model->objective.maxRightLen,
                     &valid);
            // 用x''-x'替代了x(unr)，需要把x''和x'记录到对应的表项中
            strcpy(varTemp->formerX, formerX->variable);
            strcpy(varTemp->latterX, latterX->variable);
            // 遍历位后移（加入了两个元素，其中替换了一个元素，增加了一个元素，实际增加一位）
            i++;
            for (j = 0; j < model->stLen; j++) { // 遍历约束
                ST *stTemp = model->subjectTo + j; // 暂存约束
                for (k = 0; k < stTemp->leftLen; k++) { // 遍历每个约束左边的项
                    if (strcmp(stTemp->left[k]->variable, targetVar) == 0) {
                        // 找到unr变量
                        Number stOriginC = stTemp->left[k]->coefficient; // 备份原系数
                        Term *stFormerX = TermCopy(formerX); // 复制前一项x''
                        formerC = Fractionize("1"); // 复用formerC变量
                        formerC = NMul(formerC, stOriginC); // 乘法分配律
                        stFormerX->coefficient = formerC;
                        Term *stLatterX = TermCopy(latterX); // 复制后一项x'
                        latterC = Fractionize("-1");
                        latterC = NMul(latterC, stOriginC); // 乘法分配律
                        stLatterX->coefficient = latterC;
                        // 原来的这一项的内存已经没用了，free掉
                        free(stTemp->left[k]);
                        stTemp->left[k] = stFormerX; // 替换为x''
                        // 将x'项推入约束
                        PushTerm(&stTemp->left, stLatterX, (long long int) k + 1, &stTemp->leftLen, &stTemp->maxLeftLen,
                                 &valid);
                        // 遍历位后移（加入了两个元素，其中替换了一个元素，增加了一个元素，实际增加一位）
                        k++;
                    }
                }
            }
            targetVar[0] = '\0'; // 目标字符串置空
        }
    }

    ST *stTemp; // 约束暂存temp
    for (i = 0; i < model->stLen; i++) {
        stTemp = model->subjectTo + i;

        // 接着检查当前约束右边是不是正数
        // 此时termBuffer指向当前约束右边的常数项
        termBuffer = stTemp->right[0];
        if ((!dual && Decimalize(termBuffer->coefficient) < 0) || // 针对普通单纯形法,若当前约束右边不是正数，整个方程需要乘个-1
            (dual && stTemp->relation > 0 && stTemp->relation != 3)) { // 针对对偶单纯形法,为了方便加入松弛变量，所有大于号>=,>必须变成<=,<
            // 右侧取相反数
            termBuffer->coefficient = NInv(termBuffer->coefficient);
            // 约束左侧取反
            TermsInvert(stTemp->left, stTemp->leftLen);
            if (stTemp->relation != 3) // 不是等式
                stTemp->relation = -stTemp->relation; // 不等符号取反，比如 >= 取反成为 <=
        }
        termBuffer = NULL;

        if (stTemp->relation != 3) {// 不是等式
            // 此时termBuffer指向待加入目标函数的项
            // 构造一个松弛/剩余变量项
            termBuffer = CreateSlack(&currentSub, &valid);
            // 这一个松弛变量先加到目标函数右边，系数为0
            termBuffer->coefficient = Fractionize("0");
            // 推入目标函数
            PushTerm(&model->objective.right, termBuffer, -1, &model->objective.rightLen, &model->objective.maxRightLen,
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
            PushTerm(&stTemp->left, termBuffer, -1, &stTemp->leftLen, &stTemp->maxLeftLen, &valid);
            termBuffer = NULL;
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
    model->valid = valid; // 标准型化处理是否成功
}

/**
 * 将约束进行对齐
 * @param model 指向待处理LP模型的指针
 * @param valid 指向一个变量的指针，这个变量储存 1/0 代表 是/否 对齐成功
 * @note 前提：model已经经过LPStandardize处理
 */
void LPAlign(LPModel *model) {
    size_t i, j;
    ST *stTemp;
    Term **ofTerms = model->objective.right;
    short int valid = 1;
    for (i = 0; i < model->stLen; i++) {
        stTemp = &model->subjectTo[i]; // 当前约束左边的多项式
        for (j = 0; j < model->objective.rightLen; j++) {
            if (j >= stTemp->leftLen || strcmp(stTemp->left[j]->variable, ofTerms[j]->variable) != 0) {
                // 当前位置的约束的变量名和目标函数中对应位置的对不上，说明未对齐
                Term *new = TermCopy(ofTerms[j]); // 复制目标函数中的这一项
                new->coefficient = Fractionize("0"); // 当然，插入到约束中时其系数只能是0
                // 将该项目插入到该约束的指定位置
                PushTerm(&stTemp->left, new, (long long int) j, &stTemp->leftLen, &stTemp->maxLeftLen, &valid);
                // 这里估计要用到双指针，多一个k变量
            }
        }
    }
    model->valid = model->valid && valid;
}

/**
 * 创建一个松弛/剩余变量
 * @param currentSub 当前x开头变量的下标，比如现在是x6，松弛变量就从x7开始
 * @param valid 指向一个变量的指针，这个变量的值 1/0 代表 是/否 成功创建
 * @return 一个指向创建的松弛变量的项(Term)的指针
 * @note 这个函数会顺带把创建的变量存入哈希表，松弛/剩余变量全都是>=0
 */
Term *CreateSlack(long int *currentSub, short int *valid) {
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
 * 根据变量名对多项式进行排序
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen 多项式指针数组的长度
 * @note 采用选择排序算法
 */
void TermsSort(Term **terms, size_t termsLen) {
    size_t i, j, minIndex;
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
        serial1 = strlen(str1) > 1 ? strtol(str1 + 1, NULL, 10) : 0;
        serial2 = strlen(str2) > 1 ? strtol(str2 + 1, NULL, 10) : 0;
    }
    return serial1 == serial2 ? 0 : serial1 - serial2;
}

/**
 * 将多项式中所有项的系数取相反数
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen  多项式指针数组长度
 */
void TermsInvert(Term **terms, size_t termsLen) {
    size_t i;
    for (i = 0; i < termsLen; i++)
        terms[i]->coefficient = NInv(terms[i]->coefficient);
}

/**
 * 将多项式中的非正变量x暂时转换为-x' (x'=-x)
 * @param terms 指向 多项式指针数组 的指针
 * @param termsLen 多项式指针数组的长度
 */
void InvertNegVars(Term **terms, size_t termsLen) {
    size_t i;
    for (i = 0; i < termsLen; i++) {
        // 从哈希表中取出变量自身的约束
        VarItem *get = GetVarItem(terms[i]->variable);
        if (get->relation < 0 && get->number == 0) {
            // 取相反数，比如x1<=0，这里相当于令x1=-x1'，那么x1'=-x1，就有 x1' >= 0
            terms[i]->coefficient = NInv(terms[i]->coefficient);
            terms[i]->inverted = 1; // 标记用x'代替了-x
        }
    }
}


