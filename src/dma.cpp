#include "dma.h"

#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/*==========================================================
 * Global State
 *=========================================================*/

static int uio_fd = -1;
static int dma_fd = -1;

static volatile uint32_t *dma_regs = nullptr;


/*==========================================================
 * Initialize DMA
 *=========================================================*/

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
        SDR_DMA_BASE_ADDR
    );

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


/*==========================================================
 * Enable UIO IRQ
 *=========================================================*/

static int dma_irq_enable(void)
{
    if (uio_fd < 0)
    {
        printf("UIO is not initialized.\n");
        return -1;
    }

    uint32_t enable = 1;

    ssize_t n = write(
        uio_fd,
        &enable,
        sizeof(enable)
    );

    if (n != sizeof(enable))
    {
        perror("write(/dev/uio0) - enable IRQ");
        return -1;
    }

    return 0;
}


/*==========================================================
 * Open UIO Interrupt
 *=========================================================*/

int dma_irq_init(void)
{
    uio_fd = open("/dev/uio0", O_RDWR);

    if (uio_fd < 0)
    {
        perror("open(/dev/uio0)");
        return -1;
    }

    printf("DMA UIO opened successfully.\n");

    if (dma_irq_enable() != 0)
    {
        close(uio_fd);

        uio_fd = -1;

        return -1;
    }

    printf("DMA UIO IRQ enabled.\n");

    return 0;
}


/*==========================================================
 * Close UIO
 *=========================================================*/

void dma_irq_close(void)
{
    if (uio_fd >= 0)
    {
        close(uio_fd);

        uio_fd = -1;
    }
}


/*==========================================================
 * Close DMA
 *=========================================================*/

void dma_close(void)
{
    if (dma_regs != nullptr)
    {
        munmap(
            (void *)dma_regs,
            SDR_DMA_MAP_SIZE
        );

        dma_regs = nullptr;
    }

    if (dma_fd >= 0)
    {
        close(dma_fd);

        dma_fd = -1;
    }

    printf("AXI DMA unmapped.\n");
}


/*==========================================================
 * Read Register
 *=========================================================*/

uint32_t dma_read_reg(uint32_t offset)
{
    if (dma_regs == nullptr)
    {
        return 0;
    }

    if (offset >= SDR_DMA_MAP_SIZE)
    {
        return 0;
    }

    return dma_regs[offset >> 2];
}


/*==========================================================
 * Write Register
 *=========================================================*/

void dma_write_reg(uint32_t offset, uint32_t value)
{
    if (dma_regs == nullptr)
    {
        return;
    }

    if (offset >= SDR_DMA_MAP_SIZE)
    {
        return;
    }

    dma_regs[offset >> 2] = value;
}


/*==========================================================
 * Reset DMA
 *=========================================================*/

void dma_reset(void)
{
    dma_write_reg(
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
    uint32_t buffer_addr,
    uint32_t length
)
{
    if (dma_regs == nullptr)
    {
        printf("DMA is not initialized.\n");
        return;
    }

    printf("\nStarting S2MM transfer...\n");

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

    dma_reset();

    printf(
        "DMASR after reset = 0x%08X\n",
        dma_read_reg(SDR_DMA_S2MM_DMASR)
    );


    /*------------------------------------------------------
     * Clear previous interrupt status
     *
     * DMASR interrupt bits are W1C.
     *-----------------------------------------------------*/

    dma_write_reg(
        SDR_DMA_S2MM_DMASR,
        SDR_DMA_DMASR_IOC_IRQ |
        SDR_DMA_DMASR_DELAY_IRQ |
        SDR_DMA_DMASR_ERR_IRQ
    );

    printf(
        "DMASR after clear = 0x%08X\n",
        dma_read_reg(SDR_DMA_S2MM_DMASR)
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
        SDR_DMA_S2MM_DMACR,
        dmacr
    );


    /*------------------------------------------------------
     * Destination address
     *-----------------------------------------------------*/

    dma_write_reg(
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
        dma_read_reg(SDR_DMA_S2MM_LENGTH)
    );


    /*------------------------------------------------------
     * Writing LENGTH starts the transfer.
     *-----------------------------------------------------*/

    dma_write_reg(
        SDR_DMA_S2MM_LENGTH,
        length
    );


    /*------------------------------------------------------
     * Immediately read LENGTH back.
     *-----------------------------------------------------*/

    printf(
        "S2MM_LENGTH immediately after write = %u\n",
        dma_read_reg(SDR_DMA_S2MM_LENGTH)
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
        dma_read_reg(SDR_DMA_S2MM_LENGTH)
    );


    /*------------------------------------------------------
     * Print current registers.
     *-----------------------------------------------------*/

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


/*==========================================================
 * Wait for S2MM Completion
 *=========================================================*/

bool dma_wait_for_completion(uint32_t timeout_ms)
{
    if (dma_regs == nullptr)
    {
        printf("DMA is not initialized.\n");
        return false;
    }

    if (uio_fd < 0)
    {
        printf("UIO is not initialized.\n");
        return false;
    }


    struct pollfd pfd;

    pfd.fd = uio_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;


    printf("Waiting for DMA IRQ...\n");


    int ret = poll(
        &pfd,
        1,
        timeout_ms
    );


    if (ret < 0)
    {
        perror("poll(/dev/uio0)");
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
        uio_fd,
        &irq_count,
        sizeof(irq_count)
    );


    if (n != sizeof(irq_count))
    {
        perror("read(/dev/uio0)");
        return false;
    }


    printf(
        "DMA IRQ received. UIO count = %u\n",
        irq_count
    );


    /*------------------------------------------------------
     * Read status at the exact IRQ point.
     *-----------------------------------------------------*/

    uint32_t status = dma_status();

    uint32_t length_at_irq =
        dma_read_reg(SDR_DMA_S2MM_LENGTH);


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
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );

        dma_irq_enable();

        return false;
    }


    if (status & SDR_DMA_DMASR_DMA_SLV_ERR)
    {
        printf("DMA Slave Error\n");

        dma_write_reg(
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );

        dma_irq_enable();

        return false;
    }


    if (status & SDR_DMA_DMASR_DMA_DEC_ERR)
    {
        printf("DMA Decode Error\n");

        dma_write_reg(
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_ERR_IRQ
        );

        dma_irq_enable();

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
            SDR_DMA_S2MM_DMASR,
            SDR_DMA_DMASR_IOC_IRQ
        );


        /*
         * Re-enable UIO interrupt.
         */
        if (dma_irq_enable() != 0)
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
    dma_irq_enable();

    return false;
}


/*==========================================================
 * Stop S2MM
 *=========================================================*/

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


/*==========================================================
 * Read Status
 *=========================================================*/

uint32_t dma_status(void)
{
    return dma_read_reg(
        SDR_DMA_S2MM_DMASR
    );
}


/*==========================================================
 * Print DMA Status
 *=========================================================*/

void dma_print_status(void)
{
    uint32_t control =
        dma_read_reg(
            SDR_DMA_S2MM_DMACR
        );

    uint32_t status =
        dma_read_reg(
            SDR_DMA_S2MM_DMASR
        );


    printf(
        "\n========== AXI DMA ==========\n"
    );

    printf(
        "Base Address : 0x%08X\n",
        SDR_DMA_BASE_ADDR
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
 *
 * This is intentionally a diagnostic function.
 *
 * Requested buffer:
 *      8192 bytes
 *
 * Number of uint32 samples:
 *      2048
 *
 * We print first and last samples to determine how much
 * of the buffer was actually updated by the latest capture.
 *=========================================================*/

void dma_dump_buffer(
    uint32_t buffer_addr,
    uint32_t length
)
{
    if (dma_fd < 0)
    {
        printf(
            "DMA /dev/mem is not initialized.\n"
        );

        return;
    }


    void *mapped_buffer = mmap(
        nullptr,
        length,
        PROT_READ,
        MAP_SHARED,
        dma_fd,
        buffer_addr
    );


    if (mapped_buffer == MAP_FAILED)
    {
        perror("mmap DMA buffer");
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


    for (uint32_t i = 0; i < first_count; i++)
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
            "\nDMA samples saved to /tmp/dma_samples.bin\n"
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
}

const uint8_t *dma_map_buffer(
    uint32_t buffer_addr,
    uint32_t length
)
{
    if (dma_fd < 0)
    {
        printf("DMA /dev/mem is not initialized.\n");
        return nullptr;
    }

    void *mapped_buffer = mmap(
        nullptr,
        length,
        PROT_READ,
        MAP_SHARED,
        dma_fd,
        buffer_addr
    );

    if (mapped_buffer == MAP_FAILED)
    {
        perror("mmap DMA buffer");
        return nullptr;
    }

    return static_cast<const uint8_t *>(mapped_buffer);
}


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
