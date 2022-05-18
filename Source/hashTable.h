/*
 * 这里放一些hashTable需要用到的结构体
 * - SomeBottle
 */

typedef struct var_item { // 变量名字典键值对项目
    char *keyName; // 变量名，同时也是这一项的键
    short int relation; // 关系符号 -2代表<= -1代表< 1代表> 2代表>= 3代表= 0代表置空
    int number;
    struct var_item *next; // 这里使用链地址法解决哈希碰撞的问题
} VarItem;

extern void InitVarDict();

extern VarItem *CreateVarItem(char *varName, short int relation, int num);

extern unsigned int PutVarItem(VarItem *item);

extern VarItem *GetVarItem(char *key);

extern void DelVarDict();