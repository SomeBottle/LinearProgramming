#include "public.h"

extern LPModel Parser(FILE *fp); // 外部变量，定义于dataParser

int main(int args, char *argv[]) {
    FILE *fileStream; // 定义文件指针（类型定义来自stdio）
    if (argv[1] != NULL) { // 看看命令行有没有传入文件名
        printf("Input file: %s\n", argv[1]);
        fileStream = fopen(argv[1], "r"); // 尝试开启文件流
        if (fileStream != NULL) {
            Parser(fileStream);
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
