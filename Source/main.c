#include "public.h"

extern LPModel Parser(FILE *fp); // 外部变量，定义于dataParser

int main(int args, char *argv[]) {
    int i;
    FILE *fileStream; // 定义文件指针（类型定义来自stdio）
    if (argv[1] != NULL) { // 看看命令行有没有传入文件名
        printf("Input file: %s\n", argv[1]);
        Number testNum = Fractionize("1.345");
        fileStream = fopen(argv[1], "r"); // 尝试开启文件流
        if (fileStream != NULL) {
            // 读取并解析线性模型为结构体，用完后记得释放掉分配的内存，这是个好习惯！
            LPModel parsed = Parser(fileStream);
            // 释放常量constants指针数组的堆内存
            printf("Freed CONSTANTS: ");
            for (i = 0; i < constantsNum; i++)
                printf("%c ", constants[i]);
            printf("\n");
            free(constants);
            freeModel(&parsed); // 释放LP模型分配的内存
            constants = NULL;
        } else { // 打开文件失败
            printf("File not readable or does not exist.\n");
        }
        fclose(fileStream);
    } else {
        printf("Lacking of argument.\nUsage: %s <filename>\n", argv[0]);
    }
    system("pause");
    return 0;
}
