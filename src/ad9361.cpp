#include "ad9361.h"

#include <iio.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>


// ============================================================
// Constructor / Destructor
// ============================================================

AD9361Controller::AD9361Controller()
    : ctx_(nullptr),
      phy_(nullptr),
      rx1_(nullptr),
      rx2_(nullptr),
      tx1_(nullptr),
      tx2_(nullptr),
      rxLo_(nullptr),
      txLo_(nullptr),
      initialized_(false)
{
}


AD9361Controller::~AD9361Controller()
{
    close();
}


// ============================================================
// Initialization
// ============================================================

bool AD9361Controller::init()
{
    if (initialized_)
        return true;

    // Create local IIO context
    ctx_ = iio_create_local_context();

    if (!ctx_) {
        std::fprintf(stderr,
                     "AD9361: failed to create local IIO context\n");
        return false;
    }

    // Find AD9361 PHY
    phy_ = iio_context_find_device(ctx_, "ad9361-phy");

    if (!phy_) {
        std::fprintf(stderr,
                     "AD9361: device 'ad9361-phy' not found\n");

        close();
        return false;
    }

    // --------------------------------------------------------
    // RX channels
    // --------------------------------------------------------

    rx1_ = iio_device_find_channel(
        phy_,
        "voltage0",
        false
    );

    rx2_ = iio_device_find_channel(
        phy_,
        "voltage1",
        false
    );

    // --------------------------------------------------------
    // TX channels
    // --------------------------------------------------------

    tx1_ = iio_device_find_channel(
        phy_,
        "voltage0",
        true
    );

    tx2_ = iio_device_find_channel(
        phy_,
        "voltage1",
        true
    );

    // --------------------------------------------------------
    // LO channels
    // --------------------------------------------------------

    rxLo_ = iio_device_find_channel(
        phy_,
        "altvoltage0",
        true
    );

    txLo_ = iio_device_find_channel(
        phy_,
        "altvoltage1",
        true
    );

    if (!rx1_ || !rx2_) {
        std::fprintf(stderr,
                     "AD9361: RX channels not found\n");

        close();
        return false;
    }

    if (!tx1_ || !tx2_) {
        std::fprintf(stderr,
                     "AD9361: TX channels not found\n");

        close();
        return false;
    }

    if (!rxLo_ || !txLo_) {
        std::fprintf(stderr,
                     "AD9361: LO channels not found\n");

        close();
        return false;
    }

    initialized_ = true;

    std::printf("AD9361: initialized successfully\n");

    return true;
}


// ============================================================
// Close
// ============================================================

void AD9361Controller::close()
{
    if (ctx_) {
        iio_context_destroy(ctx_);
    }

    ctx_ = nullptr;
    phy_ = nullptr;

    rxLo_ = nullptr;
    txLo_ = nullptr;

    rx1_ = nullptr;
    rx2_ = nullptr;

    tx1_ = nullptr;
    tx2_ = nullptr;

    initialized_ = false;
}


// ============================================================
// Status
// ============================================================

bool AD9361Controller::isInitialized() const
{
    return initialized_;
}


// ============================================================
// Generic Device Attribute - String Write
// ============================================================

bool AD9361Controller::writeDeviceAttr(
    const char *attr,
    const char *value)
{
    if (!initialized_ || !phy_ || !attr || !value)
        return false;

    int ret = iio_device_attr_write(
        phy_,
        attr,
        value
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to write device attr '%s'\n",
                     attr);
        return false;
    }

    return true;
}


// ============================================================
// Generic Device Attribute - Integer Write
// ============================================================

bool AD9361Controller::writeDeviceAttr(
    const char *attr,
    long long value)
{
    if (!initialized_ || !phy_ || !attr)
        return false;

    int ret = iio_device_attr_write_longlong(
        phy_,
        attr,
        value
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to write device attr '%s'\n",
                     attr);
        return false;
    }

    return true;
}


// ============================================================
// Generic Device Attribute - Read
// ============================================================

bool AD9361Controller::readDeviceAttr(
    const char *attr,
    char *buffer,
    size_t size)
{
    if (!initialized_ ||
        !phy_ ||
        !attr ||
        !buffer ||
        size == 0)
        return false;

    ssize_t ret = iio_device_attr_read(
        phy_,
        attr,
        buffer,
        size
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to read device attr '%s'\n",
                     attr);
        return false;
    }

    buffer[size - 1] = '\0';

    return true;
}


// ============================================================
// Generic Channel Attribute - String Write
// ============================================================

bool AD9361Controller::writeChannelAttr(
    struct iio_channel *channel,
    const char *attr,
    const char *value)
{
    if (!initialized_ ||
        !channel ||
        !attr ||
        !value)
        return false;

    int ret = iio_channel_attr_write(
        channel,
        attr,
        value
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to write channel attr '%s'\n",
                     attr);
        return false;
    }

    return true;
}


// ============================================================
// Generic Channel Attribute - Integer Write
// ============================================================

bool AD9361Controller::writeChannelAttr(
    struct iio_channel *channel,
    const char *attr,
    long long value)
{
    if (!initialized_ ||
        !channel ||
        !attr)
        return false;

    int ret = iio_channel_attr_write_longlong(
        channel,
        attr,
        value
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to write channel attr '%s'\n",
                     attr);
        return false;
    }

    return true;
}


// ============================================================
// Generic Channel Attribute - Read
// ============================================================

bool AD9361Controller::readChannelAttr(
    struct iio_channel *channel,
    const char *attr,
    char *buffer,
    size_t size)
{
    if (!initialized_ ||
        !channel ||
        !attr ||
        !buffer ||
        size == 0)
        return false;

    ssize_t ret = iio_channel_attr_read(
        channel,
        attr,
        buffer,
        size
    );

    if (ret < 0) {
        std::fprintf(stderr,
                     "AD9361: failed to read channel attr '%s'\n",
                     attr);
        return false;
    }

    buffer[size - 1] = '\0';

    return true;
}


// ============================================================
// Find RX Channel
// ============================================================

struct iio_channel *
AD9361Controller::findRxChannel(int channel)
{
    switch (channel) {

    case 1:
        return rx1_;

    case 2:
        return rx2_;

    default:
        return nullptr;
    }
}


// ============================================================
// Find TX Channel
// ============================================================

struct iio_channel *
AD9361Controller::findTxChannel(int channel)
{
    switch (channel) {

    case 1:
        return tx1_;

    case 2:
        return tx2_;

    default:
        return nullptr;
    }
}


// ============================================================
// RX Frequency
//
// Shared between RX1 and RX2
// ============================================================

bool AD9361Controller::setRxFrequency(uint64_t hz)
{
    return writeChannelAttr(
        rxLo_,
        "frequency",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getRxFrequency(uint64_t &hz)
{
    if (!initialized_ || !rxLo_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        rxLo_,
        "frequency",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// TX Frequency
//
// Shared between TX1 and TX2
// ============================================================

bool AD9361Controller::setTxFrequency(uint64_t hz)
{
    return writeChannelAttr(
        txLo_,
        "frequency",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getTxFrequency(uint64_t &hz)
{
    if (!initialized_ || !txLo_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        txLo_,
        "frequency",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// RX Bandwidth
//
// Shared between RX1 and RX2
// ============================================================

bool AD9361Controller::setRxBandwidth(uint64_t hz)
{
    return writeChannelAttr(
        rx1_,
        "rf_bandwidth",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getRxBandwidth(uint64_t &hz)
{
    if (!initialized_ || !rx1_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        rx1_,
        "rf_bandwidth",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// TX Bandwidth
//
// Shared between TX1 and TX2
// ============================================================

bool AD9361Controller::setTxBandwidth(uint64_t hz)
{
    return writeChannelAttr(
        tx1_,
        "rf_bandwidth",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getTxBandwidth(uint64_t &hz)
{
    if (!initialized_ || !tx1_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        tx1_,
        "rf_bandwidth",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// RX Sample Rate
//
// Shared between RX1 and RX2
// ============================================================

bool AD9361Controller::setRxSampleRate(uint64_t hz)
{
    return writeChannelAttr(
        rx1_,
        "sampling_frequency",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getRxSampleRate(uint64_t &hz)
{
    if (!initialized_ || !rx1_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        rx1_,
        "sampling_frequency",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// TX Sample Rate
//
// Shared between TX1 and TX2
// ============================================================

bool AD9361Controller::setTxSampleRate(uint64_t hz)
{
    return writeChannelAttr(
        tx1_,
        "sampling_frequency",
        static_cast<long long>(hz)
    );
}


bool AD9361Controller::getTxSampleRate(uint64_t &hz)
{
    if (!initialized_ || !tx1_)
        return false;

    long long value = 0;

    int ret = iio_channel_attr_read_longlong(
        tx1_,
        "sampling_frequency",
        &value
    );

    if (ret < 0)
        return false;

    hz = static_cast<uint64_t>(value);

    return true;
}


// ============================================================
// RX Gain
// ============================================================

bool AD9361Controller::setRxGain(
    int channel,
    double gainDb)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch)
        return false;

    char value[64];

    std::snprintf(
        value,
        sizeof(value),
        "%.2f",
        gainDb
    );

    return writeChannelAttr(
        ch,
        "hardwaregain",
        value
    );
}


bool AD9361Controller::getRxGain(
    int channel,
    double &gainDb)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch)
        return false;

    char value[64];

    if (!readChannelAttr(
            ch,
            "hardwaregain",
            value,
            sizeof(value)))
        return false;

    gainDb = std::strtod(value, nullptr);

    return true;
}


// ============================================================
// RX Gain Control Mode
// ============================================================

bool AD9361Controller::setRxGainMode(
    int channel,
    const char *mode)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch || !mode)
        return false;

    return writeChannelAttr(
        ch,
        "gain_control_mode",
        mode
    );
}


bool AD9361Controller::getRxGainMode(
    int channel,
    char *mode,
    size_t size)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch)
        return false;

    return readChannelAttr(
        ch,
        "gain_control_mode",
        mode,
        size
    );
}


// ============================================================
// RX RSSI
// ============================================================

bool AD9361Controller::getRxRssi(
    int channel,
    double &rssiDb)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch)
        return false;

    char value[128];

    if (!readChannelAttr(
            ch,
            "rssi",
            value,
            sizeof(value)))
        return false;

    rssiDb = std::strtod(value, nullptr);

    return true;
}


// ============================================================
// RX RF Port
// ============================================================

bool AD9361Controller::setRxRfPort(
    int channel,
    const char *port)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch || !port)
        return false;

    return writeChannelAttr(
        ch,
        "rf_port_select",
        port
    );
}

bool AD9361Controller::getRxRfPort(
    int channel,
    char *port,
    size_t size)
{
    struct iio_channel *ch = findRxChannel(channel);

    if (!ch || !port || size == 0)
        return false;

    return readChannelAttr(
        ch,
        "rf_port_select",
        port,
        size
    );
}

// ============================================================
// TX Attenuation
// ============================================================

bool AD9361Controller::setTxAttenuation(
    int channel,
    double db)
{
    struct iio_channel *ch = findTxChannel(channel);

    if (!ch)
        return false;

    char value[64];

    std::snprintf(
        value,
        sizeof(value),
        "%.2f",
        db
    );

    return writeChannelAttr(
        ch,
        "hardwaregain",
        value
    );
}


bool AD9361Controller::getTxAttenuation(
    int channel,
    double &db)
{
    struct iio_channel *ch = findTxChannel(channel);

    if (!ch)
        return false;

    char value[64];

    if (!readChannelAttr(
            ch,
            "hardwaregain",
            value,
            sizeof(value)))
        return false;

    db = std::strtod(value, nullptr);

    return true;
}


// ============================================================
// TX RF Port
// ============================================================

bool AD9361Controller::setTxRfPort(
    int channel,
    const char *port)
{
    struct iio_channel *ch = findTxChannel(channel);

    if (!ch || !port)
        return false;

    return writeChannelAttr(
        ch,
        "rf_port_select",
        port
    );
}

bool AD9361Controller::getTxRfPort(
    int channel,
    char *port,
    size_t size)
{
    struct iio_channel *ch = findTxChannel(channel);

    if (!ch || !port || size == 0)
        return false;

    return readChannelAttr(
        ch,
        "rf_port_select",
        port,
        size
    );
}


// ============================================================
// ENSM
// ============================================================

bool AD9361Controller::setEnsmMode(
    const char *mode)
{
    return writeDeviceAttr(
        "ensm_mode",
        mode
    );
}


bool AD9361Controller::getEnsmMode(
    char *mode,
    size_t size)
{
    return readDeviceAttr(
        "ensm_mode",
        mode,
        size
    );
}


// ============================================================
// Calibration
// ============================================================

bool AD9361Controller::setCalibrationMode(
    const char *mode)
{
    return writeDeviceAttr(
        "calib_mode",
        mode
    );
}


bool AD9361Controller::getCalibrationMode(
    char *mode,
    size_t size)
{
    return readDeviceAttr(
        "calib_mode",
        mode,
        size
    );
}


// ============================================================
// Path Rates
// ============================================================

bool AD9361Controller::getRxPathRates(
    char *buffer,
    size_t size)
{
    return readDeviceAttr(
        "rx_path_rates",
        buffer,
        size
    );
}


bool AD9361Controller::getTxPathRates(
    char *buffer,
    size_t size)
{
    return readDeviceAttr(
        "tx_path_rates",
        buffer,
        size
    );
}

bool AD9361Controller::getStatus(AD9361Status& status)
{
    if (!initialized_)
        return false;

    status.rxFrequency = 0;
    status.txFrequency = 0;

    status.rxBandwidth = 0;
    status.txBandwidth = 0;

    status.rxSampleRate = 0;
    status.txSampleRate = 0;

    status.rx1Gain = 0.0;
    status.rx2Gain = 0.0;

    status.rx1Rssi = 0.0;
    status.rx2Rssi = 0.0;

    status.tx1Attenuation = 0.0;
    status.tx2Attenuation = 0.0;

    status.rx1GainMode[0] = '\0';
    status.rx2GainMode[0] = '\0';

    status.rx1RfPort[0] = '\0';
    status.rx2RfPort[0] = '\0';

    status.tx1RfPort[0] = '\0';
    status.tx2RfPort[0] = '\0';

    status.ensmMode[0] = '\0';
    status.calibrationMode[0] = '\0';

    status.rxPathRates[0] = '\0';
    status.txPathRates[0] = '\0';


    // --------------------------------------------------
    // RX / TX frequency
    // --------------------------------------------------

    if (!getRxFrequency(status.rxFrequency))
        return false;

    if (!getTxFrequency(status.txFrequency))
        return false;


    // --------------------------------------------------
    // RX / TX bandwidth
    // --------------------------------------------------

    if (!getRxBandwidth(status.rxBandwidth))
        return false;

    if (!getTxBandwidth(status.txBandwidth))
        return false;


    // --------------------------------------------------
    // RX / TX sample rate
    // --------------------------------------------------

    if (!getRxSampleRate(status.rxSampleRate))
        return false;

    if (!getTxSampleRate(status.txSampleRate))
        return false;


    // --------------------------------------------------
    // RX gains
    // --------------------------------------------------

    if (!getRxGain(1, status.rx1Gain))
        return false;

    if (!getRxGain(2, status.rx2Gain))
        return false;


    // --------------------------------------------------
    // RX gain modes
    // --------------------------------------------------

    if (!getRxGainMode(
            1,
            status.rx1GainMode,
            sizeof(status.rx1GainMode)))
        return false;

    if (!getRxGainMode(
            2,
            status.rx2GainMode,
            sizeof(status.rx2GainMode)))
        return false;


    // --------------------------------------------------
    // RX RSSI
    // --------------------------------------------------

    if (!getRxRssi(1, status.rx1Rssi))
        return false;

    if (!getRxRssi(2, status.rx2Rssi))
        return false;


    // --------------------------------------------------
    // RX RF ports
    // --------------------------------------------------

    if (!getRxRfPort(
            1,
            status.rx1RfPort,
            sizeof(status.rx1RfPort)))
        return false;

    if (!getRxRfPort(
            2,
            status.rx2RfPort,
            sizeof(status.rx2RfPort)))
        return false;


    // --------------------------------------------------
    // TX attenuation
    // --------------------------------------------------

    if (!getTxAttenuation(1, status.tx1Attenuation))
        return false;

    if (!getTxAttenuation(2, status.tx2Attenuation))
        return false;


    // --------------------------------------------------
    // TX RF ports
    // --------------------------------------------------

    if (!getTxRfPort(
            1,
            status.tx1RfPort,
            sizeof(status.tx1RfPort)))
        return false;

    if (!getTxRfPort(
            2,
            status.tx2RfPort,
            sizeof(status.tx2RfPort)))
        return false;


    // --------------------------------------------------
    // ENSM
    // --------------------------------------------------

    if (!getEnsmMode(
            status.ensmMode,
            sizeof(status.ensmMode)))
        return false;


    // --------------------------------------------------
    // Calibration
    // --------------------------------------------------

    if (!getCalibrationMode(
            status.calibrationMode,
            sizeof(status.calibrationMode)))
        return false;


    // --------------------------------------------------
    // Path rates
    // --------------------------------------------------

    if (!getRxPathRates(
            status.rxPathRates,
            sizeof(status.rxPathRates)))
        return false;

    if (!getTxPathRates(
            status.txPathRates,
            sizeof(status.txPathRates)))
        return false;


    return true;
}