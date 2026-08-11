#ifndef DMA_H
#define DMA_H

#include <stdint.h>

/*--------------------------------------------------------------------
 * AXI DMA Base Address
 *-------------------------------------------------------------------*/
#define SDR_DMA_BASE_ADDR          0x40400000
#define SDR_DMA_MAP_SIZE           0x10000

/*--------------------------------------------------------------------
 * S2MM Register Offsets
 *-------------------------------------------------------------------*/
#define SDR_DMA_S2MM_DMACR             0x30
#define SDR_DMA_S2MM_DMASR             0x34
#define SDR_DMA_S2MM_CURDESC           0x38
#define SDR_DMA_S2MM_CURDESC_MSB       0x3C
#define SDR_DMA_S2MM_TAILDESC          0x40
#define SDR_DMA_S2MM_TAILDESC_MSB      0x44
#define SDR_DMA_S2MM_DA                0x48
#define SDR_DMA_S2MM_DA_MSB            0x4C
#define SDR_DMA_S2MM_LENGTH            0x58
/*----------------------------------------------------------
 * DMACR Bits
 *---------------------------------------------------------*/

#define SDR_DMA_DMASR_HALTED            (1 << 0)
#define SDR_DMA_DMASR_IDLE              (1 << 1)
#define SDR_DMA_DMASR_SG_INCLD          (1 << 3)

#define SDR_DMA_DMASR_DMA_INT_ERR       (1 << 4)
#define SDR_DMA_DMASR_DMA_SLV_ERR       (1 << 5)
#define SDR_DMA_DMASR_DMA_DEC_ERR       (1 << 6)

#define SDR_DMA_DMASR_IOC_IRQ           (1 << 12)
#define SDR_DMA_DMASR_DELAY_IRQ         (1 << 13)
#define SDR_DMA_DMASR_ERR_IRQ           (1 << 14)

#define SDR_DMA_DMACR_RS             (1 << 0)
#define SDR_DMA_DMACR_RESET          (1 << 2)

#define SDR_DMA_DMACR_IOC_IRQ_EN     (1 << 12)
#define SDR_DMA_DMACR_DLY_IRQ_EN     (1 << 13)
#define SDR_DMA_DMACR_ERR_IRQ_EN     (1 << 14)

/*--------------------------------------------------------------------
 * API
 *-------------------------------------------------------------------*/
int dma_init(void);
void dma_close(void);

int dma_irq_init(void);
void dma_irq_close(void);

uint32_t dma_read_reg(uint32_t offset);
void dma_write_reg(uint32_t offset, uint32_t value);

void dma_reset(void);

uint32_t dma_status(void);
void dma_print_status(void);

int dma_start(uint32_t dst_addr, uint32_t length);
int dma_stop(void);

void dma_start_s2mm(uint32_t buffer_addr, uint32_t length);
bool dma_wait_for_irq(uint32_t timeout_ms);
bool dma_wait_for_completion(uint32_t timeout_ms);
void dma_stop_s2mm(void);

#endif /* DMA_H */