//////////////////////////////////////////////////////////////////////////////////
// nvme_main.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//				  Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// This file is part of Cosmos+ OpenSSD.
//
// Cosmos+ OpenSSD is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// Cosmos+ OpenSSD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cosmos+ OpenSSD; see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: ENC Lab. <http://enc.hanyang.ac.kr>
// Engineer: Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//			 Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//			 Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe Main
// File Name: nvme_main.c
//
// Version: v1.2.0
//
// Description:
//   - initializes FTL and NAND
//   - handles NVMe controller
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.2.0
//   - header file for buffer is changed from "ia_lru_buffer.h" to "lru_buffer.h"
//   - Low level scheduler execution is allowed when there is no i/o command
//
// * v1.1.0
//   - DMA status initialization is added
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "xil_printf.h"
#include "debug.h"
#include "io_access.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_main.h"
#include "nvme_admin_cmd.h"
#include "nvme_io_cmd.h"

#include "../memory_map.h"

#include "nmc/nmc_requests.h"
#include "xtime_l.h"
#include "time.h"
#include "cdma/cdma.h"
volatile NVME_CONTEXT g_nvmeTask;

extern FILENAME_BUFFER *nmcMappingFilenameBufPtr;
int count;
int *timer_reg = 0xF8F00200;
int *count_address = 0x2FFFE010;
u32 tbegin_l,tbegin_h,tend_l,tend_h;
int time_flag;
int cdma_flag;
float texe;
void nvme_main()
{
    unsigned int exeLlr;
    unsigned int rstCnt = 0;
    *count_address = 0;

    count = 0;
    InitFTL();
    if (checkChannelInfo())
    {
        pr_error("Channel does not match, please re-insert the nand flash module");
    }
    else{
        xil_printf("\r\nFTL reset complete!!! \r\n");
        xil_printf("Turn on the host PC \r\n");
    }
    
    /**
     * The main loop of Cosmos+ firmware.
     *
     * This loop can be separated into several small parts:
     *
     * - NVMe Manager
     * - Low-level Scheduler
     */
    while (1)
    {
        exeLlr = 1;

        if (g_nvmeTask.status == NVME_TASK_WAIT_CC_EN)
        {
            unsigned int ccEn;
            ccEn = check_nvme_cc_en();
            if (ccEn == 1)
            {
                set_nvme_admin_queue(1, 1, 1);
                set_nvme_csts_rdy(1);
                g_nvmeTask.status = NVME_TASK_RUNNING;
                xil_printf("\r\nNVMe ready!!!\r\n");
            }
        }
        else if (g_nvmeTask.status == NVME_TASK_RUNNING)
        {
            NVME_COMMAND nvmeCmd;
            unsigned int cmdValid;
            
            cmdValid = get_nvme_cmd(&nvmeCmd.qID, &nvmeCmd.cmdSlotTag, &nvmeCmd.cmdSeqNum, nvmeCmd.cmdDword);
            /**
             *  Interpret NVMe commands received from host.
             *
             *  In this step, we need to check which type of the NVMe command you got:
             *
             * - If it's Admin command:
             *
             * 		Handle it in NVMe Manager without forwarding to FTL.
             *
             * - If it's I/O (NVM) command:
             *
             * 		Forward to the NVM Command Manager (FTL).
             */
            if (cmdValid == 1)
            {
                rstCnt = 0;
                if (nvmeCmd.qID == 0)
                {
                    handle_nvme_admin_cmd(&nvmeCmd);
                }
                else
                {
                    time_flag = 0;
                    cdma_flag = 0;
                    handle_nvme_io_cmd(&nvmeCmd);
                    ReqTransSliceToLowLevel();
                    if(time_flag == 1){
                        check_auto_tx_dma_done();
                        tend_l = *(volatile u32 *)(timer_reg);
                        tend_h = *(volatile u32 *)(timer_reg+1);
                        // xil_printf("\r\n tend_L:%u \r\n",tend_l);
                        // xil_printf("\r\n tend_H:%u \r\n",tend_h);
                        // xil_printf("\r\n Cexe_L:%u \r\n",(tend_l - tbegin_l));
                        // xil_printf("\r\n Cexe_H:%u \r\n",(tend_h - tbegin_h));
                    }
                    exeLlr = 0;
                }
            }
        }
        else if (g_nvmeTask.status == NVME_TASK_SHUTDOWN)
        {
            NVME_STATUS_REG nvmeReg;
            nvmeReg.dword = IO_READ32(NVME_STATUS_REG_ADDR);
            if (nvmeReg.ccShn != 0)
            {
                unsigned int qID;
                set_nvme_csts_shst(1);

                for (qID = 0; qID < 8; qID++)
                {
                    set_io_cq(qID, 0, 0, 0, 0, 0, 0);
                    set_io_sq(qID, 0, 0, 0, 0, 0);
                }

                set_nvme_admin_queue(0, 0, 0);
                g_nvmeTask.cacheEn = 0;
                set_nvme_csts_shst(2);
                g_nvmeTask.status = NVME_TASK_WAIT_RESET;

                // flush grown bad block info
                UpdateBadBlockTableForGrownBadBlock(RESERVED_DATA_BUFFER_BASE_ADDR);

                xil_printf("\r\nNVMe shutdown!!!\r\n");
            }
        }
        else if (g_nvmeTask.status == NVME_TASK_WAIT_RESET)
        {
            unsigned int ccEn;
            ccEn = check_nvme_cc_en();
            if (ccEn == 0)
            {
                g_nvmeTask.cacheEn = 0;
                set_nvme_csts_shst(0);
                set_nvme_csts_rdy(0);
                g_nvmeTask.status = NVME_TASK_IDLE;
                xil_printf("\r\nNVMe disable!!!\r\n");
            }
        }
        else if (g_nvmeTask.status == NVME_TASK_RESET)
        {
            unsigned int qID;
            for (qID = 0; qID < 8; qID++)
            {
                set_io_cq(qID, 0, 0, 0, 0, 0, 0);
                set_io_sq(qID, 0, 0, 0, 0, 0);
            }

            if (rstCnt >= 5)
            {
                pcie_async_reset(rstCnt);
                rstCnt = 0;
                xil_printf("\r\nPcie iink disable!!!\r\n");
                xil_printf("Wait few minute or reconnect the PCIe cable\r\n");
            }
            else
                rstCnt++;

            g_nvmeTask.cacheEn = 0;
            set_nvme_admin_queue(0, 0, 0);
            set_nvme_csts_shst(0);
            set_nvme_csts_rdy(0);
            g_nvmeTask.status = NVME_TASK_IDLE;

            xil_printf("\r\nNVMe reset!!!\r\n");
        }

        /**
         * Do scheduling.
         *
         * We need to execute the requests that were put on corresponding queue in prev
         * part.
         *
         * As described in the paper, Host DMA operations have the highest priority, so
         * we should call the `CheckDoneNvmeDmaReq` first, then `SchedulingNandReq`.
         */
        if (exeLlr && ((nvmeDmaReqQ.headReq != REQ_SLOT_TAG_NONE) || notCompletedNandReqCnt || blockedReqCnt))
        {
            CheckDoneNvmeDmaReq();
            if (nvmeDmaReqQ.headReq == REQ_SLOT_TAG_NONE) // wait until DMA finished
                nmcReqScheduling();                       // after receiving filename from host, do inference
            SchedulingNandReq();
        }

        if (cdma_flag == 1)
        {
            check_auto_rx_dma_done();
            tbegin_l = *(volatile u32 *)(timer_reg);
            tbegin_h = *(volatile u32 *)(timer_reg+1);
            cdma(count,1);
            check_cdma_done();
            tend_l = *(volatile u32 *)(timer_reg);
            tend_h = *(volatile u32 *)(timer_reg+1);
            xil_printf("\r\n CDMA Cexe_L:%u \r\n",(tend_l - tbegin_l));
            xil_printf("\r\n CDMA Cexe_H:%u \r\n",(tend_h - tbegin_h));
            tbegin_l = *(volatile u32 *)(timer_reg);
            tbegin_h = *(volatile u32 *)(timer_reg+1);
            cdma(count,0);
            check_cdma_done();
            tend_l = *(volatile u32 *)(timer_reg);
            tend_h = *(volatile u32 *)(timer_reg+1);
            // xil_printf("\r\n tend_L:%u \r\n",tend_l);
            // xil_printf("\r\n tend_H:%u \r\n",tend_h);
            xil_printf("\r\n CDMA Cexe_L:%u \r\n",(tend_l - tbegin_l));
            xil_printf("\r\n CDMA Cexe_H:%u \r\n",(tend_h - tbegin_h));
            cdma_flag = 0;
            count++;
        }
    }
}