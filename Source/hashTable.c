/*
 * 本源文件专门用来存放哈希表部分
 * 不出意外的话主要就变量约束需要用到哈希表了
 * SomeBottle 20220516
 */

#include "public.h"

struct { // 创建变量约束哈希表
    int tableSize; // 哈希表底层数组长度
    VarItem **table;
    // 存放VarItem指针的指针变量，这样做是为了把所有VarItem放在堆内存中，
    // 不然初始化数组大小就得是sizeof(VarItem)*项目数了，这是一笔不菲的内存消耗。
} varDict = {
        .tableSize=VAR_HASH_TABLE_LEN,
        .table=NULL
};

void InitVarDict() { // 初始化变量约束哈希表
    varDict.table = (VarItem **) calloc(VAR_HASH_TABLE_LEN, sizeof(VarItem *));
}

void DelVarDict() { // 销毁变量约束哈希表
    int i;
    for (i = 0; i < varDict.tableSize; i++) {
        VarItem *temp;
        VarItem *temp2;
        temp = varDict.table[i];
        if (temp != NULL) {
            while (temp != NULL) { // 遍历释放
                temp2 = temp;
                temp = temp->next;
                free(temp2->keyName); // 释放变量名
                free(temp2); // 释放链地址节点
            }
        }
    }
    free(varDict.table);
    varDict.table = NULL;
    varDict.tableSize = 0;
}

unsigned int VarHash(char *varName) {
    // 由变量名算出对应哈希，利用折叠法
    int i, temp;
    unsigned int result = 0;
    size_t len = strlen(varName); // 字符串长度
    for (i = 0; i < len; i++) {
        if (isalnum(varName[i])) {
            temp = (int) varName[i]; // 折叠法
            if (i > 0)
                temp -= 48; // 后面两个字符的ASCII可以减去48
            result += temp;
        } else { // 字符串必须全部是字母或者数字
            printf("Variable hashing failed: all of the chars must be Number or Letters in alphabet.\n");
            result = 0;
            break;
        }
    }
    /* 因为规定变量最多三个字符，且必须以字母开头，所以第一个字符ASCII码从65开始
     * 后两个字符ASCII码则是从48开始到122为止（0-z）
     * 为求简便，hash结果中可以减去一部分
     */
    result -= 65;
    return result; // 如果返回的是0肯定就失败了
}

VarItem *CreateVarItem(char *varName, short int relation, int num) {
    VarItem *new = (VarItem *) malloc(sizeof(VarItem)); // 在堆内存中分配一个键值对项目
    new->keyName = (char *) malloc((strlen(varName) + 1) * sizeof(char));
    strcpy(new->keyName, varName); // 复制字符串
    new->relation = relation;
    new->number = num;
    return new;
}

unsigned int PutVarItem(VarItem *item) { // 将键值对项目存入表中
    unsigned int hash = VarHash(item->keyName); // 计算哈希
    if (hash) {
        VarItem *currentNode = varDict.table[hash];
        if (currentNode == NULL) { // 这个哈希对应的链地址还未初始化
            currentNode = (VarItem *) calloc(1, sizeof(VarItem));
            varDict.table[hash] = currentNode; // 初始化链地址
        }
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }
        item->next = NULL;
        currentNode->next = item; // 链地址法解决哈希碰撞，将当前键值对指针存入链表
    } else { // 哈希运算失败
        return 0;
    }
    return 1;
}

VarItem *GetVarItem(char *key) { // 查询对应变量的项目
    unsigned int hash = VarHash(key);
    VarItem *currentNode = varDict.table[hash];
    if (currentNode != NULL) {
        currentNode = currentNode->next; // 从首个节点开始
    } else {
        return NULL;
    }
    while (currentNode != NULL) {
        if (strcmp(currentNode->keyName, key) == 0) { // 找到了
            break;
        } else {
            currentNode = currentNode->next; // 遍历链表
        }
    }
    return currentNode;
}


