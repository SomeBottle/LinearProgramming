/**
 * 解法路由
 * 在这里让用户选择一种解法
 * SomeBottle 20220523
 */
#include "public.h"
#include "simplex.h"

/**
 * 路由入口
 * @param model 指向LP模型的指针
 */
void Entry(LPModel *model) {
    LPModel standardForm = CopyModel(model); // 复制一份模型，这个模型会被化作标准型备用
    VarTable varDictBak = BackupVarDict(); // 备份变量哈希表
    LPStandardize(&standardForm, 1); // 对复制的模型进行标准化
    LPAlign(&standardForm); // 标准化后对约束进行对齐操作，方便化成矩阵
    // 经过标准化和对齐处理后模型valid位变成了0，说明出现了问题
    if (!standardForm.valid) {
        printf("ERROR: LPModel Standardization failed.\n");
    } else {
        short int cycle = 1;
        char receive = '\0';
        printf("\n-------------------\n> Aligned Standard Form for Simplex  \n\n");
        PrintModel(standardForm);
        PAUSE;
        while (cycle) {
            CLEAR;
            printf("Choose the algorithm:\n");
            printf("\t0. Exit\n"); // 退出
            printf("\t1. Primal Simplex Method\n"); // 原始单纯形法
            printf("\t2. Dual Simplex Method\n"); // 对偶单纯形法
            printf("\t...Under development.\n");
            printf("Type in the number correspond to the option: ");
            receive = getchar();
            switch (receive) {
                case '1':
                    printf("Simplex Method\n");
                    break;
                case '2':
                    printf("Primal Simplex Method\n");
                    break;
                case '0': // 退出程序
                    cycle = 0;
                default:
                    break;
            }
        }
    }
    // 销毁哈希表副本
    DelVarDict(&varDictBak);
    // 销毁模型副本
    FreeModel(&standardForm);
}