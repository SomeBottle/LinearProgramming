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
    VarTable varDictBak = BackupVarDict(); // 备份变量哈希表
    LPModel modelCopy;
    short int cycle = 1;
    char receive = '\0';
    while (cycle) {
        CLEAR;
        modelCopy = CopyModel(model); // 复制一份模型，这个模型会被化作标准型备用
        printf("Choose the algorithm:\n");
        printf("\t1. Primal Simplex Method\n"); // 原始单纯形法
        printf("\t2. Dual Simplex Method\n"); // 对偶单纯形法
        printf("\t...Under development.\n");
        printf("\t(Other inputs). Exit\n"); // 退出
        printf("Type in the number correspond to the option: ");
        receive = ReadChar();
        switch (receive) {
            case '1':
                printf("Simplex Method\n");
                newSimplex(model);
                break;
            case '2':
                printf("Primal Simplex Method\n");
                break;
            default: // 退出程序
                cycle = 0;
                break;
        }
        // 销毁模型副本
        FreeModel(&modelCopy);
        if (cycle)
            RestoreVarDict(&varDictBak); // 恢复成之前备份的哈希表
    }
    // 销毁哈希表副本
    DelVarDict(&varDictBak);
}