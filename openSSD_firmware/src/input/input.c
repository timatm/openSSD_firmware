#include "input.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void bufferinit(){
    int *address = partial_buffer_address;
    for(int i = 0;i < data_buffer_size;i++){
        *address = 0x0;
        address++;
    }
    address = dat_adrress;
    for(int i = 0;i < data_buffer_size;i++){
        *address = 0x0;
        address++;
    }
}
void read_dat(){
    FILE *fp;
    int *address = dat_adrress;
    int data;
    // C:\\Users\\ttt\\openssd\\vm\\test\\openSSD-firmware\\src\\input\\input
    char filename[] = "C:/input.dat";
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error opening file in input.c.\n");
    }
    while (fread(&data, sizeof(int), 1, fp) == 1) {
        *(address) = data;
        address++;
    }
    // 关闭文件
    fclose(fp);
}

