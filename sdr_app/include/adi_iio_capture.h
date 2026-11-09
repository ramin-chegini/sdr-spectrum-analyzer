#ifndef ADI_IIO_CAPTURE_H
#define ADI_IIO_CAPTURE_H

#include <iio.h>

#include <cstddef>
#include <cstdint>
#include <string>

class ADIIIOCapture
{
public:

    ADIIIOCapture();
    ~ADIIIOCapture();

    // --------------------------------------------------
    // Initialization / Shutdown
    // --------------------------------------------------

    bool init();
    void close();

    bool isInitialized() const;

    // --------------------------------------------------
    // RAW IQ Capture
    //
    // samples = number of IQ samples
    //
    // Output format:
    // I0 Q0 I1 Q1 ...
    // each component = int16_t
    // --------------------------------------------------

    bool capture(
        size_t samples,
        const std::string& filename);

private:

    struct iio_context* ctx_;
    struct iio_device* rx_;

    struct iio_channel* rx_i_;
    struct iio_channel* rx_q_;

    struct iio_buffer* rxbuf_;

    bool initialized_;

    // --------------------------------------------------
    // Helpers
    // --------------------------------------------------

    bool findRxDevice();

    bool setupRxChannels();

    bool createBuffer(
        size_t samples);

    void destroyBuffer();

    bool saveBuffer(
        const std::string& filename,
        size_t samples);
};

#endif