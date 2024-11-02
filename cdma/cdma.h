// BRAM address
#define BRAMADDR 0x50000000
#define BRAM8 0x50000000
#define BRAM9 0x50040000
#define BRAM10 0x50080000
#define BRAM11 0x500C0000
#define BRAM12 0x50100000
// DDR source address and destination address
#define SRCADDR 0x31000000
#define RECADDR 0x2FFFE004


#define INPUT_SIZE 65536  //1024*48
//
//#define PL_BRAM_Addr 0x45202000
//#define PL_BRAM_Addr2 0x45302000
#define OUTPUT_SIZE  65536  //1024*64
int cdma(int counter,int enable);
void check_cdma_done();
int cdma_write(int counter,int *Binaddr,int *Doutaddr,int dataSize);
int cdma_read(int counter,int *Boutaddr,int *Dinaddr,int dataSize);
