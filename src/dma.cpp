#include "dma.h"

#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>


/*==========================================================
 * Initialize DMA
 *=========================================================*/

int dma_init(DmaContext *ctx, uint32_t base_addr)
{
    if (ctx == nullptr)
    {
        printf("DMA context is null.\n");
        return -1;
    }

    /*------------------------------------------------------
     * Initialize context
     *-----------------------------------------------------*/

    ctx->dma_fd = -1;
    ctx->uio_fd = -1;
    ctx->dma_regs = nullptr;
    ctx->base_addr = base_addr;
    ctx->map_size = DMA_MAP_SIZE;


    /*------------------------------------------------------
     * Open /dev/mem
     *-----------------------------------------------------*/

    ctx->dma_fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (ctx->dma_fd < 0)
    {
        perror("open(/dev/mem)");
        return -1;
    }


    /*------------------------------------------------------
     * Map AXI DMA registers
     *-----------------------------------------------------*/

    ctx->dma_regs = (volatile uint32_t *)mmap(
        nullptr,
        ctx->map_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        ctx->dma_fd,
        ctx->base_addr
    );


    if (ctx->dma_regs == (volatile uint32_t *)MAP_FAILED)
    {
        perror("mmap()");

        close(ctx->dma_fd);

        ctx->dma_fd = -1;
        ctx->dma_regs = nullptr;

        return -1;
    }


    printf("\nAXI DMA mapped successfully.\n");
    printf("Base Address : 0x%08X\n", ctx->base_addr);
    printf("Map Size     : 0x%08X\n", ctx->map_size);

    return 0;
}


/*==========================================================
 * Enable UIO IRQ
 *=========================================================*/

static int dma_irq_enable(DmaContext *ctx)
{
    if (ctx == nullptr)
    {
        printf("DMA context is null.\n");
        return -1;
    }

    if (ctx->uio_fd < 0)
    {
        printf("UIO is not initialized.\n");
        return -1;
    }


    uint32_t enable = 1;

    ssize_t n = write(
        ctx->uio_fd,
        &enable,
        sizeof(enable)
    );


    if (n != sizeof(enable))
    {
        perror("write(UIO) - enable IRQ");
        return -1;
    }


    return 0;
}


/*==========================================================
 * Open UIO Interrupt
 *=========================================================*/

int dma_irq_init(
    DmaContext *ctx,
    const char *uio_device
)
{
    if (ctx == nullptr)
    {
        printf("DMA context is null.\n");
        return -1;
    }

    if (uio_device == nullptr)
    {
        printf("UIO device is null.\n");
        return -1;
    }


    ctx->uio_fd = open(
        uio_device,
        O_RDWR
    );


    if (ctx->uio_fd < 0)
    {
        perror("open(UIO)");
        return -1;
    }


    printf(
        "DMA UIO opened successfully: %s\n",
        uio_device
    );


    if (dma_irq_enable(ctx) != 0)
    {
        close(ctx->uio_fd);

        ctx->uio_fd = -1;

        return -1;
    }


    printf("DMA UIO IRQ enabled.\n");

    return 0;
}


/*==========================================================
 * Close UIO
 *=========================================================*/

void dma_irq_close(DmaContext *ctx)
{
    if (ctx == nullptr)
        return;


    if (ctx->uio_fd >= 0)
    {
        close(ctx->uio_fd);

        ctx->uio_fd = -1;
    }
}


/*==========================================================
 * Close DMA
 *=========================================================*/

void dma_close(DmaContext *ctx)
{
    if (ctx == nullptr)
        return;


    if (ctx->dma_regs != nullptr)
    {
        munmap(
            (void *)ctx->dma_regs,
            ctx->map_size
        );

        ctx->dma_regs = nullptr;
    }


    if (ctx->dma_fd >= 0)
    {
        close(ctx->dma_fd);

        ctx->dma_fd = -1;
    }


    printf(
        "AXI DMA unmapped. Base = 0x%08X\n",
        ctx->base_addr
    );
}


/*==========================================================
 * Read Register
 *=========================================================*/

uint32_t dma_read_reg(
    DmaContext *ctx,
    uint32_t offset
)
{
    if (ctx == nullptr)
        return 0;


    if (ctx->dma_regs == nullptr)
        return 0;


    if (offset >= ctx->map_size)
        return 0;


    return ctx->dma_regs[offset >> 2];
}


/*==========================================================
 * Write Register
 *=========================================================*/

void dma_write_reg(
    DmaContext *ctx,
    uint32_t offset,
    uint32_t value
)
{
    if (ctx == nullptr)
        return;


    if (ctx->dma_regs == nullptr)
        return;


    if (offset >= ctx->map_size)
        return;


    ctx->dma_regs[offset >> 2] = value;
}


/*==========================================================
 * Reset DMA
 *=========================================================*/

void dma_reset(DmaContext *ctx)
{
    if (ctx == nullptr)
        return;


    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_DMACR,
        SDR_DMA_DMACR_RESET
    );


    /*
     * Give reset some time.
     */

    usleep(1000);
}


/*==========================================================
 * Start S2MM Simple Transfer
 *=========================================================*/

void dma_start_s2mm(
    DmaContext *ctx,
    uint32_t buffer_addr,
    uint32_t length
)
{
    if (ctx == nullptr)
    {
        printf("DMA context is null.\n");
        return;
    }


    if (ctx->dma_regs == nullptr)
    {
        printf("DMA is not initialized.\n");
        return;
    }


    printf(
        "\nStarting S2MM transfer...\n"
    );


    printf(
        "DMA Base    : 0x%08X\n",
        ctx->base_addr
    );


    printf(
        "Destination : 0x%08X\n",
        buffer_addr
    );


    printf(
        "Length      : %u bytes\n",
        length
    );


    /*------------------------------------------------------
     * Reset DMA
     *-----------------------------------------------------*/

    dma_reset(ctx);


    printf(
        "DMASR after reset = 0x%08X\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMASR
        )
    );


    /*------------------------------------------------------
     * Clear previous interrupt status
     *
     * DMASR interrupt bits are W1C.
     *-----------------------------------------------------*/

    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_DMASR,
        SDR_DMA_DMASR_IOC_IRQ |
        SDR_DMA_DMASR_DELAY_IRQ |
        SDR_DMA_DMASR_ERR_IRQ
    );


    printf(
        "DMASR after clear = 0x%08X\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMASR
        )
    );


    /*------------------------------------------------------
     * Start S2MM
     *
     * RS         = Run
     * IOC_IRQ_EN = Interrupt on Complete
     * ERR_IRQ_EN = Error interrupt
     *-----------------------------------------------------*/

    uint32_t dmacr =
        SDR_DMA_DMACR_RS |
        SDR_DMA_DMACR_IOC_IRQ_EN |
        SDR_DMA_DMACR_ERR_IRQ_EN;


    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_DMACR,
        dmacr
    );


    /*------------------------------------------------------
     * Destination address
     *-----------------------------------------------------*/

    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_DA,
        buffer_addr
    );


    /*------------------------------------------------------
     * IMPORTANT:
     *
     * Read LENGTH before programming it.
     *-----------------------------------------------------*/

    printf(
        "S2MM_LENGTH before = %u\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_LENGTH
        )
    );


    /*------------------------------------------------------
     * Writing LENGTH starts the transfer.
     *-----------------------------------------------------*/

    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_LENGTH,
        length
    );


    /*------------------------------------------------------
     * Immediately read LENGTH back.
     *-----------------------------------------------------*/

    printf(
        "S2MM_LENGTH immediately after write = %u\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_LENGTH
        )
    );


    /*------------------------------------------------------
     * Allow DMA to run briefly.
     *-----------------------------------------------------*/

    usleep(100);


    /*------------------------------------------------------
     * Read LENGTH again.
     *
     * This is diagnostic only.
     *-----------------------------------------------------*/

    printf(
        "S2MM_LENGTH after 100us = %u\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_LENGTH
        )
    );


    /*------------------------------------------------------
     * Print current registers.
     *-----------------------------------------------------*/

    printf(
        "DMACR = 0x%08X\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMACR
        )
    );


    printf(
        "DMASR = 0x%08X\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMASR
        )
    );


    printf(
        "S2MM_DA = 0x%08X\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DA
        )
    );


    printf(
        "S2MM_LENGTH = %u\n",
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_LENGTH
        )
    );
}


/*==========================================================
 * Wait for S2MM Completion
 *=========================================================*/

bool dma_wait_for_completion(
    DmaContext *ctx,
    uint32_t timeout_ms
)
{
    if (ctx == nullptr)
    {
        printf("DMA context is null.\n");
        return false;
    }


    if (ctx->dma_regs == nullptr)
    {
        printf("DMA is not initialized.\n");
        return false;
    }


    if (ctx->uio_fd < 0)
    {
        printf("UIO is not initialized.\n");
        return false;
    }


    struct pollfd pfd;

    pfd.fd = ctx->uio_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;


    printf(
        "Waiting for DMA IRQ "
        "(Base = 0x%08X)...\n",
        ctx->base_addr
    );


    int ret = poll(
        &pfd,
        1,
        timeout_ms
    );


    if (ret < 0)
    {
        perror("poll(UIO)");
        return false;
    }


    if (ret == 0)
    {
        printf(
            "DMA IRQ timeout after %u ms.\n",
            timeout_ms
        );

        return false;
    }


    uint32_t irq_count = 0;


    ssize_t n = read(
        ctx->uio_fd,
        &irq_count,
        sizeof(irq_count)
    );


    if (n != sizeof(irq_count))
    {
        perror("read(UIO)");
        return false;
    }


    printf(
        "DMA IRQ received. "
        "UIO count = %u\n",
        irq_count
    );


    /*------------------------------------------------------
     * Read status at the exact IRQ point.
     *-----------------------------------------------------*/

    uint32_t status =
        dma_status(ctx);


    uint32_t length_at_irq =
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_LENGTH
        );


    printf(
        "DMASR after IRQ = 0x%08X\n",
        status
    );


    printf(
        "S2MM_LENGTH at IRQ = %u\n",
        length_at_irq
    );


    /*------------------------------------------------------
     * Error checks
     *-----------------------------------------------------*/

    if (status & SDR_DMA_DMASR_DMA_INT_ERR)
    {
        printf("DMA Internal Error\n");


        dma_write_reg(
            ctx,
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );


        dma_irq_enable(ctx);

        return false;
    }


    if (status & SDR_DMA_DMASR_DMA_SLV_ERR)
    {
        printf("DMA Slave Error\n");


        dma_write_reg(
            ctx,
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );


        dma_irq_enable(ctx);

        return false;
    }


    if (status & SDR_DMA_DMASR_DMA_DEC_ERR)
    {
        printf("DMA Decode Error\n");


        dma_write_reg(
            ctx,
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );


        dma_irq_enable(ctx);

        return false;
    }


    /*------------------------------------------------------
     * Successful completion
     *-----------------------------------------------------*/

    if (status & SDR_DMA_DMASR_IOC_IRQ)
    {
        printf(
            "DMA transfer completed by IRQ.\n"
        );


        /*
         * Clear IOC interrupt.
         */

        dma_write_reg(
            ctx,
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_IOC_IRQ
        );


        /*
         * Re-enable UIO interrupt.
         */

        if (dma_irq_enable(ctx) != 0)
        {
            printf(
                "Failed to re-enable UIO IRQ.\n"
            );

            return false;
        }


        return true;
    }


    printf(
        "IRQ received, but IOC IRQ is not set.\n"
    );


    /*
     * Make UIO ready for next interrupt.
     */

    dma_irq_enable(ctx);

    return false;
}


/*==========================================================
 * Stop S2MM
 *=========================================================*/

void dma_stop_s2mm(DmaContext *ctx)
{
    if (ctx == nullptr)
        return;


    if (ctx->dma_regs == nullptr)
        return;


    dma_write_reg(
        ctx,
        SDR_DMA_S2MM_DMACR,
        0
    );


    printf(
        "S2MM stopped. Base = 0x%08X\n",
        ctx->base_addr
    );
}


/*==========================================================
 * Read Status
 *=========================================================*/

uint32_t dma_status(DmaContext *ctx)
{
    if (ctx == nullptr)
        return 0;


    return dma_read_reg(
        ctx,
        SDR_DMA_S2MM_DMASR
    );
}


/*==========================================================
 * Print DMA Status
 *=========================================================*/

void dma_print_status(DmaContext *ctx)
{
    if (ctx == nullptr)
        return;


    uint32_t control =
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMACR
        );


    uint32_t status =
        dma_read_reg(
            ctx,
            SDR_DMA_S2MM_DMASR
        );


    printf(
        "\n========== AXI DMA ==========\n"
    );


    printf(
        "Base Address : 0x%08X\n",
        ctx->base_addr
    );


    printf(
        "DMACR = 0x%08X\n",
        control
    );


    printf(
        "DMASR = 0x%08X\n\n",
        status
    );


    printf(
        "Halted      : %s\n",
        (status & SDR_DMA_DMASR_HALTED)
            ? "YES"
            : "NO"
    );


    printf(
        "Idle        : %s\n",
        (status & SDR_DMA_DMASR_IDLE)
            ? "YES"
            : "NO"
    );


    printf(
        "IOC IRQ     : %s\n",
        (status & SDR_DMA_DMASR_IOC_IRQ)
            ? "YES"
            : "NO"
    );


    printf(
        "Delay IRQ   : %s\n",
        (status & SDR_DMA_DMASR_DELAY_IRQ)
            ? "YES"
            : "NO"
    );


    printf(
        "Error IRQ   : %s\n",
        (status & SDR_DMA_DMASR_ERR_IRQ)
            ? "YES"
            : "NO"
    );


    printf(
        "Internal Err: %s\n",
        (status & SDR_DMA_DMASR_DMA_INT_ERR)
            ? "YES"
            : "NO"
    );


    printf(
        "Slave Err   : %s\n",
        (status & SDR_DMA_DMASR_DMA_SLV_ERR)
            ? "YES"
            : "NO"
    );


    printf(
        "Decode Err  : %s\n",
        (status & SDR_DMA_DMASR_DMA_DEC_ERR)
            ? "YES"
            : "NO"
    );


    printf(
        "=============================\n\n"
    );
}


/*==========================================================
 * DMA Buffer Dump
 *=========================================================*/

void dma_dump_buffer(
    uint32_t buffer_addr,
    uint32_t length
)
{
    /*
     * This function uses /dev/mem only for accessing
     * the physical DMA buffer.
     *
     * Since dma_fd is no longer global, open /dev/mem
     * locally here.
     */

    int fd = open(
        "/dev/mem",
        O_RDWR | O_SYNC
    );


    if (fd < 0)
    {
        perror("open(/dev/mem)");
        return;
    }


    void *mapped_buffer = mmap(
        nullptr,
        length,
        PROT_READ,
        MAP_SHARED,
        fd,
        buffer_addr
    );


    if (mapped_buffer == MAP_FAILED)
    {
        perror("mmap DMA buffer");

        close(fd);

        return;
    }


    volatile uint32_t *buffer =
        (volatile uint32_t *)mapped_buffer;


    uint32_t samples =
        length / sizeof(uint32_t);


    printf(
        "\n========== DMA BUFFER ==========\n"
    );


    printf(
        "Address : 0x%08X\n",
        buffer_addr
    );


    printf(
        "Length  : %u bytes\n",
        length
    );


    printf(
        "Samples : %u\n",
        samples
    );


    /*------------------------------------------------------
     * First 16 samples
     *-----------------------------------------------------*/

    printf(
        "\nFirst 16 samples:\n"
    );


    uint32_t first_count =
        (samples < 16)
            ? samples
            : 16;


    for (uint32_t i = 0;
         i < first_count;
         i++)
    {
        printf(
            "[%04u] 0x%08X\n",
            i,
            buffer[i]
        );
    }


    /*------------------------------------------------------
     * Last 16 samples
     *-----------------------------------------------------*/

    printf(
        "\nLast 16 samples:\n"
    );


    uint32_t last_start =
        (samples > 16)
            ? samples - 16
            : 0;


    for (
        uint32_t i = last_start;
        i < samples;
        i++
    )
    {
        printf(
            "[%04u] 0x%08X\n",
            i,
            buffer[i]
        );
    }


    /*------------------------------------------------------
     * Save complete DMA buffer to binary file
     *-----------------------------------------------------*/

    FILE *fp = fopen(
        "/tmp/dma_samples.bin",
        "wb"
    );


    if (fp == nullptr)
    {
        perror("fopen /tmp/dma_samples.bin");
    }
    else
    {
        size_t written = fwrite(
            (const void *)buffer,
            sizeof(uint32_t),
            samples,
            fp
        );


        fclose(fp);


        printf(
            "\nDMA samples saved to "
            "/tmp/dma_samples.bin\n"
        );


        printf(
            "Samples written : %zu\n",
            written
        );


        printf(
            "Bytes written   : %zu\n",
            written * sizeof(uint32_t)
        );
    }


    printf(
        "================================\n\n"
    );


    munmap(
        mapped_buffer,
        length
    );


    close(fd);
}


/*==========================================================
 * Map DMA Buffer
 *=========================================================*/

const uint8_t *dma_map_buffer(
    uint32_t buffer_addr,
    uint32_t length
)
{
    /*
     * Buffer mapping is independent of a specific DMA.
     * Open /dev/mem locally.
     */

    int fd = open(
        "/dev/mem",
        O_RDWR | O_SYNC
    );


    if (fd < 0)
    {
        perror("open(/dev/mem)");
        return nullptr;
    }


    void *mapped_buffer = mmap(
        nullptr,
        length,
        PROT_READ,
        MAP_SHARED,
        fd,
        buffer_addr
    );


    if (mapped_buffer == MAP_FAILED)
    {
        perror("mmap DMA buffer");

        close(fd);

        return nullptr;
    }


    /*
     * IMPORTANT:
     *
     * We cannot close fd here if we later need it for
     * munmap only; munmap itself does not require fd.
     */

    close(fd);


    return static_cast<const uint8_t *>(
        mapped_buffer
    );
}


/*==========================================================
 * Unmap DMA Buffer
 *=========================================================*/

void dma_unmap_buffer(
    const uint8_t *buffer,
    uint32_t length
)
{
    if (buffer == nullptr)
        return;


    munmap(
        const_cast<uint8_t *>(buffer),
        length
    );
}