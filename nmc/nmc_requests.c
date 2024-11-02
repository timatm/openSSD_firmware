#include "debug.h"
#include "nmc/nmc_requests.h"
#include "nmc/nmc_file_table.h"

#include <string.h>
#include "data_buffer.h"
#include "request_allocation.h"

/* -------------------------------------------------------------------------- */
/*                              internal members                              */
/* -------------------------------------------------------------------------- */

#define NMC_MODEL_FILENAME "model0604.onnx" // TEST
#define MAPPING_TABLE_BASE_ADDRESS 0x45800000

extern NMC_FILE_TABLE *nmcFileTable;
extern FILENAME_BUFFER *nmcMappingFilenameBufPtr;
extern bool verify_img_flag;
static struct
{
    bool initialized;
    uint32_t iReqEntry;
    uint32_t filetype;
    uint32_t nblks;
} nmcPendingNewMappingReq = {.initialized = false, .iReqEntry = REQ_SLOT_TAG_NONE};

static struct
{
    uint32_t iReqEntry;
} nmcPendingInferenceReq = {.iReqEntry = REQ_SLOT_TAG_NONE};

static void nmcInferenceMain(NMC_FILE_INFO imageInfo, NMC_FILE_INFO modelInfo);
static void nmcFilenameFromDataBuf(uint32_t iBufEntry, char *filename);

/* -------------------------------------------------------------------------- */
/*                              public interfaces                             */
/* -------------------------------------------------------------------------- */

bool nmcRegisterNewMappingReqInit(uint32_t filetype, uint32_t nblks)
{
    if (!nmcPendingNewMappingReq.initialized && nmcPendingNewMappingReq.iReqEntry == REQ_SLOT_TAG_NONE)
    {
        pr_info("NMC: A New Mapping Request is Initialized (type=%u, nblks=%u)", filetype, nblks);

        nmcPendingNewMappingReq.initialized = true;
        nmcPendingNewMappingReq.filetype    = filetype;
        nmcPendingNewMappingReq.nblks       = nblks;
        return true;
    }
    else
    {
        pr_error("nmcRegisterNewMappingReqDone: Already initialized or registered...");
        return false;
    }
}

bool nmcRegisterNewMappingReqDone(uint32_t iReqEntry)
{
    P_SSD_REQ_FORMAT reqEntry;

    if (nmcPendingNewMappingReq.initialized && nmcPendingNewMappingReq.iReqEntry == REQ_SLOT_TAG_NONE)
    {
        reqEntry = REQ_ENTRY(iReqEntry);

        pr_info("NMC: A New Mapping Request is Registered by Req[%u]:", iReqEntry);
        pr_debug(".nvmeCmdSlotTag:    %u", reqEntry->nvmeCmdSlotTag);
        pr_debug(".logicalSliceAddr:  %u", reqEntry->logicalSliceAddr);
        pr_debug(".dataBufInfo.entry: %u", reqEntry->dataBufInfo.entry);
        pr_debug("     (base address: 0x%x)", BUF_DATA_ENTRY2ADDR(reqEntry->dataBufInfo.entry));

        nmcPendingNewMappingReq.iReqEntry = iReqEntry;
        return true;
    }
    else
    {
        pr_error("nmcRegisterNewMappingReqDone: Not initialized or already registered...");
        return false;
    }
}

/**
 * @brief Tell the NMC subsystem the given request want to do inference.
 *
 * Because the inference request (slice request) will be split into two sub-requests (NVM
 * request and DMA request), and then be handled separately by the fw, we should remember
 * which slice request issued the DMA transfer (for reading filename from host DRAM), so
 * that the fw can do something (check the DMA buffer content) after the DMA transfer
 * finished.
 *
 * @param iReqEntry The index of the parent slice request of the inference request.
 * @return true if the registration complete successfully; otherwise false.
 */
bool nmcRegisterInferenceReq(uint32_t iReqEntry)
{
    P_SSD_REQ_FORMAT reqInference;

    if (nmcPendingInferenceReq.iReqEntry == REQ_SLOT_TAG_NONE)
    {
        // check request info
        reqInference = REQ_ENTRY(iReqEntry);

        pr_info("NMC: An inference request registered by Req[%u]:", iReqEntry);
        pr_debug(".nvmeCmdSlotTag:    %u", reqInference->nvmeCmdSlotTag);
        pr_debug(".logicalSliceAddr:  %u", reqInference->logicalSliceAddr);
        pr_debug(".dataBufInfo.entry: %u", reqInference->dataBufInfo.entry);
        pr_debug("     (base address: 0x%x)", BUF_DATA_ENTRY2ADDR(reqInference->dataBufInfo.entry));

        nmcPendingInferenceReq.iReqEntry = iReqEntry;
        return true;
    }
    else
    {
        pr_error("NMC: Too much inference requests.");
        return false;
    }
}

/**
 * @brief Scheduling pending NMC requests.
 *
 * The original firmware does not have a machanism to handle the write commands other than
 * buffered in data buffers. However, the NMC request may need to trigger some tasks after
 * receiving some data from the host.
 *
 * Therefore, this function is used for handling the tasks after receiving the needed data
 * from the host. And the work flow is:
 *
 * 1. Get a pending task in a predefined priority order
 * 2. Trigger the predefined tasks using the received data
 * 3. Wait for the task (blocking)
 * 4. Back to 1 if there is still any pending request; otherwise, reset the request status
 */
void nmcReqScheduling()
{
    int res;
    char filename[NMC_FILENAME_MAX_BYTES+1]; /* be careful with stack size */
    NMC_FILE_INFO imageInfo, modelInfo;
    // for verify_img 
    if (verify_img_flag == true){
        nmcFilenameFromDataBuf(REQ_ENTRY(nmcPendingInferenceReq.iReqEntry)->dataBufInfo.entry, filename);
        pr_info("dataBufInfo filename:%s",filename);
        strncpy(nmcMappingFilenameBufPtr->title,filename , NMC_FILENAME_MAX_BYTES);
        pr_info("nmcMappingFilenameBufPtr->title filename:%s",nmcMappingFilenameBufPtr->title);
    }

    else{
        // nmcPendingNewMappingReq for new_mapping request
        if (nmcPendingNewMappingReq.initialized && nmcPendingNewMappingReq.iReqEntry != REQ_SLOT_TAG_NONE)
        {
            nmcFilenameFromDataBuf(REQ_ENTRY(nmcPendingNewMappingReq.iReqEntry)->dataBufInfo.entry, filename);
            pr_info("Received filename: '%s'", filename);

            res = nmcNewMapping(filename, nmcPendingNewMappingReq.filetype, nmcPendingNewMappingReq.nblks);
            if (res != SC_VENDOR_NMC_SUCCESS)
                pr_error("NMC: Allocate New Mapping Failed");
        }
        //for inference
        if (nmcPendingInferenceReq.iReqEntry != REQ_SLOT_TAG_NONE)
        {
            // check filename
            nmcFilenameFromDataBuf(REQ_ENTRY(nmcPendingInferenceReq.iReqEntry)->dataBufInfo.entry, filename);

            pr_debug("NMC: Target image filename: '%s'", filename);

            // get the location of the mapping table of target model and image
            imageInfo = nmcSearchFile(filename);
            pr_info("NMC: The file info of '%s':", filename);
            dumpFileInfo(imageInfo);

            modelInfo = nmcSearchFile(NMC_MODEL_FILENAME);
            pr_info("NMC: The file info of '%s':", NMC_MODEL_FILENAME);
            dumpFileInfo(modelInfo);

            // start the inference process
            nmcInferenceMain(imageInfo, modelInfo);
        }
    }
    verify_img_flag = false;

    // DMA and internal tasks shall already done, just reset the status
    nmcPendingInferenceReq.iReqEntry = REQ_SLOT_TAG_NONE;

    nmcPendingNewMappingReq.initialized = false;
    nmcPendingNewMappingReq.iReqEntry   = REQ_SLOT_TAG_NONE;
}

/* -------------------------------------------------------------------------- */
/*                      internal function implementations                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Inferencing the specified image with the specified model
 *
 * The main function for inferencing the specified image with the specified model.
 *
 * @param imageInfo The file info of the specified image.
 * @param modelInfo The file info of the specified model.
 */
static void nmcInferenceMain(NMC_FILE_INFO imageInfo, NMC_FILE_INFO modelInfo)
{
    pr_info("nmcInferenceMain start");
    NMC_MAPPING_LOC* ch_mapping_table_address = 0x0;
    // ensure the mapping of both the model and the image exists.
    if (imageInfo.type == NMC_FILE_TYPE_NONE || modelInfo.type == NMC_FILE_TYPE_NONE)
        pr_warn("NMC: cannot find the file info of the specified files, inference aborted...");
    else
    {
        // TODO: notify FPGA
        // TODO: handling inference results
        for (int i = 0;i < 8;i++){

            // store model_mapping_table imformation at (0x45800000 + channel_num*0x10)
            ch_mapping_table_address = MAPPING_TABLE_BASE_ADDRESS + 0x10 * i;
            ch_mapping_table_address->iWay = modelInfo.mapping[i].iWay;
            ch_mapping_table_address->iDir = NMC_MAPPING_DIR2PBLK(modelInfo.mapping[i].iDir);
            ch_mapping_table_address->iPage = modelInfo.mapping[i].iPage;

            // ch_mapping_table_address->iWay = 0x1;
            // ch_mapping_table_address->iDir = 0x234;
            // ch_mapping_table_address->iPage = 0x5678;

            // store image_mapping_table imformation at (0x45800000 + channel_num*0x10 + 8)
            ch_mapping_table_address = ch_mapping_table_address+ 1;
            ch_mapping_table_address->iWay = imageInfo.mapping[i].iWay;
            ch_mapping_table_address->iDir = NMC_MAPPING_DIR2PBLK(imageInfo.mapping[i].iDir);
            ch_mapping_table_address->iPage = imageInfo.mapping[i].iPage;

            // ch_mapping_table_address->iWay = 0x8;
            // ch_mapping_table_address->iDir = 0x765;
            // ch_mapping_table_address->iPage = 0x4321;
        }
    }
}

/* -------------------------------------------------------------------------- */
/*                         internal utility functions                         */
/* -------------------------------------------------------------------------- */

static void nmcFilenameFromDataBuf(uint32_t iBufEntry, char *filename)
{
    // check filename
    char *bufAddr = (char *)BUF_DATA_ENTRY2ADDR(iBufEntry);
    int flag = 0;
    // clear buffer and copy filename
    // memset(filename, 0, NMC_FILENAME_MAX_BYTES);
    // strncpy(filename, (const char *)bufAddr, NMC_FILENAME_MAX_BYTES);
    // pr_info("filename:%s",filename);
    // discard the trailing \r\n
    // for (int i = NMC_FILENAME_MAX_BYTES; i >= 0; --i)
    // {
    //     pr_info("filename[i]: %c",filename[i]);
    //     if (filename[i] == '\0')
    //         continue;
    //     else if (filename[i] == '\r' || filename[i] == '\n')
    //         filename[i] = '\0';
    //     else
    //         break;
    // }
    for (int i = 1; i < NMC_FILENAME_MAX_BYTES; i++)
    {
        if(bufAddr[i] == '\0')
        {
            flag = i;
            pr_info("flag:%i",flag);
            break;;
        }
    }
    memset(filename, 0, NMC_FILENAME_MAX_BYTES);
    strncpy(filename, (const char *)bufAddr, flag);
    pr_info("filename in nmcFilenameFromDataBuf:%s",filename);
}
