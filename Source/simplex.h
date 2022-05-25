/**
 * simplex单纯形法部分头文件
 * 通常被router.c引用
 */
// Simplex Funcs below:
#ifndef SIMPLEX_H
#define SIMPLEX_H

extern void LPStandardize(LPModel *model, short int dual);

extern void LPAlign(LPModel *model);

extern void newSimplex(LPModel *model);

#endif