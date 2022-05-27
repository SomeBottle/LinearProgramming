/*
 * 本源文件专门用来存放哈希表部分
 * 不出意外的话主要就变量约束需要用到哈希表了
 * SomeBottle 20220516
 */

#include "public.h"

static size_t VarHash(char *varName);

static VarTable CopyVarDict(VarTable *target);

static void DelVarDict(VarTable *target);

/** 变量约束哈希表*/
static VarTable varDict = {
        .tableSize=VAR_HASH_TABLE_LEN,
        .table=NULL,
        .maxSubOfX=0
};

/** 哈希表备份*/
static VarTable varDictCopy = {.table=NULL};

/** 初始化变量约束哈希表 */
void InitVarDict() {
    varDict.table = (VarItem **) calloc(VAR_HASH_TABLE_LEN, sizeof(VarItem *));
}

/**
 * 获得当前x开头变量的最大下标
 * @return 一个size_t类型
 * @note 为了加入松弛变量/剩余变量/人工变量做准备
 */
size_t GetCurrentXSub() {
    return varDict.maxSubOfX;
}

/**
 * 复制哈希表数组
 * @param target 指向待复制哈希表(VarTable)
 * @return 一个VarTable结构体对象
 */
VarTable CopyVarDict(VarTable *target) {
    size_t i;
    // 新分配一个副本
    VarItem **tableCopy = (VarItem **) calloc(target->tableSize, sizeof(VarItem *));
    for (i = 0; i < target->tableSize; i++) {
        VarItem *temp;
        VarItem *newTemp;
        VarItem *prevNewTemp = NULL;
        temp = target->table[i];
        while (temp != NULL) { // 这里存放了链节点，复制整个链表
            // 分配一个新节点
            newTemp = (VarItem *) calloc(1, sizeof(VarItem));
            memcpy(newTemp, temp, sizeof(VarItem)); // 复制VarItem
            newTemp->next = NULL; // 设定下一项为NULL
            if (prevNewTemp != NULL) {
                // 新分配一个变量名
                newTemp->keyName = (char *) calloc(strlen(temp->keyName) + 1, sizeof(char));
                // 复制变量名，放在这里是因为头节点变量名为NULL
                strcpy(newTemp->keyName, temp->keyName);
                prevNewTemp->next = newTemp; // 和上一个节点连接起来
            } else {
                tableCopy[i] = newTemp; // 将头节点存入新哈希表副本
            }
            prevNewTemp = newTemp;
            temp = temp->next; // 遍历链表
        }
    }
    VarTable copyDict = {
            .table=tableCopy,
            .tableSize=target->tableSize,
            .maxSubOfX=target->maxSubOfX
    };
    return copyDict;
}

/**
 * 备份变量哈希表底层数组，本质上是调用static CopyVarDict
 * @note 设计有点借鉴JavaScript里的CanvasRenderingContext2D.save()
 */
void BackupVarDict() {
    if (varDictCopy.table != NULL) // 多次调用此函数时要释放掉上回的哈希表
        DelVarDict(&varDictCopy);
    varDictCopy = CopyVarDict(&varDict);
}

/**
 * 还原BackupVarDict备份的哈希表底层数组
 * @note 设计有点借鉴JavaScript里的CanvasRenderingContext2D.restore()
 */
void RestoreVarDict() {
    DelVarDict(&varDict); // 销毁当前哈希表的内容
    // 还原备份
    varDict = CopyVarDict(&varDictCopy); // 深拷贝一份，与varDictCopy哈希表断开关联
}

/**
 * 获取哈希表中储存的所有项目
 * @param len 指向一个size_t类型变量，用于储存返回数组的长度
 * @param valid 指向一个变量，这个变量存放 1/0 代表 是/否 获取成功
 * @return 指向一个VarItem指针数组的指针
 * @note 一定要记得用完后对结果进行free
 * @note len,valid这两个参数都可以传入NULL
 */
VarItem **GetVarItems(size_t *len, short int *valid) {
    // 获得变量约束哈希表的所有键值对(key)，记得free
    int sizePerAlloc = 5; // 每次分配时增加多少元素
    size_t maxLen = sizePerAlloc; // 返回的指针数组长度
    int ptr = 0, i; // 当前指向的地址
    VarItem **allItems = (VarItem **) calloc(maxLen, sizeof(VarItem *));
    VarItem *currentNode;
    for (i = 0; i < varDict.tableSize; i++) {
        currentNode = varDict.table[i];
        if (currentNode != NULL) { // 这里有数据
            currentNode = currentNode->next; // 从首个节点开始
            while (currentNode != NULL) {
                allItems[ptr++] = currentNode; // 存入结果列表
                if (ptr >= maxLen) { // 结果数组长度不够了，重新分配一下
                    maxLen += sizePerAlloc; // 增加元素
                    allItems = (VarItem **) realloc(allItems, sizeof(VarItem *) * maxLen);
                    if (allItems != NULL) {
                        memset(allItems + ptr, 0, sizePerAlloc * sizeof(VarItem *)); // 清空新分配的内存
                    } else { // 内存重分配失败
                        if (valid != NULL)
                            *valid = 0;
                        printf("Memory reallocation failed when getting VarItems.\n");
                    }
                }
                currentNode = currentNode->next;
            }
        }
    }
    if (len != NULL)
        *len = ptr;
    return allItems; // 记得free
}

/**
 * 销毁所有变量哈希表，包括备份
 * @note 本质还是调用DelVarDict，不过操作对象是静态对象varDict和varDictCopy
 */
void RevokeAllDict() {
    DelVarDict(&varDict); // 销毁刚用完的哈希表
    DelVarDict(&varDictCopy); // 销毁哈希表备份
}

/** 销毁变量哈希表
 * @param target 指向目标哈希表的指针(VarTable结构体)
 * @note 这个也可以用于删除BackupVarDict创建的备份
 */
void DelVarDict(VarTable *target) {
    size_t i;
    // 如果该哈希表已被销毁就不作操作
    if (target->table == NULL) return;
    for (i = 0; i < target->tableSize; i++) {
        VarItem *temp;
        VarItem *temp2;
        temp = target->table[i];
        while (temp != NULL) { // 遍历释放
            temp2 = temp;
            temp = temp->next;
            free(temp2->keyName); // 释放变量名
            free(temp2); // 释放链地址节点
        }
    }
    free(target->table);
    target->table = NULL;
    target->tableSize = 0;
    target->maxSubOfX = 0;
}

/**
 * 由变量名(字符串)算出对应哈希，利用折叠法
 * @param varName 待运算字符串
 * @return 一个无符号整数，作为哈希值
 */
size_t VarHash(char *varName) {
    size_t i, temp;
    size_t len = strlen(varName); // 字符串长度
    if (len <= 0) { // 常数项一般会触发这里
        return 0;
    }
    if (!ValidVar(varName)) { // 不是合法的变量名
        printf("Var hashing failed: invalid variable name.\n");
        return 0;
    }
    size_t result = 0;
    for (i = 0; i < len; i++) {
        temp = (int) varName[i]; // 折叠法
        if (i > 0)
            temp -= 48; // 后面字符的ASCII可以减去48
        result += temp;
    }
    /* 因为规定变量最多三个字符，且必须以字母开头，所以第一个字符ASCII码从65开始
     * 后面的字符ASCII码则是从48开始到122为止（0-z）
     * 为求简便，hash结果中可以减去一部分
     */
    result -= 65;
    return result > 0 ? result : 0; // 如果返回的是0肯定就失败了
}

/**
 * 创建一个哈希表项目
 * @param varName 变量名（字符串）
 * @param relation 关系代号
 * @param num 关系式右边数值
 * @return 一个指向新分配的哈希表项目VarItem的指针
 * @note 关系代号-2代表<= -1代表< 1代表> 2代表>= 3代表= 0代表置空(无约束)
 */
VarItem *CreateVarItem(char *varName, short int relation, int num) {
    VarItem *new = (VarItem *) calloc(1, sizeof(VarItem)); // 在堆内存中分配一个键值对项目
    new->keyName = (char *) malloc((strlen(varName) + 1) * sizeof(char));
    strcpy(new->keyName, varName); // 复制字符串
    new->relation = relation;
    new->number = num;
    return new;
}

/**
 * 将哈希表项目存入表中
 * @param item 指向哈希表项目VarItem的指针
 * @return 1/0 代表 是/否 存入成功
 */
short int PutVarItem(VarItem *item) { // 将键值对项目存入表中
    size_t hash = VarHash(item->keyName); // 计算哈希
    if (hash) {
        // 获得这一项x的下标，比如x67，这里就是67
        size_t currentSub = strtoull(item->keyName + 1, NULL, 10);
        // 如果比已有maxSubOfX大，就更新x的最大下标
        if (item->keyName[0] == 'x' && currentSub > varDict.maxSubOfX)
            varDict.maxSubOfX = currentSub;
        if (hash >= varDict.tableSize) { // 算出的哈希大于分配的哈希数组长度，不够放了
            // 重新进行分配
            size_t newSize = hash + 1; // 新的数组大小
            size_t sizeDiff = newSize - varDict.tableSize; // 新分配了多少个元素
            varDict.table = (VarItem **) realloc(varDict.table, sizeof(VarItem *) * newSize);
            if (varDict.table != NULL) {
                // 清空新分配的内存部分
                memset(varDict.table + varDict.tableSize, 0, sizeof(VarItem *) * sizeDiff);
                // 重标记数组大小
                varDict.tableSize = newSize;
            } else { // 重分配失败
                printf("Memory reallocation failed when putting VarItem");
                return 0;
            }
        }

        // 开始装入
        VarItem *currentNode = varDict.table[hash];
        if (currentNode == NULL) { // 这个哈希对应的链地址还未初始化
            currentNode = (VarItem *) calloc(1, sizeof(VarItem));
            varDict.table[hash] = currentNode; // 初始化链地址头节点
        }
        item->next = NULL;
        while (currentNode->next != NULL) {
            if (strcmp(item->keyName, currentNode->next->keyName) == 0) { // 如果表中已经存放了这个键值对
                item->next = currentNode->next->next; // 在对应位置插入节点
                free(currentNode->next->keyName); // 别忘了keyName也是分配的堆内存！
                free(currentNode->next); // 替换掉原来的键值对，这里就把原来的项目释放了
                break;
            } else { // 否则一直寻找直至下一个节点为NULL
                currentNode = currentNode->next;
            }
        }
        currentNode->next = item; // 链地址法解决哈希碰撞，将当前键值对指针存入链表
    } else { // 哈希运算失败
        return 0;
    }
    return 1;
}

/**
 * 根据变量名查询哈希表项目
 * @param key 待查询变量名
 * @return 指向哈希表项目的指针
 */
VarItem *GetVarItem(char *key) { // 查询对应变量的项目
    size_t hash = VarHash(key);
    if (!hash)
        return NULL; // 哈希运算失败，这种情况可能是遇到常数项了
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


