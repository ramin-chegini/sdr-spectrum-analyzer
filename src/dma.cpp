#include "dma.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static int dma_fd = -1;
static volatile uint32_t *dma_regs = nullptr;

/*----------------------------------------------------------
 * Initialize DMA
 *---------------------------------------------------------*/
int dma_init(void)
{
    dma_fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (dma_fd < 0)
    {
        perror("open(/dev/mem)");
        return -1;
    }

    dma_regs = (volatile uint32_t *)mmap(
        nullptr,
        SDR_DMA_MAP_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        dma_fd,
        SDR_DMA_BASE_ADDR);

    if (dma_regs == (volatile uint32_t *)MAP_FAILED)
    {
        perror("mmap()");
        close(dma_fd);
        dma_fd = -1;
        dma_regs = nullptr;
        return -1;
    }

    printf("AXI DMA mapped successfully.\n");
    printf("Base Address : 0x%08X\n", SDR_DMA_BASE_ADDR);
    printf("Map Size     : 0x%08X\n", SDR_DMA_MAP_SIZE);

    return 0;
}

/*----------------------------------------------------------
 * Close DMA
 *---------------------------------------------------------*/
void dma_close(void)
{
    if (dma_regs != nullptr)
    {
        munmap((void *)dma_regs, SDR_DMA_MAP_SIZE);
        dma_regs = nullptr;
    }

    if (dma_fd >= 0)
    {
        close(dma_fd);
        dma_fd = -1;
    }
    printf("AXI DMA unmapped.\n");
}

/*----------------------------------------------------------
 * Read Register
 *---------------------------------------------------------*/
uint32_t dma_read_reg(uint32_t offset)
{
    if (dma_regs == nullptr)
        return 0;

    if (offset >= SDR_DMA_MAP_SIZE)
        return 0;

    return dma_regs[offset >> 2];
}

/*----------------------------------------------------------
 * Write Register
 *---------------------------------------------------------*/
void dma_write_reg(uint32_t offset, uint32_t value)
{
    if (dma_regs == nullptr)
        return;

    if (offset >= SDR_DMA_MAP_SIZE)
        return;

    dma_regs[offset >> 2] = value;
}

/*----------------------------------------------------------
 * Reset DMA
 *---------------------------------------------------------*/
void dma_reset(void)
{
    dma_write_reg(SDR_DMA_S2MM_DMACR,
                  SDR_DMA_DMACR_RESET);
     usleep(1000);            
}

/*----------------------------------------------------------
 * Read Status Register
 *---------------------------------------------------------*/
uint32_t dma_status(void)
{
    return dma_read_reg(SDR_DMA_S2MM_DMASR);
}

/*----------------------------------------------------------
 * Print DMA Status
 *---------------------------------------------------------*/
void dma_print_status(void)
{
    uint32_t control = dma_read_reg(SDR_DMA_S2MM_DMACR);
    uint32_t status  = dma_read_reg(SDR_DMA_S2MM_DMASR);

    printf("\n========== AXI DMA ==========\n");
    printf("Base Address : 0x%08X\n", SDR_DMA_BASE_ADDR);
    printf("DMACR = 0x%08X\n", control);
    printf("DMASR = 0x%08X\n\n", status);

    printf("Halted      : %s\n",
           (status & SDR_DMA_DMASR_HALTED) ? "YES" : "NO");

    printf("Idle        : %s\n",
           (status & SDR_DMA_DMASR_IDLE) ? "YES" : "NO");

    printf("IOC IRQ     : %s\n",
           (status & SDR_DMA_DMASR_IOC_IRQ) ? "YES" : "NO");

    printf("Delay IRQ   : %s\n",
           (status & SDR_DMA_DMASR_DELAY_IRQ) ? "YES" : "NO");

    printf("Error IRQ   : %s\n",
           (status & SDR_DMA_DMASR_ERR_IRQ) ? "YES" : "NO");

    printf("Internal Err: %s\n",
           (status & SDR_DMA_DMASR_DMA_INT_ERR) ? "YES" : "NO");

    printf("Slave Err   : %s\n",
           (status & SDR_DMA_DMASR_DMA_SLV_ERR) ? "YES" : "NO");

    printf("Decode Err  : %s\n",
           (status & SDR_DMA_DMASR_DMA_DEC_ERR) ? "YES" : "NO");

    printf("=============================\n\n");
}