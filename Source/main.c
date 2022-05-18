#include "public.h"


int main(int args, char *argv[]) {
    int i;
    FILE *fileStream; // 定义文件指针（类型定义来自stdio）
    if (argv[1] != NULL) { // 看看命令行有没有传入文件名
        printf("Input file: %s\n", argv[1]);
        fileStream = fopen(argv[1], "r"); // 尝试开启文件流
        if (fileStream != NULL) {
            double timeCost;
            clock_t startTime, endTime;
            startTime = clock();
            // 初始化变量约束哈希表
            InitVarDict();
            // 读取并解析线性模型为结构体，用完后记得释放掉分配的内存，这是个好习惯！
            LPModel parsed = Parser(fileStream);
            // 移项处理并且合并同类项，同时将变量取值范围记入哈希表
            ConstraintsTrans(&parsed);
            endTime = clock();
            timeCost = ((double) (endTime - startTime)) / CLOCKS_PER_SEC;
            printf("LPModel Parsed in %.9fs\n\n", timeCost);
            if (parsed.valid) { // LP模型有效
                PrintModel(parsed);
            }
            // 释放常量constants指针数组的堆内存
            printf("\nFreed CONSTANTS: ");
            for (i = 0; i < constantsNum; i++)
                printf("%c ", constants[i].name);
            printf("\n");
            free(constants); // 释放常量数组
            FreeModel(&parsed); // 释放LP模型分配的内存
            DelVarDict(); // 释放哈希表
            constants = NULL;
        } else { // 打开文件失败
            printf("File not readable or does not exist.\n");
        }
        fclose(fileStream);
    } else {
        printf("Missing argument. \nUsage: \n\t%s <filename>\n\n", argv[0]);
    }
    PAUSE;
    return 0;
}
