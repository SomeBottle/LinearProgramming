/**
 * 矩阵(Matrix)操作部分头文件
 * SomeBottle 20220524
 */

#ifndef LPSIMPLEX_MATRIX_H
#define LPSIMPLEX_MATRIX_H

#include <stdio.h>
#include "numOprts.h"

/** 用于单纯形表的矩阵结构体*/
typedef struct {
    size_t ofLen; // 目标函数右边的项目数
    char **varNames; // 变量名数组
    Number **ofCosts; // 存放目标函数右边的价值系数
    Number ***cMatrix; // 技术系数矩阵（第一列对应表中的b，其他列对应约束中的技术系数）
    size_t basicLen; // 基变量数目，这也对应了矩阵的行数
    char **basicVars; // 基变量数组
    Number **basicCosts; // 基变量对应的价值系数
} SimplexMatrix;

extern SimplexMatrix CreateSMatrix(LPModel *model, size_t *lack, short int *valid);

extern void RevokeSMatrix(SimplexMatrix *matrix);

#endif //LPSIMPLEX_MATRIX_H
