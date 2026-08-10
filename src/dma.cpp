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
 * Start S2MM Simple Transfer
 *---------------------------------------------------------*/
void dma_start_s2mm(uint32_t buffer_addr, uint32_t length)
{
    if (dma_regs == nullptr)
    {
        printf("DMA is not initialized.\n");
        return;
    }

    printf("\nStarting S2MM transfer...\n");
    printf("Destination : 0x%08X\n", buffer_addr);
    printf("Length      : %u bytes\n", length);

    /*
     * Reset DMA
     */
    dma_reset();

    /*
     * Start S2MM channel
     *
     * RS            = Run/Stop
     * IOC_IRQ_EN    = Interrupt on Complete
     * ERR_IRQ_EN    = Error Interrupt
     */
    uint32_t dmacr =
        SDR_DMA_DMACR_RS |
        SDR_DMA_DMACR_IOC_IRQ_EN |
        SDR_DMA_DMACR_ERR_IRQ_EN;

    dma_write_reg(
        SDR_DMA_S2MM_DMACR,
        dmacr
    );

    /*
     * Set destination address
     */
    dma_write_reg(
        SDR_DMA_S2MM_DA,
        buffer_addr
    );

    /*
     * Writing LENGTH starts the transfer
     */
    dma_write_reg(
        SDR_DMA_S2MM_LENGTH,
        length
    );

    printf(
        "DMACR = 0x%08X\n",
        dma_read_reg(SDR_DMA_S2MM_DMACR)
    );

    printf(
        "DMASR = 0x%08X\n",
        dma_read_reg(SDR_DMA_S2MM_DMASR)
    );

    printf(
        "S2MM_DA = 0x%08X\n",
        dma_read_reg(SDR_DMA_S2MM_DA)
    );

    printf(
        "S2MM_LENGTH = %u\n",
        dma_read_reg(SDR_DMA_S2MM_LENGTH)
    );
}

/*----------------------------------------------------------
 * Wait for S2MM completion
 *---------------------------------------------------------*/
bool dma_wait_for_completion(uint32_t timeout_ms)
{
    if (dma_regs == nullptr)
    {
        return false;
    }

    for (uint32_t i = 0; i < timeout_ms; i++)
    {
        uint32_t status = dma_status();

        if (status & SDR_DMA_DMASR_DMA_INT_ERR)
        {
            printf("DMA Internal Error\n");
            return false;
        }

        if (status & SDR_DMA_DMASR_DMA_SLV_ERR)
        {
            printf("DMA Slave Error\n");
            return false;
        }

        if (status & SDR_DMA_DMASR_DMA_DEC_ERR)
        {
            printf("DMA Decode Error\n");
            return false;
        }

        if (status & SDR_DMA_DMASR_IOC_IRQ)
        {
            printf("DMA transfer completed.\n");
            return true;
        }

        usleep(1000);
    }

    printf(
        "DMA timeout after %u ms.\n",
        timeout_ms
    );

    return false;
}

/*----------------------------------------------------------
 * Stop S2MM
 *---------------------------------------------------------*/
void dma_stop_s2mm(void)
{
    if (dma_regs == nullptr)
    {
        return;
    }

    dma_write_reg(
        SDR_DMA_S2MM_DMACR,
        0
    );

    printf("S2MM stopped.\n");
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