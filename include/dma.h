#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>

/*==========================================================
 * AXI DMA Base Configuration
 *=========================================================*/

#define SDR_DMA_BASE_ADDR       0x40400000
#define SDR_DMA_MAP_SIZE       0x00010000

/*==========================================================
 * S2MM Register Offsets
 *=========================================================*/

#define SDR_DMA_S2MM_DMACR     0x30
#define SDR_DMA_S2MM_DMASR     0x34
#define SDR_DMA_S2MM_DA        0x48
#define SDR_DMA_S2MM_LENGTH    0x58

/*==========================================================
 * DMACR Bits
 *=========================================================*/

#define SDR_DMA_DMACR_RS            (1u << 0)
#define SDR_DMA_DMACR_RESET         (1u << 2)
#define SDR_DMA_DMACR_IOC_IRQ_EN    (1u << 12)
#define SDR_DMA_DMACR_ERR_IRQ_EN    (1u << 14)

/*==========================================================
 * DMASR Bits
 *=========================================================*/

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

/*==========================================================
 * DMA Initialization
 *=========================================================*/

int dma_init(void);

int dma_irq_init(void);

void dma_irq_close(void);

void dma_close(void);

/*==========================================================
 * DMA Register Access
 *=========================================================*/

uint32_t dma_read_reg(uint32_t offset);

void dma_write_reg(uint32_t offset, uint32_t value);

/*==========================================================
 * DMA Control
 *=========================================================*/

void dma_reset(void);

void dma_start_s2mm(uint32_t buffer_addr, uint32_t length);

bool dma_wait_for_completion(uint32_t timeout_ms);

void dma_stop_s2mm(void);

/*==========================================================
 * DMA Status
 *=========================================================*/

uint32_t dma_status(void);

void dma_print_status(void);

/*==========================================================
 * DMA Buffer Debug
 *=========================================================*/

void dma_dump_buffer(uint32_t buffer_addr, uint32_t length);

#endif