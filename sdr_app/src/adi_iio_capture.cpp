#include "adi_iio_capture.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

ADIIIOCapture::ADIIIOCapture()
    : ctx_(nullptr),
      rx_(nullptr),
      rx_i_(nullptr),
      rx_q_(nullptr),
      rxbuf_(nullptr),
      initialized_(false)
{
}

ADIIIOCapture::~ADIIIOCapture()
{
    close();
}

// --------------------------------------------------
// Initialization
// --------------------------------------------------

bool ADIIIOCapture::init()
{
    if (initialized_)
        return true;

    std::cout << "ADI IIO: creating context..." << std::endl;

    ctx_ = iio_create_default_context();

    if (!ctx_) {
        std::cerr
            << "ADI IIO: failed to create context"
            << std::endl;
        return false;
    }

    std::cout
        << "ADI IIO: context created"
        << std::endl;

    if (!findRxDevice()) {
        close();
        return false;
    }

    if (!setupRxChannels()) {
        close();
        return false;
    }

    initialized_ = true;

    std::cout
        << "ADI IIO: RX initialized successfully"
        << std::endl;

    return true;
}

// --------------------------------------------------
// Find RX device
// --------------------------------------------------

bool ADIIIOCapture::findRxDevice()
{
    const unsigned int count =
        iio_context_get_devices_count(ctx_);

    std::cout
        << "ADI IIO: devices = "
        << count
        << std::endl;

    for (unsigned int i = 0; i < count; ++i) {

        struct iio_device* dev =
            iio_context_get_device(ctx_, i);

        if (!dev)
            continue;

        const char* name =
            iio_device_get_name(dev);

        if (!name)
            continue;

        std::cout
            << "  [" << i << "] "
            << name
            << std::endl;

        if (std::strcmp(name, "cf-ad9361-lpc") == 0) {

            rx_ = dev;

            std::cout
                << "ADI IIO: RX device found: "
                << name
                << std::endl;

            return true;
        }
    }

    std::cerr
        << "ADI IIO: cf-ad9361-lpc not found"
        << std::endl;

    return false;
}

// --------------------------------------------------
// Setup RX channels
// --------------------------------------------------

bool ADIIIOCapture::setupRxChannels()
{
    if (!rx_)
        return false;

    rx_i_ =
        iio_device_find_channel(
            rx_,
            "voltage0",
            false);

    rx_q_ =
        iio_device_find_channel(
            rx_,
            "voltage1",
            false);

    if (!rx_i_) {

        std::cerr
            << "ADI IIO: voltage0 not found"
            << std::endl;

        return false;
    }

    if (!rx_q_) {

        std::cerr
            << "ADI IIO: voltage1 not found"
            << std::endl;

        return false;
    }

    std::cout
        << "ADI IIO: RX I channel found"
        << std::endl;

    std::cout
        << "ADI IIO: RX Q channel found"
        << std::endl;

    iio_channel_enable(rx_i_);
    iio_channel_enable(rx_q_);

    std::cout
        << "ADI IIO: RX channels enabled"
        << std::endl;

    return true;
}

// --------------------------------------------------
// Create buffer
// --------------------------------------------------

bool ADIIIOCapture::createBuffer(size_t samples)
{
    if (!rx_)
        return false;

    rxbuf_ =
        iio_device_create_buffer(
            rx_,
            samples,
            false);

    if (!rxbuf_) {

        std::cerr
            << "ADI IIO: failed to create RX buffer"
            << std::endl;

        return false;
    }

    std::cout
        << "ADI IIO: RX buffer created"
        << std::endl;

    return true;
}

// --------------------------------------------------
// Capture
// --------------------------------------------------

bool ADIIIOCapture::capture(
    size_t samples,
    const std::string& filename)
{
    if (!initialized_) {

        std::cerr
            << "ADI IIO: not initialized"
            << std::endl;

        return false;
    }

    if (samples == 0) {

        std::cerr
            << "ADI IIO: invalid sample count"
            << std::endl;

        return false;
    }

    if (!createBuffer(samples))
        return false;

    std::cout
        << "ADI IIO: refilling RX buffer..."
        << std::endl;

    const ssize_t nbytes =
        iio_buffer_refill(rxbuf_);

    if (nbytes < 0) {

        std::cerr
            << "ADI IIO: buffer refill failed: "
            << nbytes
            << std::endl;

        destroyBuffer();
        return false;
    }

    std::cout
        << "ADI IIO: received "
        << nbytes
        << " bytes"
        << std::endl;

    const char* first =
        static_cast<const char*>(
            iio_buffer_first(rxbuf_, rx_i_));

    const char* end =
        static_cast<const char*>(
            iio_buffer_end(rxbuf_));

    const ptrdiff_t step =
        iio_buffer_step(rxbuf_);

    if (!first || !end || step <= 0) {

        std::cerr
            << "ADI IIO: invalid buffer layout"
            << std::endl;

        destroyBuffer();
        return false;
    }

    std::ofstream file(
        filename,
        std::ios::binary);

    if (!file) {

        std::cerr
            << "ADI IIO: cannot open "
            << filename
            << std::endl;

        destroyBuffer();
        return false;
    }

    size_t count = 0;

    for (const char* p = first;
         p < end && count < samples;
         p += step) {

        const int16_t i =
            reinterpret_cast<const int16_t*>(p)[0];

        const int16_t q =
            reinterpret_cast<const int16_t*>(p)[1];

        file.write(
            reinterpret_cast<const char*>(&i),
            sizeof(i));

        file.write(
            reinterpret_cast<const char*>(&q),
            sizeof(q));

        ++count;
    }

    file.close();

    std::cout
        << "ADI IIO: file closed"
        << std::endl;

    std::cout
        << "ADI IIO: destroying buffer..."
        << std::endl;

    destroyBuffer();

    std::cout
        << "ADI IIO: buffer destroyed"
        << std::endl;

    std::cout
        << "ADI IIO: capture completed"
        << std::endl;

    std::cout
        << "Samples : "
        << count
        << std::endl;

    std::cout
        << "Bytes   : "
        << count * sizeof(int16_t) * 2
        << std::endl;

    std::cout
        << "File    : "
        << filename
        << std::endl;

    return count == samples;
}

// --------------------------------------------------
// Destroy buffer
// --------------------------------------------------

void ADIIIOCapture::destroyBuffer()
{
    if (rxbuf_) {

        iio_buffer_destroy(rxbuf_);

        rxbuf_ = nullptr;
    }
}

// --------------------------------------------------
// Close
// --------------------------------------------------

void ADIIIOCapture::close()
{

    std::cout
    << "ADI IIO: close()"
    << std::endl;

    destroyBuffer();

    std::cout
    << "ADI IIO: buffer cleanup done"
    << std::endl;

    if (rx_) {

        std::cout
        << "ADI IIO: disabling channels..."
        << std::endl;

        iio_channel_disable(rx_i_);
        iio_channel_disable(rx_q_);

        std::cout
        << "ADI IIO: channels disabled"
        << std::endl;
    }

    rx_i_ = nullptr;
    rx_q_ = nullptr;
    rx_ = nullptr;

    if (ctx_) {

        iio_context_destroy(ctx_);

        ctx_ = nullptr;
    }

    initialized_ = false;
}

// --------------------------------------------------
// Status
// --------------------------------------------------

bool ADIIIOCapture::isInitialized() const
{
    return initialized_;
}