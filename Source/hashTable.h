/*
 * 这里放一些hashTable需要用到的结构体
 * - SomeBottle
 */
#include <stdio.h>


#define VAR_HASH_TABLE_LEN 210 /** 变量哈希数组长度*/

/**
 * 变量哈希表的一个项目的结构体
 * @note 关于relation：-2代表<= -1代表< 1代表> 2代表>= 3代表= 0代表置空(无约束unr)
 */
typedef struct var_item { // 变量名字典键值对项目
    char *keyName; // 变量名，同时也是这一项的键
    short int relation; // 关系符号
    int number;
    char formerX[8]; // 变量无约束时会有的x'' (用x''-x'替代无约束变量)
    char latterX[8]; // 变量无约束时会有的x'
    struct var_item *next; // 这里使用链地址法解决哈希碰撞的问题
} VarItem;

/**
 * 哈希表底层数组结构体
 */
typedef struct { // 创建变量约束哈希表
    size_t tableSize; // 哈希表底层数组长度
    VarItem **table;
    // 存放VarItem指针的指针变量，这样做是为了把所有VarItem放在堆内存中，
    // 不然初始化数组大小就得是sizeof(VarItem)*项目数了，这是一笔不菲的内存消耗。
} VarTable;

extern void InitVarDict();

extern VarTable BackupVarDict();

extern void RestoreVarDict(VarTable target);

extern VarItem *CreateVarItem(char *varName, short int relation, int num);

extern short int PutVarItem(VarItem *item);

extern VarItem *GetVarItem(char *key);

extern VarItem **GetVarItems(size_t *len, long int *maxSub, short int *valid);

extern void DelVarDict(VarTable *target);

extern void RevokeCurrDict();