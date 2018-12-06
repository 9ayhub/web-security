#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

/*-----------------------------------------*/
// 轮函数，迭代函数

#define F(b, c, d) ((b & c) | (~b & d))
#define G(b, c, d) ((b & d) | (c & ~d))
#define H(b, c, d) (b ^ c ^ d)
#define I(b, c, d) (c ^ (b | ~d))
#define CLS(a, s) ((a << s) | (a >> (32 - s)))
#define iteration(a, b, c, d, g, x, t, s) {\
    switch(g) {\
        case 0:\
            a += F(b, c, d) + x + t;\
            break;\
        case 1:\
            a += G(b, c, d) + x + t;\
            break;\
        case 2:\
            a += H(b, c, d) + x + t;\
            break;\
        case 3:\
            a += I(b, c, d) + x + t;\
            break;\
    }\
    a = CLS(a, s);\
    a += b;\
}

/*-----------------------------------------*/
// 表

int x[64] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    1, 6,11, 0, 5,10,15, 4, 9,14, 3, 8,13, 2, 7,12,
    5, 8,11,14, 1, 4, 7,10,13, 0, 3, 6, 9,12,15, 2,
    0, 7,14, 5,12, 3,10, 1, 8,15, 6,13, 4,11, 2, 9
};

unsigned int T[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

int s[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

/*-----------------------------------------*/
// 工具函数

// 通过读取文件信息获得文件大小
int get_file_size(char* filename)
{
    struct stat statbuf;
    stat(filename, &statbuf);
    int size = statbuf.st_size;
    return size;
}

// 字符ascii码转8位二进制，写入Y
void dec_to_bi(int c, int* Y, int j) {
    int r = 0, temp[8] = {0}, i = 0;
    do {
        r = c % 2;
        c /= 2;
        temp[i++] = r;
    } while(c >= 1);
    for(i = 0; i < 8; i++, j++) {
        Y[j] = temp[7 - i];
    }
}

// 十进制转64位二进制，用于填充，返回长度位数（8的倍数）
int dec_to_bi_64(int K, int* len_64) {
    int r = 0, i = 0, temp[64] = {0};
    do {
        temp[i++] = K % 2;
        K /= 2;
    } while(K >= 1);
    int j = i % 8 != 0 ? j = 8 - i % 8 : 0;
    for(; i > 0; i--, j++) {
        len_64[j] = temp[i - 1];
    }
    return j;
}

// 512 bits分组转为32 bits一块的X(注意大端小端转换)
void Y_to_X(int* Y, unsigned int* X) {
    int i = 0;
    for(; i < 16; i++) {
        unsigned char temp[4] = {0};
        for(int j = 0; j < 4; j++) {
            for(int k = 0; k < 8; k++) {
                temp[j] += Y[i * 32 + j * 8 + k] * pow(2, 7 - k);
            }
            X[i] += (unsigned int)temp[j] << j * 8;
        }
    }
}

// 打印结果，要进行大小端转换
void final(unsigned int* MD, unsigned char* digit) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            digit[i * 4 + j] = MD[i] >> j * 8;
        }
    }
}

/*-----------------------------------------*/
// 压缩函数

unsigned int* H_MD5(unsigned int* MD, unsigned int* X) {
    unsigned int CV[4] = {MD[0], MD[1], MD[2], MD[3]};
    //4轮16次循环
    for(int k = 0; k < 4; k++) {
        for(int i = 0; i < 16; i++) {
            iteration(MD[0], MD[1], MD[2], MD[3], k, X[x[16 * k + i]], T[16 * k + i], s[16 * k + i]);
            // (A, B, C, D) --> (D, A, B, C) 
            unsigned int temp = MD[3];
            for(int j = 3; j > 0; j--) {
                MD[j] = MD[j - 1];
            }
            MD[0] = temp;
        }
    }
    for(int i = 0; i < 4; i++) {
        MD[i] += CV[i];
    }
    return MD;
}

/*-----------------------------------------*/

int main() {
    /*****************
        1.填充 
    ******************/
    int K = get_file_size("source.txt") * 8; // 原数据长度为K bits
    printf("\nfile size: %d bits\n", K);
    // 填充长度为P bits的标识100...0，1≤P≤512
    int P = 1;
    if((K + P) % 512 != 448 % 512) {
        P = (448 - K % 512 + 513) % 513;
    }
    // 尾部附加K值得低64位（即K mod 2^64）
    int len_64[64] = {0};
    int width = dec_to_bi_64(K, len_64);
    //从文件中获取输入
    FILE *fpin;
    if((fpin = fopen("source.txt", "r")) == NULL) {
        printf("source file can't open!\n");
        exit(0);
    }

    /*****************
        2.分块
    ******************/
    // 分割为L个512-bit分组：Y0,Y1,...,YL-1。
    int L = (K + P + 64) / 512;

    /*****************
        3.初始化
    ******************/
    // 初始化一个128-bit 的 MD 缓冲区，记为CVq，表示成4个32-bit寄存器 (A, B, C, D)
    unsigned int MD[4]; 
    // 置16进制初值作为初始向量IV，并采用小端存储 (little-endian)
    MD[0] = 0x67452301;
    MD[1] = 0xEFCDAB89;
    MD[2] = 0x98BADCFE;
    MD[3] = 0x10325476;
    
    /*****************
        4.循环压缩
    ******************/
    // 以512-bit 消息分组为单位，每一分组 Yq (q = 0, 1, …, L-1) 经过4个循环的压缩算法，表示为：
    // CV0 = IV
    // CVi = HMD5(CVi-1，Yi)
    for(int i = 0; i < L; i++) {
        // 真正开始填充
        int Y[512] = {0};
        char c;
        int j = 0;
        while(j < 512 && (c = fgetc(fpin)) != EOF) {
            dec_to_bi(c, Y, j);
            j += 8;
        }
        // 注意，填充100...000不一定是在最后一组！
        if(j != 512 && j != 0 || j == 0 && P + 64 == 512) {
            Y[j] = 1;
        }
        if(i == L - 1) {
            for(int k = 0; k < 64; k++) {
                Y[512 - 64 + k] = len_64[k];
            }
        }
        // Y转unsigned int X[16]
        unsigned int X[16] = {0};
        Y_to_X(Y, X);
        // 大小端处理不是单纯指12345678->78563412,也不是指12345678->56781234，要根据具体数字进行转换。
        // 这里长度是一个整体，比如0x08600000，转换后应为00000860
        if(i == L - 1) {
            unsigned int temp = 0;
            for(int j = 0; j < 32; j++) {
                temp += len_64[j] * pow(2, 31 - j);
            }
            X[14] = temp >> 32 - width;
        }
        // 开始执行压缩函数
        H_MD5(MD, X);
    }

    /*****************
        5.显示结果
    ******************/
    printf("\n======================\n密码：");
    unsigned char digit[16];
    final(MD, digit);
    for(int i = 0; i < 16; i++) {
        printf("%x", digit[i]);
    }
    printf("\n\n");
    
    fclose(fpin);
    return 0;
}