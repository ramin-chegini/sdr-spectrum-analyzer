#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <string>

#include "RegisterController.h"
#include "dma.h"
#include "ad9361.h"
#include "adi_iio_capture.h"

using namespace std;

static void printHelp()
{
    cout << "Usage:" << endl;
    cout << endl;

    cout << "Register Controller Commands:" << endl;
    cout << "  sdr_app enable" << endl;
    cout << "  sdr_app disable" << endl;
    cout << "  sdr_app reset" << endl;
    cout << "  sdr_app mode <value>" << endl;
    cout << "  sdr_app filter <value>" << endl;
    cout << "  sdr_app param0 <value>" << endl;
    cout << "  sdr_app param1 <value>" << endl;
    cout << "  sdr_app status" << endl;

    cout << endl;

    cout << "DMA Commands:" << endl;
    cout << "  sdr_app dma" << endl;
    cout << "  sdr_app dma reset" << endl;
    cout << "  sdr_app dma capture" << endl;

    cout << endl;

    cout << "AD9361 Commands:" << endl;
    cout << "  sdr_app ad9361 status" << endl;

    cout << endl;

    cout << "RX Commands:" << endl;
    cout << "  sdr_app ad9361 rx frequency" << endl;
    cout << "  sdr_app ad9361 rx frequency <Hz>" << endl;

    cout << "  sdr_app ad9361 rx bandwidth" << endl;
    cout << "  sdr_app ad9361 rx bandwidth <Hz>" << endl;

    cout << "  sdr_app ad9361 rx sample-rate" << endl;
    cout << "  sdr_app ad9361 rx sample-rate <Hz>" << endl;
    cout << "  sdr_app ad9361 rx capture <samples> [filename]" << endl;

    cout << endl;

    cout << "RX1 / RX2 Commands:" << endl;
    cout << "  sdr_app ad9361 rx1 gain" << endl;
    cout << "  sdr_app ad9361 rx1 gain <dB>" << endl;
    cout << "  sdr_app ad9361 rx1 gain-mode" << endl;
    cout << "  sdr_app ad9361 rx1 gain-mode <mode>" << endl;
    cout << "  sdr_app ad9361 rx1 agc <mode>" << endl;
    cout << "  sdr_app ad9361 rx1 rssi" << endl;
    cout << "  sdr_app ad9361 rx2 gain" << endl;
    cout << "  sdr_app ad9361 rx2 gain <dB>" << endl;
    cout << "  sdr_app ad9361 rx2 gain-mode" << endl;
    cout << "  sdr_app ad9361 rx2 gain-mode <mode>" << endl;
    cout << "  sdr_app ad9361 rx2 agc <mode>" << endl;
    cout << "  sdr_app ad9361 rx2 rssi" << endl;
    cout << "  sdr_app ad9361 rx1 rf-port" << endl;
    cout << "  sdr_app ad9361 rx1 rf-port <port>" << endl;
    cout << "  sdr_app ad9361 rx2 rf-port" << endl;
    cout << "  sdr_app ad9361 rx2 rf-port <port>" << endl;

    cout << endl;

    cout << "TX Commands:" << endl;
    cout << "  sdr_app ad9361 tx frequency" << endl;
    cout << "  sdr_app ad9361 tx frequency <Hz>" << endl;
    cout << "  sdr_app ad9361 tx bandwidth" << endl;
    cout << "  sdr_app ad9361 tx bandwidth <Hz>" << endl;
    cout << "  sdr_app ad9361 tx sample-rate" << endl;
    cout << "  sdr_app ad9361 tx sample-rate <Hz>" << endl;
    cout << "  sdr_app ad9361 tx1 attenuation" << endl;
    cout << "  sdr_app ad9361 tx1 attenuation <dB>" << endl;
    cout << "  sdr_app ad9361 tx2 attenuation" << endl;
    cout << "  sdr_app ad9361 tx2 attenuation <dB>" << endl;
    cout << "  sdr_app ad9361 tx1 rf-port" << endl;
    cout << "  sdr_app ad9361 tx1 rf-port <port>" << endl;
    cout << "  sdr_app ad9361 tx2 rf-port" << endl;
    cout << "  sdr_app ad9361 tx2 rf-port <port>" << endl;
}

static bool writeCaptureMetadata(
    const std::string& filename,
    size_t samples,
    size_t bytes,
    AD9361Controller& ad9361)
{
    uint64_t rxFrequency = 0;
    uint64_t rxBandwidth = 0;
    uint64_t rxSampleRate = 0;

    double rx1Gain = 0.0;
    double rx2Gain = 0.0;

    char rx1GainMode[64] = {};
    char rx2GainMode[64] = {};

    char rx1RfPort[64] = {};
    char rx2RfPort[64] = {};

    if (!ad9361.getRxFrequency(rxFrequency))
        return false;

    if (!ad9361.getRxBandwidth(rxBandwidth))
        return false;

    if (!ad9361.getRxSampleRate(rxSampleRate))
        return false;

    if (!ad9361.getRxGain(1, rx1Gain))
        return false;

    if (!ad9361.getRxGain(2, rx2Gain))
        return false;

    if (!ad9361.getRxGainMode(
            1,
            rx1GainMode,
            sizeof(rx1GainMode)))
        return false;

    if (!ad9361.getRxGainMode(
            2,
            rx2GainMode,
            sizeof(rx2GainMode)))
        return false;

    if (!ad9361.getRxRfPort(
            1,
            rx1RfPort,
            sizeof(rx1RfPort)))
        return false;

    if (!ad9361.getRxRfPort(
            2,
            rx2RfPort,
            sizeof(rx2RfPort)))
        return false;

    std::string metadataFile = filename + ".json";

    std::ofstream file(
        metadataFile,
        std::ios::out | std::ios::trunc);

    if (!file)
    {
        std::cerr
            << "Failed to create metadata file: "
            << metadataFile
            << std::endl;

        return false;
    }

    file << "{\n";

    file << "  \"samples\": "
         << samples << ",\n";

    file << "  \"bytes\": "
         << bytes << ",\n";

    file << "  \"format\": \"S12/16\",\n";

    file << "  \"endianness\": \"little\",\n";

    file << "  \"channels\": \"I,Q\",\n";

    file << "  \"rx_frequency_hz\": "
         << rxFrequency << ",\n";

    file << "  \"rx_bandwidth_hz\": "
         << rxBandwidth << ",\n";

    file << "  \"rx_sample_rate_hz\": "
         << rxSampleRate << ",\n";

    file << "  \"rx1_gain_db\": "
         << rx1Gain << ",\n";

    file << "  \"rx1_gain_mode\": \""
         << rx1GainMode
         << "\",\n";

    file << "  \"rx1_rf_port\": \""
         << rx1RfPort
         << "\",\n";

    file << "  \"rx2_gain_db\": "
         << rx2Gain << ",\n";

    file << "  \"rx2_gain_mode\": \""
         << rx2GainMode
         << "\",\n";

    file << "  \"rx2_rf_port\": \""
         << rx2RfPort
         << "\"\n";

    file << "}\n";

    file.close();

    std::cout
        << "Metadata : "
        << metadataFile
        << std::endl;

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printHelp();
        return 0;
    }

    string cmd(argv[1]);

    RegisterController fpga;

    /*
     * =========================================================
     * AD9361 Commands
     * =========================================================
     */

    if (cmd == "ad9361")
    {
        AD9361Controller ad9361;

        if (!ad9361.init())
        {
            cout << "AD9361 initialization failed." << endl;
            return -1;
        }

        if (argc < 3)
        {
            printHelp();
            return 0;
        }

        string subcmd(argv[2]);

        /*
         * -----------------------------------------------------
         * AD9361 STATUS
         * -----------------------------------------------------
         */

    if (subcmd == "status")
    {
        AD9361Controller::AD9361Status status;

        if (!ad9361.getStatus(status))
        {
            cout << "Failed to read AD9361 status"
                << endl;

            return -1;
        }

        cout << endl;
        cout << "====================================" << endl;
        cout << "          AD9361 Status" << endl;
        cout << "====================================" << endl;


        // --------------------------------------------------
        // RX
        // --------------------------------------------------

        cout << endl;
        cout << "[RX]" << endl;

        cout << "RX LO          : "
            << status.rxFrequency
            << " Hz" << endl;

        cout << "RX Bandwidth   : "
            << status.rxBandwidth
            << " Hz" << endl;

        cout << "RX Sample Rate : "
            << status.rxSampleRate
            << " Hz" << endl;

        cout << endl;

        cout << "RX1 Gain       : "
            << status.rx1Gain
            << " dB" << endl;

        cout << "RX1 Gain Mode  : "
            << status.rx1GainMode
            << endl;

        cout << "RX1 RSSI       : "
            << status.rx1Rssi
            << " dB" << endl;

        cout << "RX1 RF Port    : "
            << status.rx1RfPort
            << endl;

        cout << endl;

        cout << "RX2 Gain       : "
            << status.rx2Gain
            << " dB" << endl;

        cout << "RX2 Gain Mode  : "
            << status.rx2GainMode
            << endl;

        cout << "RX2 RSSI       : "
            << status.rx2Rssi
            << " dB" << endl;

        cout << "RX2 RF Port    : "
            << status.rx2RfPort
            << endl;


        // --------------------------------------------------
        // TX
        // --------------------------------------------------

        cout << endl;
        cout << "[TX]" << endl;

        cout << "TX LO          : "
            << status.txFrequency
            << " Hz" << endl;

        cout << "TX Bandwidth   : "
            << status.txBandwidth
            << " Hz" << endl;

        cout << "TX Sample Rate : "
            << status.txSampleRate
            << " Hz" << endl;

        cout << endl;

        cout << "TX1 Attenuation: "
            << status.tx1Attenuation
            << " dB" << endl;

        cout << "TX1 RF Port    : "
            << status.tx1RfPort
            << endl;

        cout << "TX2 Attenuation: "
            << status.tx2Attenuation
            << " dB" << endl;

        cout << "TX2 RF Port    : "
            << status.tx2RfPort
            << endl;


        // --------------------------------------------------
        // SYSTEM
        // --------------------------------------------------

        cout << endl;
        cout << "[SYSTEM]" << endl;

        cout << "ENSM Mode      : "
            << status.ensmMode
            << endl;

        cout << "Calibration    : "
            << status.calibrationMode
            << endl;


        // --------------------------------------------------
        // PATH RATES
        // --------------------------------------------------

        cout << endl;
        cout << "[RX PATH RATES]" << endl;
        cout << status.rxPathRates << endl;

        cout << endl;
        cout << "[TX PATH RATES]" << endl;
        cout << status.txPathRates << endl;


        cout << endl;
        cout << "====================================" << endl;

        return 0;
    }
        /*
         * =====================================================
         * RX
         * =====================================================
         */

        if (subcmd == "rx")
        {
            if (argc < 4)
            {
                printHelp();
                return -1;
            }

            string parameter(argv[3]);


            /*
             * RX FREQUENCY
             */

            if (parameter == "frequency")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getRxFrequency(value))
                        return -1;

                    cout << "RX LO = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setRxFrequency(value))
                {
                    cout << "Failed to set RX frequency"
                         << endl;
                    return -1;
                }

                cout << "RX LO = "
                     << value
                     << " Hz" << endl;

                return 0;
            }


            /*
             * RX BANDWIDTH
             */

            if (parameter == "bandwidth")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getRxBandwidth(value))
                        return -1;

                    cout << "RX Bandwidth = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setRxBandwidth(value))
                    return -1;

                cout << "RX Bandwidth = "
                     << value
                     << " Hz" << endl;

                return 0;
            }


            /*
             * RX SAMPLE RATE
             */

            if (parameter == "sample-rate")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getRxSampleRate(value))
                        return -1;

                    cout << "RX Sample Rate = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setRxSampleRate(value))
                    return -1;

                cout << "RX Sample Rate = "
                     << value
                     << " Hz" << endl;

                return 0;
            }
    /*
    * -----------------------------------------------------
    * ADI IIO RX CAPTURE
    * -----------------------------------------------------
    */

    if (parameter == "capture")
    {
        if (argc < 5)
        {
            cout << "Usage:" << endl;
            cout << "  sdr_app ad9361 rx capture <samples> [filename]"
                << endl;
            return -1;
        }

        size_t samples =
            strtoull(
                argv[4],
                nullptr,
                0);

        string filename = "/tmp/adi_rx_iq.bin";

        if (argc >= 6)
        {
            filename = argv[5];
        }

        if (samples == 0)
        {
            cout << "Invalid sample count"
                << endl;
            return -1;
        }

            ADIIIOCapture capture;

            cout << endl;
            cout << "===================================="
                << endl;
            cout << " ADI IIO RX Capture"
                << endl;
            cout << "===================================="
                << endl;

            cout << "Samples : "
                << samples
                << endl;

           cout << "Output  : " << filename << endl;


            cout << endl;

            if (!capture.init())
            {
                cout << "ADI IIO initialization failed."
                    << endl;

                return -1;
            }

            if (!capture.capture(
                    samples,
                    filename))
            {
                cout << "ADI IIO capture failed."
                    << endl;

                capture.close();

                return -1;
            }

            capture.close();

            cout << endl;

            cout << "ADI IIO RX capture successful."
                << endl;

            cout << endl;

            size_t bytes =
                samples * sizeof(int16_t) * 2;

            if (!writeCaptureMetadata(
                    filename,
                    samples,
                    bytes,
                    ad9361))
            {
                cout << "Warning: failed to write capture metadata."
                    << endl;
            }

            return 0;
        }
            cout << "Unknown RX parameter: "
                 << parameter << endl;

            printHelp();
            return -1;
        }


        /*
         * =====================================================
         * RX1 / RX2
         * =====================================================
         */

        if (subcmd == "rx1" || subcmd == "rx2")
        {
            int channel =
                (subcmd == "rx1") ? 1 : 2;

            if (argc < 4)
            {
                printHelp();
                return -1;
            }

            string parameter(argv[3]);

    /*
    * -----------------------------------------------------
    * RX RF PORT
    * -----------------------------------------------------
    */

    if (parameter == "rf-port")
    {
        char port[128] = {};

        // READ
        // sdr_app ad9361 rx1 rf-port
        // sdr_app ad9361 rx2 rf-port
        if (argc == 4)
        {
            if (!ad9361.getRxRfPort(
                    channel,
                    port,
                    sizeof(port)))
            {
                cout << "Failed to read RX"
                    << channel
                    << " RF port"
                    << endl;

                return -1;
            }

            cout << "RX" << channel
                << " RF Port = "
                << port
                << endl;

            return 0;
        }

        // WRITE
        // sdr_app ad9361 rx1 rf-port A_BALANCED
        // sdr_app ad9361 rx2 rf-port B_BALANCED
        if (argc == 5)
        {
            if (!ad9361.setRxRfPort(
                    channel,
                    argv[4]))
            {
                cout << "Failed to set RX"
                    << channel
                    << " RF port"
                    << endl;

                return -1;
            }

            // Readback
            if (!ad9361.getRxRfPort(
                    channel,
                    port,
                    sizeof(port)))
            {
                cout << "RF port was set, "
                    << "but readback failed"
                    << endl;

                return -1;
            }

            cout << "RX" << channel
                << " RF Port = "
                << port
                << endl;

            return 0;
        }

        cout << "Usage:" << endl;
        cout << "  sdr_app ad9361 rx"
            << channel
            << " rf-port"
            << endl;

        cout << "  sdr_app ad9361 rx"
            << channel
            << " rf-port <port>"
            << endl;

        return -1;
    }
            /*
             * RX GAIN
             */

            if (parameter == "gain")
            {
                double value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getRxGain(
                            channel,
                            value))
                        return -1;

                    cout << "RX" << channel
                         << " Gain = "
                         << value
                         << " dB" << endl;

                    return 0;
                }

                value = strtod(
                    argv[4],
                    nullptr);

                if (!ad9361.setRxGain(
                        channel,
                        value))
                    return -1;

                cout << "RX" << channel
                     << " Gain = "
                     << value
                     << " dB" << endl;

                return 0;
            }


            /*
             * GAIN MODE
             */

            if (parameter == "gain-mode")
            {
                char mode[64] = {};

                if (argc == 4)
                {
                    if (!ad9361.getRxGainMode(
                            channel,
                            mode,
                            sizeof(mode)))
                        return -1;

                    cout << "RX" << channel
                         << " Gain Mode = "
                         << mode;

                    return 0;
                }

                if (!ad9361.setRxGainMode(
                        channel,
                        argv[4]))
                    return -1;

                cout << "RX" << channel
                     << " Gain Mode = "
                     << argv[4] << endl;

                return 0;
            }


            /*
             * AGC
             *
             * AGC is implemented through
             * AD9361 gain_control_mode.
             */

            if (parameter == "agc")
            {
                if (argc < 5)
                {
                    cout << "Missing AGC mode"
                         << endl;
                    return -1;
                }

                const char* mode = argv[4];

                if (!ad9361.setRxGainMode(
                        channel,
                        mode))
                    return -1;

                cout << "RX" << channel
                     << " AGC = "
                     << mode << endl;

                return 0;
            }


            /*
             * RSSI
             */

            if (parameter == "rssi")
            {
                double rssi = 0;

                if (!ad9361.getRxRssi(
                        channel,
                        rssi))
                    return -1;

                cout << "RX" << channel
                     << " RSSI = "
                     << rssi
                     << " dB" << endl;

                return 0;
            }


            cout << "Unknown RX channel parameter: "
                 << parameter << endl;

            printHelp();
            return -1;
        }

    /*
    * =====================================================
    * TX1 / TX2
    * =====================================================
    */

    if (subcmd == "tx1" || subcmd == "tx2")
    {
        int channel =
            (subcmd == "tx1") ? 1 : 2;

        if (argc < 4)
        {
            printHelp();
            return -1;
        }

        string parameter(argv[3]);

        /*
        * TX ATTENUATION
        */

        if (parameter == "attenuation")
        {
            double value = 0.0;

            /*
            * READ
            *
            * sdr_app ad9361 tx1 attenuation
            * sdr_app ad9361 tx2 attenuation
            */

            if (argc == 4)
            {
                if (!ad9361.getTxAttenuation(
                        channel,
                        value))
                {
                    cout << "Failed to read TX"
                        << channel
                        << " attenuation"
                        << endl;

                    return -1;
                }

                cout << "TX" << channel
                    << " Attenuation = "
                    << value
                    << " dB"
                    << endl;

                return 0;
            }

            /*
            * WRITE
            *
            * sdr_app ad9361 tx1 attenuation <dB>
            * sdr_app ad9361 tx2 attenuation <dB>
            */

            value = strtod(
                argv[4],
                nullptr);

            if (!ad9361.setTxAttenuation(
                    channel,
                    value))
            {
                cout << "Failed to set TX"
                    << channel
                    << " attenuation"
                    << endl;

                return -1;
            }

            cout << "TX" << channel
                << " Attenuation = "
                << value
                << " dB"
                << endl;

            return 0;
        }

         /*
         * -----------------------------------------------------
         * TX RF PORT
         * -----------------------------------------------------
         */

        if (parameter == "rf-port")
        {
            char port[128] = {};

            // READ
            // sdr_app ad9361 tx1 rf-port
            // sdr_app ad9361 tx2 rf-port

            if (argc == 4)
            {
                if (!ad9361.getTxRfPort(
                        channel,
                        port,
                        sizeof(port)))
                {
                    cout << "Failed to read TX"
                         << channel
                         << " RF port"
                         << endl;

                    return -1;
                }

                cout << "TX" << channel
                     << " RF Port = "
                     << port
                     << endl;

                return 0;
            }

            // WRITE
            // sdr_app ad9361 tx1 rf-port A
            // sdr_app ad9361 tx2 rf-port B

            if (argc == 5)
            {
                if (!ad9361.setTxRfPort(
                        channel,
                        argv[4]))
                {
                    cout << "Failed to set TX"
                         << channel
                         << " RF port"
                         << endl;

                    return -1;
                }

                // Readback
                if (!ad9361.getTxRfPort(
                        channel,
                        port,
                        sizeof(port)))
                {
                    cout << "RF port was set, "
                         << "but readback failed"
                         << endl;

                    return -1;
                }

                cout << "TX" << channel
                     << " RF Port = "
                     << port
                     << endl;

                return 0;
            }

            cout << "Usage:" << endl;
            cout << "  sdr_app ad9361 tx"
                 << channel
                 << " rf-port"
                 << endl;

            cout << "  sdr_app ad9361 tx"
                 << channel
                 << " rf-port <port>"
                 << endl;

            return -1;
        }

        cout << "Unknown TX channel parameter: "
            << parameter
            << endl;

        printHelp();
        return -1;
    }

        /*
         * =====================================================
         * TX
         * =====================================================
         */

        if (subcmd == "tx")
        {
            if (argc < 4)
            {
                printHelp();
                return -1;
            }

            string parameter(argv[3]);


            /*
             * TX FREQUENCY
             */

            if (parameter == "frequency")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getTxFrequency(value))
                        return -1;

                    cout << "TX LO = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setTxFrequency(value))
                    return -1;

                cout << "TX LO = "
                     << value
                     << " Hz" << endl;

                return 0;
            }


            /*
             * TX BANDWIDTH
             */

            if (parameter == "bandwidth")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getTxBandwidth(value))
                        return -1;

                    cout << "TX Bandwidth = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setTxBandwidth(value))
                    return -1;

                cout << "TX Bandwidth = "
                     << value
                     << " Hz" << endl;

                return 0;
            }


            /*
             * TX SAMPLE RATE
             */

            if (parameter == "sample-rate")
            {
                uint64_t value = 0;

                if (argc == 4)
                {
                    if (!ad9361.getTxSampleRate(value))
                        return -1;

                    cout << "TX Sample Rate = "
                         << value
                         << " Hz" << endl;

                    return 0;
                }

                value = strtoull(
                    argv[4],
                    nullptr,
                    0);

                if (!ad9361.setTxSampleRate(value))
                    return -1;

                cout << "TX Sample Rate = "
                     << value
                     << " Hz" << endl;

                return 0;
            }


            cout << "Unknown TX parameter: "
                 << parameter << endl;

            printHelp();
            return -1;
        }


        cout << "Unknown AD9361 command: "
             << subcmd << endl;

        printHelp();
        return -1;
    }



    /*
     * =========================================================
     * DMA Commands
     * =========================================================
     */
    if (cmd == "dma")
    {
        /*
         * Initialize AXI DMA register access
         */
        if (dma_init() != 0)
        {
            cout << "DMA initialization failed." << endl;
            return -1;
        }

        /*
         * sdr_app dma
         */
        if (argc == 2)
        {
            dma_print_status();
        }

        /*
         * sdr_app dma <subcommand>
         */
        else if (argc == 3)
        {
            string subcmd(argv[2]);

            /*
             * -------------------------------------------------
             * DMA RESET
             * -------------------------------------------------
             */
            if (subcmd == "reset")
            {
                dma_reset();

                cout << "DMA Reset Done" << endl;

                dma_print_status();
            }

            /*
             * -------------------------------------------------
             * DMA CAPTURE
             * -------------------------------------------------
             */
            else if (subcmd == "capture")
            {
                const uint32_t dma_buffer_addr = 0x3F000000;
                const uint32_t dma_length = 4096;

                cout << endl;
                cout << "====================================" << endl;
                cout << " AXI DMA S2MM Capture" << endl;
                cout << "====================================" << endl;

                cout << "Buffer Address : 0x"
                     << hex << dma_buffer_addr << dec << endl;

                cout << "Transfer Length: "
                     << dma_length << " bytes" << endl;

                /*
                 * ---------------------------------------------
                 * Open FPGA register controller
                 * ---------------------------------------------
                 */
                if (!fpga.open())
                {
                    cout << "FPGA Open Failed" << endl;
                    dma_irq_close();                    
                    dma_close();
                    return -1;
                }

                /*
                 * ---------------------------------------------
                 * Open UIO interrupt
                 * ---------------------------------------------
                 */
                if (dma_irq_init() != 0)
                {
                    cout << "DMA UIO initialization failed." << endl;
                    dma_irq_close();                    
                    dma_close();
                    return -1;
                }

                /*
                 * ---------------------------------------------
                 * Start DMA first
                 *
                 * DMA is now waiting for AXI-Stream data.
                 * ---------------------------------------------
                 */
                dma_start_s2mm(
                    dma_buffer_addr,
                    dma_length
                );

                /*
                 * ---------------------------------------------
                 * Enable AXI-Stream data generation
                 * ---------------------------------------------
                 */
                fpga.enableDSP(true);

                cout << endl;
                cout << "Waiting for DMA IRQ..." << endl;

                /*
                 * ---------------------------------------------
                 * Wait for DMA interrupt through UIO
                 *
                 * dma_wait_for_completion() now waits on
                 * /dev/uio0 instead of polling DMASR.
                 * ---------------------------------------------
                 */
            bool capture_ok =
                dma_wait_for_completion(5000);

            /*
            * ---------------------------------------------
            * Stop AXI-Stream data generation
            * ---------------------------------------------
            */
            fpga.enableDSP(false);

            /*
            * ---------------------------------------------
            * Report result and dump buffer
            * ---------------------------------------------
            */
            if (capture_ok)
            {
                cout << "DMA Capture Completed" << endl;

                dma_dump_buffer(
                    dma_buffer_addr,
                    dma_length
                );
            }
            else
            {
                cout << "DMA Capture Failed" << endl;
            }

            /*
                * ---------------------------------------------
                * Print final DMA status
                * ---------------------------------------------
                */
            dma_print_status();

            /*
                * ---------------------------------------------
                * Close UIO
                * ---------------------------------------------
                */
            dma_irq_close();
            }

            /*
             * -------------------------------------------------
             * UNKNOWN DMA COMMAND
             * -------------------------------------------------
             */
            else
            {
                cout << "Unknown DMA command: "
                     << subcmd << endl;

                printHelp();
            }
        }

        /*
         * Close DMA register mapping
         */
        dma_irq_close();
        dma_close();

        return 0;
    }

    /*
     * =========================================================
     * Register Controller Commands
     * =========================================================
     */

    if (!fpga.open())
    {
        cout << "FPGA Open Failed" << endl;
        return -1;
    }

    /*
     * ---------------------------------------------------------
     * ENABLE
     * ---------------------------------------------------------
     */
    if (cmd == "enable")
    {
        fpga.enableDSP(true);
        cout << "DSP Enabled" << endl;
    }

    /*
     * ---------------------------------------------------------
     * DISABLE
     * ---------------------------------------------------------
     */
    else if (cmd == "disable")
    {
        fpga.enableDSP(false);
        cout << "DSP Disabled" << endl;
    }

    /*
     * ---------------------------------------------------------
     * RESET
     * ---------------------------------------------------------
     */
    else if (cmd == "reset")
    {
        fpga.resetDSP();
        cout << "DSP Reset Done" << endl;
    }

    /*
     * ---------------------------------------------------------
     * MODE
     * ---------------------------------------------------------
     */
    else if (cmd == "mode")
    {
        if (argc < 3)
        {
            cout << "Missing mode value" << endl;
            return -1;
        }

        uint32_t mode =
            strtoul(argv[2], nullptr, 0);

        fpga.setMode(mode);

        cout << "Mode = "
             << mode << endl;
    }

    /*
     * ---------------------------------------------------------
     * FILTER
     * ---------------------------------------------------------
     */
    else if (cmd == "filter")
    {
        if (argc < 3)
        {
            cout << "Missing filter value" << endl;
            return -1;
        }

        uint32_t filter =
            strtoul(argv[2], nullptr, 0);

        fpga.setFilter(filter);

        cout << "Filter = "
             << filter << endl;
    }

    /*
     * ---------------------------------------------------------
     * PARAM0
     * ---------------------------------------------------------
     */
    else if (cmd == "param0")
    {
        if (argc < 3)
        {
            cout << "Missing Param0 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        fpga.setParam0(value);

        cout << "Param0 = 0x"
             << hex << value << dec << endl;
    }

    /*
     * ---------------------------------------------------------
     * PARAM1
     * ---------------------------------------------------------
     */
    else if (cmd == "param1")
    {
        if (argc < 3)
        {
            cout << "Missing Param1 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        fpga.setParam1(value);

        cout << "Param1 = 0x"
             << hex << value << dec << endl;
    }

    /*
     * ---------------------------------------------------------
     * STATUS
     * ---------------------------------------------------------
     */
    else if (cmd == "status")
    {
        cout << endl;
        cout << "====================================" << endl;
        cout << " Register Controller Status" << endl;
        cout << "====================================" << endl;

        cout << "DSP Enable : "
             << (fpga.isEnabled() ? "ON" : "OFF")
             << endl;

        cout << "Mode       : "
             << fpga.getMode()
             << endl;

        cout << "Filter     : "
             << fpga.getFilter()
             << endl;

        cout << "Param0     : 0x"
             << hex << fpga.getParam0()
             << dec << endl;

        cout << "Param1     : 0x"
             << hex << fpga.getParam1()
             << dec << endl;
    }

    /*
     * ---------------------------------------------------------
     * UNKNOWN COMMAND
     * ---------------------------------------------------------
     */
    else
    {
        cout << "Unknown command: "
             << cmd << endl << endl;

        printHelp();

        return -1;
    }

    return 0;
}