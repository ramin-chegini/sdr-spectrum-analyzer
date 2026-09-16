#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include "dma.h"

/*
 * ---------------------------------------------------------
 * One-Shot Audio DMA Capture + L2 Send
 * ---------------------------------------------------------
 */
bool audio_dma_capture_and_send(DmaContext *audio_dma);

/*
 * ---------------------------------------------------------
 * Continuous Audio Streaming
 * ---------------------------------------------------------
 *
 * start:
 *     Start Audio streaming in background.
 *
 * stop:
 *     Stop background Audio streaming.
 *
 * status:
 *     Check whether Audio streaming is running.
 */
bool audio_stream_start(DmaContext *audio_dma);
bool audio_stream_stop();
bool audio_stream_is_running();

#endif