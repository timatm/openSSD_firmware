//#include <stdio.h>
//#include "platform.h"
#include "xil_printf.h"
#include "pl_bram_plus1.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xaxicdma.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "cdma.h"

#define PL_BRAM_BASE XPAR_PL_BRAM_PLUS1_0_S00_AXI_BASEADDR //PL_RAM_RD

#define PL_BRAM_START PL_BRAM_PLUS1_S00_AXI_SLV_REG0_OFFSET //RAM
#define PL_BRAM_START_ADDR PL_BRAM_PLUS1_S00_AXI_SLV_REG1_OFFSET //RAM
#define PL_BRAM_LEN PL_BRAM_PLUS1_S00_AXI_SLV_REG2_OFFSET //PL
#define START_ADDR 0 //RAM
#define BRAM_DATA_BYTE 4 //BRAM

XAxiCdma_Config *axi_cdma_cfg;
XAxiCdma axi_cdma;


static int SourceAddr  = 0x31000000;
int* DestAddr    = ((int *) RECADDR);

void check_cdma_done(){
    int Status;
    axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
    if (!axi_cdma_cfg) {
        printf("AXAxiCdma_LookupConfig failed\n\r");
    }
    while (XAxiCdma_IsBusy(&axi_cdma));
}

int cdma(int counter,int enable)
{
//    init_platform();

    // print("Initial start......\n\r");



    int Status;

    int *SrcPtr;
    int *DestPtr;

    SrcPtr = (int*)SourceAddr;
    DestPtr = (int*)DestAddr;

    int *Doutaddr = SrcPtr + counter*16384;
    int *Binaddr = (int *) BRAMADDR;
    int *Boutaddr = ((int *) BRAMADDR) + counter*16384;
    int *Dinaddr = *DestAddr;

    if (enable == 1){
        // Set up the AXI CDMA
        // DRAM --> BRAM
        printf("--Set up the AXI CDMA from DDR to %x\t\n\r",&Boutaddr[0]);
        axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
        if (!axi_cdma_cfg) {
            printf("AXAxiCdma_LookupConfig failed\n\r");
        }

        Status = XAxiCdma_CfgInitialize(&axi_cdma, axi_cdma_cfg, axi_cdma_cfg->BaseAddress);
        if (Status == XST_SUCCESS ){
            // printf("XAxiCdma_CfgInitialize succeed\n\r");
        }
        // printf("--Disable Interrupt of AXI CDMA\n\r");
        XAxiCdma_IntrDisable(&axi_cdma, XAXICDMA_XR_IRQ_ALL_MASK);

        if (XAxiCdma_IsBusy(&axi_cdma)) {
            printf("AXI CDMA is busy...\n\r");
            while (XAxiCdma_IsBusy(&axi_cdma));
        }


        Xil_DCacheFlush();

        Status = XAxiCdma_SimpleTransfer(
                                        &axi_cdma,
                                        (u32) Doutaddr,
                                        (u32) Boutaddr,
                                        INPUT_SIZE,
                                        NULL,
                                        NULL);

        Xil_DCacheFlush();
        // printf("--transaction from DDR to %x\t is done\n\r",&Boutaddr[0]);
    }
    else {
        // Set up the AXI CDMA
        // BRAM --> DRAM
        // printf("--Set up the AXI CDMA from %x\t to DDR\n\r",&Boutaddr[0]);
        axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
        if (!axi_cdma_cfg) {
            printf("AXAxiCdma_LookupConfig failed\n\r");
        }

        Status = XAxiCdma_CfgInitialize(&axi_cdma, axi_cdma_cfg, axi_cdma_cfg->BaseAddress);
        if (Status == XST_SUCCESS ){
            // printf("XAxiCdma_CfgInitialize succeed\n\r");
        }
        // printf("--Disable Interrupt of AXI CDMA\n\r");
        XAxiCdma_IntrDisable(&axi_cdma, XAXICDMA_XR_IRQ_ALL_MASK);

        if (XAxiCdma_IsBusy(&axi_cdma)) {
        printf("AXI CDMA is busy...\n\r");
        while (XAxiCdma_IsBusy(&axi_cdma));
        }


        Xil_DCacheFlush();

        Status = XAxiCdma_SimpleTransfer(
                                            &axi_cdma,
                                            (u32) Boutaddr,
                                            (u32) Dinaddr,
                                            OUTPUT_SIZE,
                                            NULL,
                                            NULL);
        
        *DestAddr = *DestAddr + OUTPUT_SIZE;
        Xil_DCacheFlush();

        // printf("--transaction from %x\t to DDR is done\n\r",&Boutaddr[0]);
        // pr_info("status : %d",Status);
    }



    // printf("transaction from BRAM0 to DDR is done\n\r");

    // for(i=0;i<1024;i++){
    // printf("DRAM address at %x ,Oringinal data : %d\t ,After : %d\n\r",&rx_buffer1[i],i,rx_buffer1[i]);
    // }

    //cleanup_platform();
    return 0;
}

int cdma_write(int counter,int *Binaddr,int *Doutaddr,int dataSize){
    int Status;
    // Set up the AXI CDMA
    // DRAM --> BRAM
    printf("--Set up the AXI CDMA from DDR to %x\t\n\r",&Binaddr[0]);
    axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
    if (!axi_cdma_cfg) {
        printf("AXAxiCdma_LookupConfig failed\n\r");
    }

    Status = XAxiCdma_CfgInitialize(&axi_cdma, axi_cdma_cfg, axi_cdma_cfg->BaseAddress);
    if (Status == XST_SUCCESS ){
        printf("XAxiCdma_CfgInitialize succeed\n\r");
    }
    printf("--Disable Interrupt of AXI CDMA\n\r");
    XAxiCdma_IntrDisable(&axi_cdma, XAXICDMA_XR_IRQ_ALL_MASK);

    if (XAxiCdma_IsBusy(&axi_cdma)) {
        printf("AXI CDMA is busy...\n\r");
        while (XAxiCdma_IsBusy(&axi_cdma));
    }


    Xil_DCacheFlush();

    Status = XAxiCdma_SimpleTransfer(
                                    &axi_cdma,
                                    (u32) Doutaddr,
                                    (u32) Binaddr,
                                    dataSize,
                                    NULL,
                                    NULL);

    Xil_DCacheFlush();
    printf("--transaction from DDR to %x\t is done\n\r",&Binaddr[0]);

    // CIP active
    PL_BRAM_PLUS1_mWriteReg(PL_BRAM_BASE, PL_BRAM_LEN , BRAM_DATA_BYTE*20) ;
    PL_BRAM_PLUS1_mWriteReg(PL_BRAM_BASE, PL_BRAM_START_ADDR, 0x50000000) ;
    PL_BRAM_PLUS1_mWriteReg(PL_BRAM_BASE, PL_BRAM_START , 1) ;
    PL_BRAM_PLUS1_mWriteReg(PL_BRAM_BASE, PL_BRAM_START , 0) ;
}

int cdma_read(int counter,int *Boutaddr,int *Dinaddr,int dataSize){
    int Status;
    // Set up the AXI CDMA
    // BRAM --> DRAM
    printf("--Set up the AXI CDMA from %x\t to DDR\n\r",&Boutaddr[0]);
    axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
    if (!axi_cdma_cfg) {
        printf("AXAxiCdma_LookupConfig failed\n\r");
    }

    Status = XAxiCdma_CfgInitialize(&axi_cdma, axi_cdma_cfg, axi_cdma_cfg->BaseAddress);
    if (Status == XST_SUCCESS ){
        printf("XAxiCdma_CfgInitialize succeed\n\r");
    }
    printf("--Disable Interrupt of AXI CDMA\n\r");
    XAxiCdma_IntrDisable(&axi_cdma, XAXICDMA_XR_IRQ_ALL_MASK);

    if (XAxiCdma_IsBusy(&axi_cdma)) {
    printf("AXI CDMA is busy...\n\r");
    while (XAxiCdma_IsBusy(&axi_cdma));
    }


    Xil_DCacheFlush();

    Status = XAxiCdma_SimpleTransfer(
                                        &axi_cdma,
                                        (u32) Boutaddr,
                                        (u32) Dinaddr,
                                        dataSize,
                                        NULL,
                                        NULL);
    
    Xil_DCacheFlush();

    printf("--transaction from %x\t to DDR is done\n\r",&Boutaddr[0]);
    printf("status : %d",Status);
}
