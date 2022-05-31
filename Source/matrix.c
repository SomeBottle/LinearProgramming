/**
 * 矩阵操作部分
 * SomeBottle 20220524
 */
#include "public.h"
#include "matrix.h"

/**
 * 将LP模型转换为单纯形表使用的矩阵
 * @param model 指向完成标准化和对齐的LP模型的指针
 * @param lack 如果valid=0,指向一个数组，存放哪几行需要加入人工变量。反之为NULL
 * @param valid 指向一个变量的指针，这个变量储存 1/0 代表 是/否 成功转化为矩阵
 * @return 一个储存着矩阵的结构体(SimplexMatrix)
 * @note 找不到单位阵的时候会设置valid=0,这个时候应该尝试加入人工变量
 * @note lack如果不是NULL也要记得free
 * @note 涉及大量分配堆内存操作，记得free! 使用RevokeSMatrix函数
 * @note 这个函数会剔除掉目标函数中的常数项，因为常数项不影响结果解
 */
SimplexMatrix CreateSMatrix(LPModel *model, size_t **lack, short int *valid) {
    size_t i, j, lackPtr;
    SimplexMatrix new = {};
    size_t objectiveLen = model->objective.rightLen; // 临时储存目标函数右边项数
    for (i = 0; i < objectiveLen; i++) {
        // 在转化为矩阵时剔除掉目标函数中的常数项，常数项对解没有影响，在最终计算目标函数值的时候加回来即可
        if (strlen(model->objective.right[i]->variable) <= 0) {
            model->objective.rightLen = RmvTerm(model->objective.right, objectiveLen, i, 1);
        }
    }
    Term **ofTerms = model->objective.right;
    new.ofLen = model->objective.rightLen; // 获得矩阵的列数
    new.basicLen = model->stLen; // 获得矩阵的行数（基变量的个数）
    // 给目标函数价值系数分配空间
    new.ofCosts = (Number **) calloc(new.ofLen, sizeof(Number *));
    // 给所有变量名分配空间
    new.varNames = (char **) calloc(new.ofLen, sizeof(char *));
    // 给技术系数矩阵分配空间（行）
    new.cMatrix = (Number ***) calloc(new.basicLen, sizeof(Number **));
    // 给基变量名数组分配空间
    new.basicVars = (char **) calloc(new.basicLen, sizeof(char *));
    // 给基变量对应价值系数分配空间
    new.basicCosts = (Number **) calloc(new.basicLen, sizeof(Number *));
    for (i = 0; i < new.basicLen; i++) {
        // 分配矩阵的列，多分配一列是因为开头一列存放的是每个约束右侧的常数b
        new.cMatrix[i] = (Number **) calloc(new.ofLen + 1, sizeof(Number *));
        Number *bNum = (Number *) calloc(1, sizeof(Number)); // 在堆内存中分配一个数字结构体
        *bNum = model->subjectTo[i].right[0]->coefficient; // 取出约束右侧b
        new.cMatrix[i][0] = bNum; // 矩阵第一列存放b
    }
    Number *numTemp = NULL; // 数值结构体临时指针
    for (j = 0; j < new.ofLen; j++) {
        size_t varLen = strlen(ofTerms[j]->variable);
        // strlen返回长度不包括末尾\0，所以要多分配一位
        new.varNames[j] = (char *) calloc(varLen + 1, sizeof(char));
        strcpy(new.varNames[j], ofTerms[j]->variable); // 写入变量名
        numTemp = (Number *) calloc(1, sizeof(Number)); // 新分配一个数字结构体的内存
        *numTemp = ofTerms[j]->coefficient; // 拷贝价值系数
        new.ofCosts[j] = numTemp; // 存入价值系数数组
        numTemp = NULL;
        int unitPart = 0; // 检查一列是否是单位阵的一部分
        double decimalized;
        size_t basicPos = 0; // 变量存入基的哪个位置
        for (i = 0; i < new.basicLen; i++) { // 固定列号j，遍历行i
            numTemp = (Number *) calloc(1, sizeof(Number));
            *numTemp = model->subjectTo[i].left[j]->coefficient; // 取到约束中对应ij的技术系数
            // 这里j+1是因为矩阵第一列存放的是约束右侧的b
            new.cMatrix[i][j + 1] = numTemp; // 存入矩阵
            decimalized = Decimalize(*numTemp);
            if (decimalized == 1) // 记录一列中1出现的位置
                basicPos = i; // 储存可能的变量入基位置
            unitPart += decimalized > 0 ? (int) decimalized : 0; // 将当前列系数中>0的整数化并累加
            numTemp = NULL;
        }
        if (unitPart == 1) {
            // 这一列所有>=0的系数(化为整数)加起来等于1，那么这一列肯定是单位阵的一部分，比如0 0 1，0 1 0这种
            // 变量入基，这里直接把指针给赋给元素了，因为变量名和其对应的价值系数是没有变的
            new.basicVars[basicPos] = new.varNames[j];
            new.basicCosts[basicPos] = new.ofCosts[j];
        }
    }
    *lack = NULL;
    lackPtr = 0;
    for (i = 0; i < new.basicLen; i++) {
        if (new.basicVars[i] == NULL) { // 基未被填满，说明没找到单位阵
            if (*lack == NULL)
                *lack = (size_t *) calloc(new.basicLen, sizeof(size_t));
            *lack[lackPtr++] = i; // 存入需要加入人工变量的行号
            *valid = 0; // 操作失败
        }
    }
    return new;
}

/**
 * 与CreateSMatrix对应，销毁单纯形表矩阵
 * @param matrix 指向单纯形表矩阵的指针（SimplexMatrix）
 */
void RevokeSMatrix(SimplexMatrix *matrix) {
    size_t i, j;
    for (j = 0; j < matrix->ofLen; j++) { // 遍历列
        free(matrix->varNames[j]); // 销毁变量行中每一项
        free(matrix->ofCosts[j]); // 销毁价值系数中每一项
        for (i = 0; i < matrix->basicLen; i++) // 遍历行
            // 销毁矩阵中每一项，注意矩阵多出了一列用于存放b
            free(matrix->cMatrix[i][j + 1]);
    }
    for (i = 0; i < matrix->basicLen; i++) {  // 遍历行
        free(matrix->cMatrix[i][0]); // 销毁每行的首列
        free(matrix->cMatrix[i]); // 销毁矩阵的每行
    }
    free(matrix->ofCosts); // 销毁价值系数数组
    free(matrix->varNames); // 销毁变量名数组
    free(matrix->cMatrix); // 销毁矩阵
    // 基变量及其价值系数数组的每项无需释放，因为是直接引用的ofCosts和varNames中的指针
    free(matrix->basicVars); // 销毁基变量名数组
    free(matrix->basicCosts); // 销毁基变量价值系数数组
    matrix->ofCosts = NULL;
    matrix->varNames = NULL;
    matrix->cMatrix = NULL;
    matrix->basicVars = NULL;
    matrix->basicCosts = NULL;
    matrix->basicLen = 0;
    matrix->ofLen = 0;
}

