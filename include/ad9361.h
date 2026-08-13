#ifndef AD9361_H
#define AD9361_H

#include <iio.h>

#include <cstdint>
#include <cstddef>

class AD9361Controller
{
public:

    struct AD9361Status
    {
        uint64_t rxFrequency;
        uint64_t txFrequency;

        uint64_t rxBandwidth;
        uint64_t txBandwidth;

        uint64_t rxSampleRate;
        uint64_t txSampleRate;

        double rx1Gain;
        double rx2Gain;

        double rx1Rssi;
        double rx2Rssi;

        char rx1GainMode[32];
        char rx2GainMode[32];

        char rx1RfPort[64];
        char rx2RfPort[64];

        double tx1Attenuation;
        double tx2Attenuation;

        char tx1RfPort[32];
        char tx2RfPort[32];

        char ensmMode[32];
        char calibrationMode[64];

        char rxPathRates[256];
        char txPathRates[256];
    };

    AD9361Controller();
    ~AD9361Controller();

    // --------------------------------------------------
    // Initialization / Shutdown
    // --------------------------------------------------

    bool init();
    void close();

    bool isInitialized() const;
    bool getStatus(AD9361Status& status);

    // --------------------------------------------------
    // RX LO
    // --------------------------------------------------

    bool setRxFrequency(uint64_t hz);
    bool getRxFrequency(uint64_t& hz);

    // --------------------------------------------------
    // TX LO
    // --------------------------------------------------

    bool setTxFrequency(uint64_t hz);
    bool getTxFrequency(uint64_t& hz);

    // --------------------------------------------------
    // RX Bandwidth
    // --------------------------------------------------

    bool setRxBandwidth(uint64_t hz);
    bool getRxBandwidth(uint64_t& hz);

    // --------------------------------------------------
    // TX Bandwidth
    // --------------------------------------------------

    bool setTxBandwidth(uint64_t hz);
    bool getTxBandwidth(uint64_t& hz);

    // --------------------------------------------------
    // RX Sample Rate
    // --------------------------------------------------

    bool setRxSampleRate(uint64_t hz);
    bool getRxSampleRate(uint64_t& hz);

    // --------------------------------------------------
    // TX Sample Rate
    // --------------------------------------------------

    bool setTxSampleRate(uint64_t hz);
    bool getTxSampleRate(uint64_t& hz);

    // --------------------------------------------------
    // RX Gain
    // channel = 1 or 2
    // --------------------------------------------------

    bool setRxGain(
        int channel,
        double gainDb);

    bool getRxGain(
        int channel,
        double& gainDb);

    // --------------------------------------------------
    // RX Gain Control Mode / AGC
    // --------------------------------------------------

    bool setRxGainMode(
        int channel,
        const char* mode);

    bool getRxGainMode(
        int channel,
        char* mode,
        size_t size);

    // --------------------------------------------------
    // RX RSSI
    // --------------------------------------------------

    bool getRxRssi(
        int channel,
        double& rssiDb);

    // --------------------------------------------------
    // RX RF Port
    // channel = 1 or 2
    // --------------------------------------------------

    bool setRxRfPort(
        int channel,
        const char* port);

    bool getRxRfPort(
        int channel,
        char* port,
        size_t size);

    // --------------------------------------------------
    // TX Attenuation
    // channel = 1 or 2
    // --------------------------------------------------

    bool setTxAttenuation(
        int channel,
        double db);

    bool getTxAttenuation(
        int channel,
        double& db);

    // --------------------------------------------------
    // TX RF Port
    // channel = 1 or 2
    // --------------------------------------------------

    bool setTxRfPort(
        int channel,
        const char* port);

    bool getTxRfPort(
        int channel,
        char* port,
        size_t size);

    // --------------------------------------------------
    // ENSM
    // --------------------------------------------------

    bool setEnsmMode(
        const char* mode);

    bool getEnsmMode(
        char* mode,
        size_t size);

    // --------------------------------------------------
    // Calibration
    // --------------------------------------------------

    bool setCalibrationMode(
        const char* mode);

    bool getCalibrationMode(
        char* mode,
        size_t size);

    // --------------------------------------------------
    // Path Rates
    // --------------------------------------------------

    bool getRxPathRates(
        char* buffer,
        size_t size);

    bool getTxPathRates(
        char* buffer,
        size_t size);

private:

    // --------------------------------------------------
    // IIO objects
    // --------------------------------------------------

    struct iio_context* ctx_;
    struct iio_device* phy_;

    struct iio_channel* rx1_;
    struct iio_channel* rx2_;

    struct iio_channel* tx1_;
    struct iio_channel* tx2_;

    struct iio_channel* rxLo_;
    struct iio_channel* txLo_;

    bool initialized_;

    // --------------------------------------------------
    // Channel helpers
    // --------------------------------------------------

    struct iio_channel* findRxChannel(
        int channel);

    struct iio_channel* findTxChannel(
        int channel);

    // --------------------------------------------------
    // Device attributes
    // --------------------------------------------------

    bool writeDeviceAttr(
        const char* attr,
        const char* value);

    bool writeDeviceAttr(
        const char* attr,
        long long value);

    bool readDeviceAttr(
        const char* attr,
        char* buffer,
        size_t size);

    // --------------------------------------------------
    // Channel attributes
    // --------------------------------------------------

    bool writeChannelAttr(
        struct iio_channel* channel,
        const char* attr,
        const char* value);

    bool writeChannelAttr(
        struct iio_channel* channel,
        const char* attr,
        long long value);

    bool readChannelAttr(
        struct iio_channel* channel,
        const char* attr,
        char* buffer,
        size_t size);
};

#endif