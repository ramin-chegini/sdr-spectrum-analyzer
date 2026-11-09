#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>


// ============================================================
// DMA Context
// ============================================================

struct DmaContext
{
    int dma_fd;
    int uio_fd;

    volatile uint32_t *dma_regs;

    uint32_t base_addr;
    uint32_t map_size;
};


// ============================================================
// DMA Base Addresses
// ============================================================

#define PSD_DMA_BASE_ADDR       0x40400000
#define AUDIO_DMA_BASE_ADDR     0x40410000

#define DMA_MAP_SIZE            0x00010000


// ============================================================
// AXI DMA S2MM Registers
// ============================================================

#define SDR_DMA_S2MM_DMACR     0x30
#define SDR_DMA_S2MM_DMASR     0x34
#define SDR_DMA_S2MM_DA        0x48
#define SDR_DMA_S2MM_LENGTH    0x58


// ============================================================
// S2MM DMACR Bits
// ============================================================

#define SDR_DMA_DMACR_RS            (1u << 0)
#define SDR_DMA_DMACR_RESET         (1u << 2)
#define SDR_DMA_DMACR_IOC_IRQ_EN    (1u << 12)
#define SDR_DMA_DMACR_ERR_IRQ_EN    (1u << 14)


// ============================================================
// S2MM DMASR Bits
// ============================================================

#define SDR_DMA_DMASR_HALTED        (1u << 0)
#define SDR_DMA_DMASR_IDLE          (1u << 1)

#define SDR_DMA_DMASR_SG_DEC_ERR    (1u << 3)
#define SDR_DMA_DMASR_DMA_INT_ERR   (1u << 4)
#define SDR_DMA_DMASR_DMA_SLV_ERR   (1u << 5)
#define SDR_DMA_DMASR_DMA_DEC_ERR   (1u << 6)

#define SDR_DMA_DMASR_SG_INT_ERR    (1u << 8)
#define SDR_DMA_DMASR_SG_SLV_ERR    (1u << 9)
#define SDR_DMA_DMASR_SG_DEC_ERR2   (1u << 10)

#define SDR_DMA_DMASR_IOC_IRQ       (1u << 12)
#define SDR_DMA_DMASR_DELAY_IRQ     (1u << 13)
#define SDR_DMA_DMASR_ERR_IRQ       (1u << 14)


// ============================================================
// DMA Initialization / IRQ
// ============================================================

int dma_init(DmaContext *ctx,
             uint32_t base_addr);

int dma_irq_init(DmaContext *ctx,
                 const char *uio_device);

void dma_irq_close(DmaContext *ctx);

void dma_close(DmaContext *ctx);


// ============================================================
// DMA Register Access
// ============================================================

uint32_t dma_read_reg(DmaContext *ctx,
                      uint32_t offset);

void dma_write_reg(DmaContext *ctx,
                   uint32_t offset,
                   uint32_t value);


// ============================================================
// DMA Control
// ============================================================

void dma_reset(DmaContext *ctx);

void dma_start_s2mm(DmaContext *ctx,
                    uint32_t buffer_addr,
                    uint32_t length);

bool dma_wait_for_completion(DmaContext *ctx,
                             uint32_t timeout_ms);

void dma_stop_s2mm(DmaContext *ctx);


// ============================================================
// DMA Status
// ============================================================

uint32_t dma_status(DmaContext *ctx);

void dma_print_status(DmaContext *ctx);


// ============================================================
// DMA Buffer Access
// ============================================================

void dma_dump_buffer(uint32_t buffer_addr,
                     uint32_t length);

const uint8_t *dma_map_buffer(uint32_t buffer_addr,
                              uint32_t length);

void dma_unmap_buffer(const uint8_t *buffer,
                      uint32_t length);


#endif // DMA_H