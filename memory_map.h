//////////////////////////////////////////////////////////////////////////////////
// memory_map.h for Cosmos+ OpenSSD
// Copyright (c) 2017 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
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
// Engineer: Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Static Memory Allocator
// File Name: memory_map.h
//
// Version: v1.0.0
//
// Description:
//	 - allocate DRAM address space (0x0010_0000 ~ 0x3FFF_FFFF) to each module
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

#include "data_buffer.h"
#include "address_translation.h"
#include "request_allocation.h"
#include "request_schedule.h"
#include "request_transform.h"
#include "garbage_collection.h"

#include "monitor/monitor.h"

#include "nmc/nmc_mapping.h"

#define ALIGN_UP(x, sz) (((x) + ((sz)-1)) & ~((sz)-1))

#define DRAM_START_ADDR 0x00100000

#define MEMORY_SEGMENTS_START_ADDR DRAM_START_ADDR
#define MEMORY_SEGMENTS_END_ADDR   0x001FFFFF

#define NVME_MANAGEMENT_START_ADDR 0x00200000
#define NVME_MANAGEMENT_END_ADDR   0x002FFFFF

#define RESERVED0_START_ADDR 0x00300000
#define RESERVED0_END_ADDR   0x0FFFFFFF

#define FTL_MANAGEMENT_START_ADDR 0x10000000
// Uncached & Unbuffered
// for data buffer

/**
 * @brief The base address of buffer for DMA requests.
 *
 * different from `DATA_BUFFER_MAP_ADDR`, but with the same number of entries (`AVAILABLE_DATA_BUFFER_ENTRY_COUNT`)
 */
#define DATA_BUFFER_BASE_ADDR 0x10000000
#define TEMPORARY_DATA_BUFFER_BASE_ADDR                                                                           \
    (DATA_BUFFER_BASE_ADDR + AVAILABLE_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_DATA_REGION_OF_SLICE)
#define SPARE_DATA_BUFFER_BASE_ADDR                                                                               \
    (TEMPORARY_DATA_BUFFER_BASE_ADDR +                                                                            \
     AVAILABLE_TEMPORARY_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_DATA_REGION_OF_SLICE)
#define TEMPORARY_SPARE_DATA_BUFFER_BASE_ADDR                                                                     \
    (SPARE_DATA_BUFFER_BASE_ADDR + AVAILABLE_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_SPARE_REGION_OF_SLICE)
#define RESERVED_DATA_BUFFER_BASE_ADDR                                                                            \
    (TEMPORARY_SPARE_DATA_BUFFER_BASE_ADDR +                                                                      \
     AVAILABLE_TEMPORARY_DATA_BUFFER_ENTRY_COUNT * BYTES_PER_SPARE_REGION_OF_SLICE)

// dedicated buffers for monitoring
#define MONITOR_START_ADDR           (RESERVED_DATA_BUFFER_BASE_ADDR + 0x00200000)
#define MONITOR_DATA_BUFFER_ADDR     (MONITOR_START_ADDR)
#define MONITOR_DATA_BUFFER_END_ADDR (MONITOR_DATA_BUFFER_ADDR + sizeof(MONITOR_DATA_BUFFER))
#define MONITOR_END_ADDR             (MONITOR_DATA_BUFFER_END_ADDR)  //MONITOR_END_ADDR: 0x11448000

// check
#define CH_INFO_START_ADDR         0x12000000
#define CH_INFO_BUFFER_ADDR        (CH_INFO_START_ADDR)
#define CH_INFO_BUFFER_END_ADDR    (CH_INFO_START_ADDR + sizeof(CH_INFO))
#define CH_INFO_END_ADDR           (CH_INFO_BUFFER_END_ADDR)

// for nand request completion
#define COMPLETE_FLAG_TABLE_ADDR 0x17000000
#define STATUS_REPORT_TABLE_ADDR (COMPLETE_FLAG_TABLE_ADDR + sizeof(COMPLETE_FLAG_TABLE))
#define ERROR_INFO_TABLE_ADDR    (STATUS_REPORT_TABLE_ADDR + sizeof(STATUS_REPORT_TABLE))
#define TEMPORARY_PAY_LOAD_ADDR  (ERROR_INFO_TABLE_ADDR + sizeof(ERROR_INFO_TABLE))
// cached & buffered
// for buffers
#define DATA_BUFFER_MAP_ADDR           0x18000000
#define DATA_BUFFFER_HASH_TABLE_ADDR   (DATA_BUFFER_MAP_ADDR + sizeof(DATA_BUF_MAP))
#define TEMPORARY_DATA_BUFFER_MAP_ADDR (DATA_BUFFFER_HASH_TABLE_ADDR + sizeof(DATA_BUF_HASH_TABLE))
// for map tables
#define LOGICAL_SLICE_MAP_ADDR        (TEMPORARY_DATA_BUFFER_MAP_ADDR + sizeof(TEMPORARY_DATA_BUF_MAP))
#define VIRTUAL_SLICE_MAP_ADDR        (LOGICAL_SLICE_MAP_ADDR + sizeof(LOGICAL_SLICE_MAP))
#define VIRTUAL_BLOCK_MAP_ADDR        (VIRTUAL_SLICE_MAP_ADDR + sizeof(VIRTUAL_SLICE_MAP))
#define PHY_BLOCK_MAP_ADDR            (VIRTUAL_BLOCK_MAP_ADDR + sizeof(VIRTUAL_BLOCK_MAP))
#define BAD_BLOCK_TABLE_INFO_MAP_ADDR (PHY_BLOCK_MAP_ADDR + sizeof(PHY_BLOCK_MAP))
#define VIRTUAL_DIE_MAP_ADDR          (BAD_BLOCK_TABLE_INFO_MAP_ADDR + sizeof(BAD_BLOCK_TABLE_INFO_MAP))
// for GC victim selection
#define GC_VICTIM_MAP_ADDR (VIRTUAL_DIE_MAP_ADDR + sizeof(VIRTUAL_DIE_MAP))

// for request pool
#define REQ_POOL_ADDR (GC_VICTIM_MAP_ADDR + sizeof(GC_VICTIM_MAP))
// for dependency table
#define ROW_ADDR_DEPENDENCY_TABLE_ADDR (REQ_POOL_ADDR + sizeof(REQ_POOL))
// for request scheduler
#define DIE_STATE_TABLE_ADDR    (ROW_ADDR_DEPENDENCY_TABLE_ADDR + sizeof(ROW_ADDR_DEPENDENCY_TABLE))
#define RETRY_LIMIT_TABLE_ADDR  (DIE_STATE_TABLE_ADDR + sizeof(DIE_STATE_TABLE))
#define WAY_PRIORITY_TABLE_ADDR (RETRY_LIMIT_TABLE_ADDR + sizeof(RETRY_LIMIT_TABLE))

#define FTL_MANAGEMENT_END_ADDR ((WAY_PRIORITY_TABLE_ADDR + sizeof(WAY_PRIORITY_TABLE)) - 1)

#define RESERVED1_START_ADDR (FTL_MANAGEMENT_END_ADDR + 1)

/* -------------------------------------------------------------------------- */
/*                         allocate space for monitor                         */
/* -------------------------------------------------------------------------- */

#define NMC_START_ADDR               (RESERVED1_START_ADDR)
#define NMC_FILE_TABLE_ADDR          (NMC_START_ADDR)
#define NMC_FILE_TABLE_END_ADDR      (NMC_FILE_TABLE_ADDR + NMC_FILE_TABLE_BYTES)
#define NMC_MAPPING_TABLE_ADDR       (NMC_FILE_TABLE_END_ADDR)
#define NMC_MAPPING_TABLE_END_ADDR   (NMC_MAPPING_TABLE_ADDR + sizeof(NMC_MAPPING_TABLE))
#define NMC_BLOCK_TABLE_ADDR         (NMC_MAPPING_TABLE_END_ADDR)
#define NMC_BLOCK_TABLE_END_ADDR     (NMC_BLOCK_TABLE_ADDR + sizeof(NMC_BLOCK_TABLE))
#define NMC_TMP_BLOCK_TABLE_ADDR     (NMC_BLOCK_TABLE_END_ADDR)
#define NMC_TMP_BLOCK_TABLE_END_ADDR (NMC_TMP_BLOCK_TABLE_ADDR + sizeof(NMC_BLOCK_TABLE))
#define NMC_END_ADDR                 (NMC_TMP_BLOCK_TABLE_END_ADDR)


// non-buffered and uncached for buffers and mmap registers
#define NMC_BUFFERS_START_ADDR            (ALIGN_UP(NMC_END_ADDR, 1024 * 1024))
#define NMC_DATA_BUFFER_START_ADDR        (NMC_BUFFERS_START_ADDR)
#define NMC_MAPPING_TABLE_BUFFER_ADDR     (NMC_DATA_BUFFER_START_ADDR)
#define NMC_MAPPING_TABLE_BUFFER_END_ADDR (NMC_MAPPING_TABLE_BUFFER_ADDR + sizeof(NMC_MAPPING_TABLE))
#define NMC_FILENAME_DATA_BUFFER_ADDR     (NMC_MAPPING_TABLE_BUFFER_END_ADDR)
#define NMC_FILENAME_DATA_BUFFER_END_ADDR (NMC_FILENAME_DATA_BUFFER_ADDR + sizeof(FILENAME_BUFFER))
#define NMC_DATA_BUFFER_END_ADDR          (NMC_FILENAME_DATA_BUFFER_END_ADDR)
#define NMC_BUFFERS_END_ADDR              (ALIGN_UP(NMC_DATA_BUFFER_END_ADDR, 1024 * 1024))

#define NMC_BUFFERS_START_MB (NMC_BUFFERS_START_ADDR >> 20)
#define NMC_BUFFERS_END_MB   (NMC_BUFFERS_END_ADDR >> 20)

// #define RESULT_DATA_BUFFER 0x30000000
#define PARTIAL_DATA_BUFFER_MAP_ADDR 0x2FFFE000
#define TEST_BUFFER_ADDR 0x2FFFE020
#define PARTIAL_RESULT_BUUFER_START_ADDR 0x2FFFF000
#define SPECIAL_DATA_HEADER_ADDR (PARTIAL_RESULT_BUUFER_START_ADDR)
// #define PARTIAL_RESULT_BUFFER_END_ADDR   (PARTIAL_DATA_BUFFER_MAP_ADDR + sizeof(PARTIAL_DATA_MAP))



#define RESERVED1_END_ADDR 0x3FFFFFFF

#define DRAM_END_ADDR 0x3FFFFFFF

#endif /* MEMORY_MAP_H_ */