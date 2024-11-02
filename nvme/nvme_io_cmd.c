//////////////////////////////////////////////////////////////////////////////////
// nvme_io_cmd.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
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
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe IO Command Handler
// File Name: nvme_io_cmd.c
//
// Version: v1.0.1
//
// Description:
//   - handles NVMe IO command
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.1
//   - header file for buffer is changed from "ia_lru_buffer.h" to "lru_buffer.h"
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "xil_printf.h"
#include "debug.h"
#include "io_access.h"
#include "monitor/monitor.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_io_cmd.h"
#include "data_buffer.h"

#include "../ftl_config.h"
#include "../request_transform.h"
#include "nmc/nmc_mapping.h"
#include "nmc/nmc_requests.h"
extern P_PARTIAL_DATA_MAP dataPartialResult;
extern P_SPECIAL_DATA_HEADER specialDataHeader;
int nvme_complete_flag;
/**
 * @brief The entry function for translating the given NVMe command into slice requests.
 *
 * @note This function only extract some information of the given NVMe command before
 * spliting the NVMe command into slice requests.
 *
 * @param cmdSlotTag @todo the entry index of the given NVMe command.
 * @param nvmeIOCmd a pointer points to the instance of given NVMe command.
 */
void handle_nvme_io_read(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
    IO_READ_COMMAND_DW12 readInfo12;
    // IO_READ_COMMAND_DW13 readInfo13;
    // IO_READ_COMMAND_DW15 readInfo15;
    unsigned int startLba[2];
    unsigned int nlb;

    readInfo12.dword = nvmeIOCmd->dword[12];
    // readInfo13.dword = nvmeIOCmd->dword[13];
    // readInfo15.dword = nvmeIOCmd->dword[15];

    startLba[0] = nvmeIOCmd->dword[10];
    startLba[1] = nvmeIOCmd->dword[11];
    nlb         = readInfo12.NLB;

    // ignore capacity check for read physical
    if (nvmeIOCmd->OPC != IO_NVM_READ_PHY)
        ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
    // ASSERT(nlb < MAX_NUM_OF_NLB);
    ASSERT((nvmeIOCmd->PRP1[0] & 0x3) == 0 && (nvmeIOCmd->PRP2[0] & 0x3) == 0); // error
    ASSERT(nvmeIOCmd->PRP1[1] < 0x10000 && nvmeIOCmd->PRP2[1] < 0x10000);

    switch (nvmeIOCmd->OPC)
    {
    case IO_NVM_NMC_INFERENCE_READ:
        pr_debug("IO Inference Read in handle_nvme_io_read");
    case IO_NVM_GET_MAPPING_TABLE:
        pr_info("IO_NVM_GET_MAPPING_TABLE in handle_nvme_io_read()");
        ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, nvmeIOCmd->OPC);
        break;
    
    case IO_NVM_READ_PHY:
    case IO_NVM_READ:
        ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, nvmeIOCmd->OPC);
        break;

    default:
        pr_error("Unexpected NVMe opcode: %u", nvmeIOCmd->OPC);
        break;
    }
}

/**
 * Entry point for NVM write commands.
 */
void handle_nvme_io_write(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
    IO_READ_COMMAND_DW12 writeInfo12;
    // IO_READ_COMMAND_DW13 writeInfo13;
    // IO_READ_COMMAND_DW15 writeInfo15;
    unsigned int startLba[2];
    unsigned int nlb;

    writeInfo12.dword = nvmeIOCmd->dword[12];
    // writeInfo13.dword = nvmeIOCmd->dword[13];
    // writeInfo15.dword = nvmeIOCmd->dword[15];

    // if(writeInfo12.FUA == 1)
    //	xil_printf("write FUA\r\n");

    startLba[0] = nvmeIOCmd->dword[10];
    startLba[1] = nvmeIOCmd->dword[11];
    nlb         = writeInfo12.NLB;

    // for IO_NVM_NMC_ALLOC, cdw11 is used for filetype, don't check capacity
    if (nvmeIOCmd->OPC != IO_NVM_NMC_ALLOC)
        ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
    // ASSERT(nlb < MAX_NUM_OF_NLB);
    ASSERT((nvmeIOCmd->PRP1[0] & 0xF) == 0 && (nvmeIOCmd->PRP2[0] & 0xF) == 0);
    ASSERT(nvmeIOCmd->PRP1[1] < 0x10000 && nvmeIOCmd->PRP2[1] < 0x10000);

    switch (nvmeIOCmd->OPC)
    {
    case IO_NVM_NMC_INFERENCE:
        pr_debug("NMC Inference Command (nvmeCmdSlotTag = %u)", cmdSlotTag);
    case IO_NVM_WRITE_BUFFER:
        pr_info("IO_NVM_WRITE_BUFFER in handle_nvme_io_write");
    case IO_NVM_NMC_VERIFY_IMG:
        pr_info("IO_NVM_NMC_VERIFY_IMG in handle_nvme_io_write");
    case IO_NVM_NMC_WRITE:
    case IO_NVM_NMC_ALLOC:
    case IO_NVM_WRITE_PHY:
    case IO_NVM_WRITE:
        ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, nvmeIOCmd->OPC);
        break;

    default:
        pr_error("Unexpected NVMe opcode: %u", nvmeIOCmd->OPC);
        break;
    }
}

extern int* timer_reg;
extern u32 tbegin_l,tbegin_h;
extern int time_flag;
extern int cdma_flag;
extern int* count_address;

void handle_nvme_io_cmd(NVME_COMMAND *nvmeCmd)
{
    
    pr_info("nvmeCmd->cmdSlotTag: %d",nvmeCmd->cmdSlotTag);
    NVME_IO_COMMAND *nvmeIOCmd;
    NVME_COMPLETION nvmeCPL;
    unsigned int opc;

#if (NMC_SHORT_FILENAME == true)
    char filename[NMC_FILENAME_MAX_BYTES + 1] = {0};
#endif
    uint32_t filetype, nblks;

    nvmeIOCmd = (NVME_IO_COMMAND *)nvmeCmd->cmdDword;
    /* xil_printf("OPC = 0x%X\r\n", nvmeIOCmd->OPC);
        xil_printf("PRP1[63:32] = 0x%X, PRP1[31:0] = 0x%X\r\n", nvmeIOCmd->PRP1[1],
       nvmeIOCmd->PRP1[0]); xil_printf("PRP2[63:32] = 0x%X, PRP2[31:0] = 0x%X\r\n", nvmeIOCmd->PRP2[1],
       nvmeIOCmd->PRP2[0]); xil_printf("dword10 = 0x%X\r\n", nvmeIOCmd->dword10); xil_printf("dword11 = 0x%X\r\n",
       nvmeIOCmd->dword11); xil_printf("dword12 = 0x%X\r\n", nvmeIOCmd->dword12);*/
    opc = (unsigned int)nvmeIOCmd->OPC;

    switch (opc)
    {
    case IO_NVM_FLUSH:
    {
        monitor_dump_data_buffer_info(MONITOR_MODE_DUMP_DIRTY, 0, 0);
        pr_debug("IO Flush Command");
        FlushDataBuf(nvmeCmd->cmdSlotTag);
        monitor_dump_data_buffer_info(MONITOR_MODE_DUMP_DIRTY, 0, 0);

        nvmeCPL.dword[0] = 0;
        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        break;
    }
    case IO_NVM_WRITE_BUFFER:
        cdma_flag = 1;
        pr_info("IO_NVM_WRITE_BUFFER in handle_nvme_io_cmd");
    case IO_NVM_NMC_VERIFY_IMG:
        pr_info("IO_NVM_NMC_VERIFY_IMG in handle_nvme_io_cmd");
    case IO_NVM_WRITE:
    case IO_NVM_WRITE_PHY:
    case IO_NVM_NMC_WRITE:
    case IO_NVM_NMC_INFERENCE:
    
    {
        pr_debug("IO Write Command");
        handle_nvme_io_write(nvmeCmd->cmdSlotTag, nvmeIOCmd);
        break;
    }
    case IO_NVM_NMC_INFERENCE_READ:
    {
        time_flag = 1;
        //partial data buffer is empty
        if( dataPartialResult->partial_dataBuf[0].transmit_data_address == dataPartialResult->partial_dataBuf[0].receive_data_address ){

            pr_debug("Partial data buffer is empty");
            nvmeCPL.statusField.SC  = SC_VENDOR_PARTIAL_BUFFER_EMPTY;
            nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
            nvmeCPL.specific = 0x0;
            check_auto_tx_dma_done();
            set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        }
        else{
            pr_debug("IO get partial result");
            handle_nvme_io_read(nvmeCmd->cmdSlotTag, nvmeIOCmd);
        }
        break;
    }
    case IO_NVM_GET_MAPPING_TABLE:
    {
        uint32_t iCH = nvmeIOCmd->dword10;
        pr_debug("iCH : %d",iCH);
        // copyMappingTable();
        pr_debug("IO_NVM_GET_MAPPING_TABLE in handle_nvme_io_cmd");
        copyMappingTable(iCH);
        handle_nvme_io_read(nvmeCmd->cmdSlotTag, nvmeIOCmd);
        break;
    }
    case IO_NVM_FIRMWARE_READ_MAPPING:
    {
        int err = 0;
        pr_info("IO_NVM_FIRMWARE_READ_MAPPING");
        err = nmcReadMappingTabbleInfo();
        if (err == 1)
        {
            nvmeCPL.statusField.SC  = SC_VENDOR_GET_MAPPING_TABLE_FAID;
            nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
            nvmeCPL.specific = 0x0;
            pr_debug("IO_NVM_FIRMWARE_READ_MAPPING failed");
            pr_debug("nvmeCPL.statusField.SC: %x",nvmeCPL.statusField.SC);
            pr_debug("nvmeCPL.statusField.SCT: %x",nvmeCPL.statusField.SCT);
            set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        }
        else{
            nvmeCPL.dword[0] = 0;
            nvmeCPL.specific = 0x0;
            nvmeCPL.specific = 0x0;
            pr_debug("IO_NVM_FIRMWARE_READ_MAPPING success");
            pr_debug("nvmeCPL.statusField.SC: %x",nvmeCPL.statusField.SC);
            pr_debug("nvmeCPL.statusField.SCT: %x",nvmeCPL.statusField.SCT);
            set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        }
        break;
    }
    case IO_NVM_READ:
    case IO_NVM_READ_PHY:
    {
        pr_debug("IO Read Command");
        handle_nvme_io_read(nvmeCmd->cmdSlotTag, nvmeIOCmd);
        break;
    }
    case IO_NVM_NMC_ALLOC:
    {
        // reset completion status
        nvmeCPL.statusFieldWord = 0;

        // check file name, file type, and number of blocks needed
        filetype = nvmeCmd->cmdDword[14];
        nblks    = nvmeCmd->cmdDword[15];

#if (NMC_SHORT_FILENAME == true)
        strncpy(filename, (char *)&nvmeCmd->cmdDword[10], 4);

        if (nmcValidFilename(filename))
        {
            nvmeCPL.statusField.SC = nmcNewMapping(filename, filetype, nblks);
            if (nvmeCPL.statusField.SC == SC_VENDOR_NMC_SUCCESS)
                nvmeCPL.statusFieldWord = 0;
            else
                nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
        }
        else
        {
            nvmeCPL.statusField.SC  = SC_VENDOR_NMC_MAPPING_FILENAME_UNSUPPORTED;
            nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
        }
#else
        if (nmcRegisterNewMappingReqInit(filetype, nblks))
        {
            handle_nvme_io_write(nvmeCmd->cmdSlotTag, nvmeIOCmd);
            break; // skip set_auto_nvme_cpl, cuz we need nvme rx
        }
        else
        {
            pr_error("Failed to register nmc new mapping (init stage)");
            nvmeCPL.statusField.SC  = SC_VENDOR_NMC_MAPPING_REGISTER_INIT_FAILED;
            nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
        }
#endif

        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        break;
    }
    case IO_NVM_NMC_FLUSH:
    {
        if ((nvmeCPL.statusField.SC = nmcFreeMapping(nvmeCmd->cmdSlotTag, nvmeCmd->cmdDword[15])))
            nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
        else
            nvmeCPL.dword[0] = 0; // mark as success

        nvmeCPL.specific = 0x0;
        set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        break;
    }
    case IO_NVM_WRITE_SLICE:
    {
        handle_nvme_io_monitor(nvmeCmd->cmdSlotTag, nvmeIOCmd);
        break;
    }
    case IO_NVM_GET_PARTIAL_STATUS:
    {
        nvmeCPL.dword[0] = 0;
        //need change and solve some problem
        nvmeCPL.statusField.SC  = SC_VENDOR_GET_PARTIAL_CONTINUE;
        nvmeCPL.statusField.SCT = SCT_VENDOR_SPECIFIC;
        nvmeCPL.specific = 0x0;
        pr_debug("GET_PARTIAL_RESULT_STATUS in handle_nvme_io_cmd");
        pr_debug("nvmeCPL.statusField.SC: %x",nvmeCPL.statusField.SC);
        pr_debug("nvmeCPL.statusField.SCT: %x",nvmeCPL.statusField.SCT);
        set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
        break;
    }
    default:
    {
        xil_printf("Not Support IO Command OPC: %X\r\n", opc);
        ASSERT(0);
        break;
    }
    }
}
