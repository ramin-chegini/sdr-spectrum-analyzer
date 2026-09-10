#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

#include "RegisterController.h"
#include "dma.h"
#include "ad9361.h"
#include "adi_iio_capture.h"
#include "l2_protocol.h"
#include "CommandDispatcher.h"

using namespace std;

static void printHelp()
{
    cout << "Usage:" << endl;
    cout << "  ./sdr_app" << endl;
    cout << "  ./sdr_app --help" << endl;
    cout << "  ./sdr_app -h" << endl;
    cout << endl;

    cout << "Register Controller Commands:" << endl;

    cout << endl;
    cout << "  CONTROL:" << endl;

    cout << "    ./sdr_app reset" << endl;
    cout << "        Generate Soft Reset pulse" << endl;

    cout << "    ./sdr_app soft-start <0|1>" << endl;
    cout << "        Soft Start" << endl;

    cout << "    ./sdr_app send-packet <0|1>" << endl;
    cout << "        Send Packet Enable" << endl;

    cout << "    ./sdr_app mod-enable <0|1>" << endl;
    cout << "        Modulation Enable" << endl;

    cout << "    ./sdr_app squelch-enable <0|1>" << endl;
    cout << "        Squelch Enable" << endl;

    cout << "    ./sdr_app mgc2-on <0|1>" << endl;
    cout << "        MGC2 Enable" << endl;

    cout << "    ./sdr_app agc1-on <0|1>" << endl;
    cout << "        AGC1 Enable" << endl;

    cout << "    ./sdr_app agc2-on <0|1>" << endl;
    cout << "        AGC2 Enable" << endl;

    cout << "    ./sdr_app amp-ok <0|1>" << endl;
    cout << "        Amplifier OK" << endl;


    cout << endl;
    cout << "  CONFIGURATION:" << endl;

    cout << "    ./sdr_app bandwidth <value>" << endl;
    cout << "        Bandwidth Select:" << endl;
    cout << "          0  = WB (AD9361 Filter / FPGA FIR Bypass, Dec=2)" << endl;
    cout << "          1  = 200 kHz => Dec=8" << endl;
    cout << "          2  = 150 kHz => Dec=8" << endl;
    cout << "          3  = 60 kHz  => Dec=16" << endl;
    cout << "          4  = 30 kHz  => Dec=32" << endl;
    cout << "          5  = 15 kHz  => Dec=64" << endl;
    cout << "          6  = 12 kHz  => Dec=64" << endl;
    cout << "          7  = 7.5 kHz => Dec=128" << endl;
    cout << "          8  = 5 kHz   => Dec=256" << endl;
    cout << "          9  = 3 kHz   => Dec=512" << endl;
    cout << "          10 = 1.2 kHz => Dec=1024" << endl;
    cout << "          11 = 0.6 kHz => Dec=2048" << endl;

    cout << "    ./sdr_app mode-select <value>" << endl;
    cout << "        Mode Select (0-255)" << endl;

    cout << "    ./sdr_app led-timer <value>" << endl;
    cout << "        LED Timer based on FreqSys (32-bit)" << endl;

    cout << "    ./sdr_app msg-value <value>" << endl;
    cout << "        Message Value (32-bit)" << endl;

    cout << "    ./sdr_app a-coeff1 <value>" << endl;
    cout << "        A Coefficient 1 (8-bit)" << endl;

    cout << "    ./sdr_app a-coeff2 <value>" << endl;
    cout << "        A Coefficient 2 (8-bit)" << endl;

    cout << "    ./sdr_app ref1 <value>" << endl;
    cout << "        Reference 1 (24-bit)" << endl;

    cout << "    ./sdr_app ref2 <value>" << endl;
    cout << "        Reference 2 (24-bit)" << endl;


    cout << endl;
    cout << "  STATUS:" << endl;

    cout << "    ./sdr_app status" << endl;
    cout << "        Read Register Controller status" << endl;

    cout << endl;

    cout << "DMA Commands:" << endl;
    cout << "  ./sdr_app dma" << endl;
    cout << "  ./sdr_app dma reset" << endl;
    cout << "  ./sdr_app dma capture" << endl;
    cout << "  ./sdr_app dma audio" << endl;    

    cout << endl;

    cout << "AD9361 Commands:" << endl;
    cout << "  ./sdr_app startup" << endl;
    cout << "      Initialize SDR startup configuration" << endl;
    cout << "  ./sdr_app ad9361 status" << endl;

    cout << endl;

    cout << "RX Commands:" << endl;

    // cout << endl;
    // cout << "  RX Frequency:" << endl;
    // cout << "    ./sdr_app ad9361 rx frequency" << endl;
    // cout << "    ./sdr_app ad9361 rx frequency <Hz>" << endl;
    // cout << "      Range: 70 MHz to 6 GHz" << endl;
    // cout << "      Step : 1 Hz" << endl;

    cout << "  RX Frequency:" << endl;
    cout << "    ./sdr_app ad9361 rx frequency" << endl;
    cout << "    ./sdr_app ad9361 rx frequency <MHz>" << endl;
    cout << "      Range: 70 to 6000 MHz" << endl;
    cout << "      Step : 1 MHz" << endl;

    cout << endl;
    cout << "  RX Bandwidth:" << endl;
    cout << "    ./sdr_app ad9361 rx bandwidth" << endl;
    cout << "    ./sdr_app ad9361 rx bandwidth <Hz>" << endl;
    cout << "      Range: 200 kHz to 56 MHz" << endl;
    cout << "      Step : 1 Hz" << endl;
    cout << "RF Bandwidth of front-end analog filter in Hz, specified as a scalar from 200 kHz to 56 MHz" << endl;

    cout << endl;
    cout << "  RX Sample Rate:" << endl;
    cout << "    ./sdr_app ad9361 rx sample-rate" << endl;
    cout << "    ./sdr_app ad9361 rx sample-rate <Hz>" << endl;
    cout << "      Range: 2,083,340 to 30,720,000 SPS" << endl;
    cout << "      Step : 1 SPS" << endl;

    cout << endl;
    cout << "  RX Capture:" << endl;
    cout << "    ./sdr_app ad9361 rx capture <samples> [filename]" << endl;
    cout << "      Format: I/Q, S12/16" << endl;


    cout << endl;
    cout << "RX1 / RX2 Commands:" << endl;

    cout << endl;
    cout << "  RX1 Gain:" << endl;
    cout << "    ./sdr_app ad9361 rx1 gain" << endl;
    cout << "    ./sdr_app ad9361 rx1 gain <dB>" << endl;
    cout << "      Range: -3 to 71 dB" << endl;
    cout << "      Step : 1 dB" << endl;
    cout << "Channel 1 gain, specified as a scalar from -3 dB to 71 dB. The acceptable minimum and maximum gain setting depends on the center frequency" << endl;

    cout << endl;
    cout << "  RX1 Gain Mode / AGC:" << endl;
    cout << "    ./sdr_app ad9361 rx1 gain-mode" << endl;
    cout << "    ./sdr_app ad9361 rx1 gain-mode <mode>" << endl;
    cout << "    ./sdr_app ad9361 rx1 agc <mode>" << endl;

    cout << endl;
    cout << "  RX1 RSSI:" << endl;
    cout << "    ./sdr_app ad9361 rx1 rssi" << endl;
    cout << "      Read-only" << endl;

    cout << endl;
    cout << "  RX1 RF Port:" << endl;
    cout << "    ./sdr_app ad9361 rx1 rf-port" << endl;
    cout << "    ./sdr_app ad9361 rx1 rf-port <port>" << endl;


    cout << endl;
    cout << "  RX2 Gain:" << endl;
    cout << "    ./sdr_app ad9361 rx2 gain" << endl;
    cout << "    ./sdr_app ad9361 rx2 gain <dB>" << endl;
    cout << "      Range: -3 to 71 dB" << endl;
    cout << "      Step : 1 dB" << endl;
    cout << "Channel 2 gain, specified as a scalar from -3 dB to 71 dB. The acceptable minimum and maximum gain setting depends on the center frequency" << endl;
    

    cout << endl;
    cout << "  RX2 Gain Mode / AGC:" << endl;
    cout << "    ./sdr_app ad9361 rx2 gain-mode" << endl;
    cout << "    ./sdr_app ad9361 rx2 gain-mode <mode>" << endl;
    cout << "    ./sdr_app ad9361 rx2 agc <mode>" << endl;

    cout << endl;
    cout << "  RX2 RSSI:" << endl;
    cout << "    ./sdr_app ad9361 rx2 rssi" << endl;
    cout << "      Read-only" << endl;

    cout << endl;
    cout << "  RX2 RF Port:" << endl;
    cout << "    ./sdr_app ad9361 rx2 rf-port" << endl;
    cout << "    ./sdr_app ad9361 rx2 rf-port <port>" << endl;


    cout << endl;
    cout << "TX Commands:" << endl;

    cout << endl;
    cout << "  TX Frequency:" << endl;
    cout << "    ./sdr_app ad9361 tx frequency" << endl;
    cout << "    ./sdr_app ad9361 tx frequency <Hz>" << endl;
    cout << "      Range: 46,875,001 Hz to 6 GHz" << endl;
    cout << "      Step : 1 Hz" << endl;

    cout << endl;
    cout << "  TX Bandwidth:" << endl;
    cout << "    ./sdr_app ad9361 tx bandwidth" << endl;
    cout << "    ./sdr_app ad9361 tx bandwidth <Hz>" << endl;
    cout << "      Range: 200 kHz to 56 MHz" << endl;
    cout << "      Step : 1 Hz" << endl;

    cout << endl;
    cout << "  TX Sample Rate:" << endl;
    cout << "    ./sdr_app ad9361 tx sample-rate" << endl;
    cout << "    ./sdr_app ad9361 tx sample-rate <Hz>" << endl;
    cout << "      Range: 2,083,340 to 30,720,000 SPS" << endl;
    cout << "      Step : 1 SPS" << endl;

    cout << endl;
    cout << "  TX1 Attenuation:" << endl;
    cout << "    ./sdr_app ad9361 tx1 attenuation" << endl;
    cout << "    ./sdr_app ad9361 tx1 attenuation <dB>" << endl;
    cout << "      Range: -89.75 to 0 dB" << endl;
    cout << "      Step : 0.25 dB" << endl;
    cout << "Attentuation specified as a scalar from -89.75 to 0 dB with a resolution of 0.25 dB" << endl;

    cout << endl;
    cout << "  TX2 Attenuation:" << endl;
    cout << "    ./sdr_app ad9361 tx2 attenuation" << endl;
    cout << "    ./sdr_app ad9361 tx2 attenuation <dB>" << endl;
    cout << "      Range: -89.75 to 0 dB" << endl;
    cout << "      Step : 0.25 dB" << endl;
    cout << "Attentuation specified as a scalar from -89.75 to 0 dB with a resolution of 0.25 dB" << endl;

    cout << endl;
    cout << "  TX1 RF Port:" << endl;
    cout << "    ./sdr_app ad9361 tx1 rf-port" << endl;
    cout << "    ./sdr_app ad9361 tx1 rf-port <port>" << endl;

    cout << endl;
    cout << "  TX2 RF Port:" << endl;
    cout << "    ./sdr_app ad9361 tx2 rf-port" << endl;
    cout << "    ./sdr_app ad9361 tx2 rf-port <port>" << endl;

    cout << endl;

    cout << "Network Commands:" << endl;
    cout << "  ./sdr_app net psd" << endl;
    cout << "  ./sdr_app net iq" << endl;
    cout << "  ./sdr_app net rx" << endl;
    cout << "      Receive configuration packet from GUI" << endl;
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

static bool initializeSDR()
{
    AD9361Controller ad9361;
    RegisterController fpga;

    if (!ad9361.init())
    {
        cout << "AD9361 initialization failed."
             << endl;

        return false;
    }

    /*
     * ---------------------------------------------
     * FPGA Register Controller
     * ---------------------------------------------
     */
    if (!fpga.open())
    {
        cout << "FPGA Register Controller Open Failed"
             << endl;

        return false;
    }

    /*
     * Addr 11 -> Mode Select = 3
     */
    fpga.setModeSelect(3);
    /*
    * ---------------------------------------------
    * Enable Audio Modulator
    *
    * Register Addr 4
    * ---------------------------------------------
    */
    fpga.setModEnable(true); 

    const uint64_t DEFAULT_RX_SAMPLE_RATE = 2083340ULL;
    const uint64_t DEFAULT_RX_BANDWIDTH  = 200000ULL;

    if (!ad9361.setRxSampleRate(DEFAULT_RX_SAMPLE_RATE))
    {
        cout << "Failed to set default RX Sample Rate."
             << endl;

        return false;
    }

    if (!ad9361.setRxBandwidth(DEFAULT_RX_BANDWIDTH))
    {
        cout << "Failed to set default RX Bandwidth."
             << endl;

        return false;
    }

    cout << endl;
    cout << "====================================" << endl;
    cout << "       SDR Startup Configuration" << endl;
    cout << "====================================" << endl;

    cout << "Mode Select    : 3" << endl;

    cout << "RX Sample Rate : "
         << DEFAULT_RX_SAMPLE_RATE
         << " Hz" << endl;

    cout << "RX Bandwidth   : "
         << DEFAULT_RX_BANDWIDTH
         << " Hz" << endl;

    cout << "Startup configuration completed."
         << endl;

    return true;
}

/*
 * =========================================================
 * MAIN SDR APPLICATION
 * =========================================================
 *
 * Default mode:
 *
 *     ./sdr_app
 *
 * 1. Initialize AD9361
 * 2. Set RX Sample Rate
 * 3. Set RX Bandwidth
 * 4. Start continuous L2 RX listener
 */

static int runMainApplication()
{
    
    /*
     * =====================================================
     * SDR STARTUP
     * =====================================================
     */

    if (!initializeSDR())
    {
        cout << "SDR startup failed."
             << endl;

        return -1;
    }


    /*
     * =====================================================
     * DEFAULT FPGA CONFIGURATION
     * =====================================================
     */

    RegisterController regCtrl;

    if (!regCtrl.open())
    {
        cout << "Failed to open FPGA Register Controller."
             << endl;

        return -1;
    }

    const uint32_t DEFAULT_LED_TIMER = 2083340;

    regCtrl.setLedTimer(DEFAULT_LED_TIMER);

    cout << "Default LED Timer : "
         << DEFAULT_LED_TIMER
         << " Hz"
         << endl;

    regCtrl.close();


    /*
     * =====================================================
     * L2 RX Listener
     * =====================================================
     */

    L2RxListener listener;

    if (!listener.open(
            "usb0",
            L2_RX_PACKETLEN))
    {
        cout << "Failed to open L2 RX listener."
             << endl;

        return -1;
    }


    cout << endl;
    cout << "====================================" << endl;
    cout << " L2 RX - Configuration Listener" << endl;
    cout << "====================================" << endl;

    cout << "Interface      : usb0" << endl;

    cout << "Packet Length  : 0x"
         << hex
         << uppercase
         << setw(4)
         << setfill('0')
         << L2_RX_PACKETLEN
         << dec
         << setfill(' ')
         << endl;

    cout << "Frame Size     : "
         << L2_RX_FRAME_SIZE
         << " bytes"
         << endl;

    cout << endl;
    cout << "Continuous RX mode enabled." << endl;
    cout << "Waiting for packets from GUI..."
         << endl;


    /*
     * =====================================================
     * Continuous GUI Command Loop
     * =====================================================
     */

    while (true)
    {
        uint16_t fpga_id = 0;
        uint8_t reg_addr = 0;
        uint8_t data[L2_RX_DATA_SIZE] = {};


        bool receive_ok =
            listener.receive(
                &fpga_id,
                &reg_addr,
                data);


        if (!receive_ok)
        {
            cout << "L2 RX failed."
                 << endl;

            break;
        }


        /*
         * =================================================
         * Apply GUI Register Command
         * =================================================
         */

        if (!applyRegisterCommand(
                fpga_id,
                reg_addr,
                data))
        {
            cout << "Failed to apply GUI register command."
                 << endl;

            continue;
        }


        cout << endl;
        cout << "GUI command processed successfully."
             << endl;

        cout << endl;
        cout << "Waiting for next GUI command..."
             << endl;
    }


    listener.close();

    return 0;
}

int main(int argc, char *argv[])
{

    DmaContext psd_dma{};
    DmaContext audio_dma{};


    // if (argc < 2)
    // {
    //     printHelp();
    //     return 0;
    // }

    if (argc == 2 &&
        (string(argv[1]) == "--help" ||
         string(argv[1]) == "-h"))
    {
        printHelp();
        return 0;
    }

    if (argc < 2)
    {
        //printHelp();
        return runMainApplication();
        //return 0;
    }    

    string cmd(argv[1]);

    /*
     * =====================================================
     * Network Layer-2 Commands
     * =====================================================
     */

    // if (argc >= 3 &&
    //     string(argv[1]) == "net" &&
    //     string(argv[2]) == "psd")
    // {

    //     uint8_t psd_test[L2_MAX_PAYLOAD_SIZE];

    //     for (size_t i = 0; i < L2_MAX_PAYLOAD_SIZE; i++)
    //     {
    //         psd_test[i] = i & 0xFF;
    //     }


    //     l2_send_packet(
    //         "usb0",
    //         L2_ETHERTYPE_PSD,
    //         L2_ID_PSD,
    //         psd_test,
    //         sizeof(psd_test)
    //     );

    //     // l2_send_packet(
    //     //     "eth0",
    //     //     L2_ETHERTYPE_PSD,
    //     //     L2_ID_PSD,
    //     //     psd_test,
    //     //     sizeof(psd_test)
    //     // );        


    //     return 0;
    // }

    if (argc >= 3 &&
        string(argv[1]) == "net" &&
        string(argv[2]) == "psd")
    {
        const uint32_t dma_buffer_addr = 0x3F000000;
        const uint32_t dma_length = 8192;

        cout << endl;
        cout << "====================================" << endl;
        cout << " DMA Buffer -> USB0 PSD" << endl;
        cout << "====================================" << endl;

        cout << "DMA Buffer : 0x"
            << hex << dma_buffer_addr << dec << endl;

        cout << "Payload    : "
            << dma_length << " bytes" << endl;


        /*
        * ---------------------------------------------
        * Map DMA buffer
        * ---------------------------------------------
        */
        const uint8_t *dma_data =
            dma_map_buffer(
                dma_buffer_addr,
                dma_length
            );

        if (dma_data == nullptr)
        {
            cout << "Failed to map DMA buffer."
                << endl;

            return -1;
        }


        /*
        * ---------------------------------------------
        * Send DMA buffer through Layer-2
        * ---------------------------------------------
        */
        bool send_ok =
            l2_send_packet(
                "usb0",
                L2_ETHERTYPE_PSD,
                L2_ID_PSD,
                dma_data,
                dma_length
            );


        /*
        * ---------------------------------------------
        * Unmap DMA buffer
        * ---------------------------------------------
        */
        dma_unmap_buffer(
            dma_data,
            dma_length
        );


        /*
        * ---------------------------------------------
        * Close /dev/mem
        * ---------------------------------------------
        */
        //dma_close();


        if (!send_ok)
        {
            cout << "PSD packet transmission failed."
                << endl;

            return -1;
        }


        cout << "PSD packet transmitted successfully."
            << endl;

        return 0;
    }


    /*
     * =====================================================
     * Network Layer-2 IQ Command
     * =====================================================
     */

    if (argc >= 3 &&
        string(argv[1]) == "net" &&
        string(argv[2]) == "iq")
    {
        const uint32_t dma_buffer_addr = 0x3F000000;
        const uint32_t dma_length = 8192;

        cout << endl;
        cout << "====================================" << endl;
        cout << " DMA Buffer -> USB0 IQ" << endl;
        cout << "====================================" << endl;

        cout << "DMA Buffer : 0x"
             << hex << dma_buffer_addr << dec << endl;

        cout << "Payload    : "
             << dma_length << " bytes" << endl;

        /*
         * ---------------------------------------------
         * Initialize DMA access
         * ---------------------------------------------
         */

        // if (dma_init() != 0)
        // {
        //     cout << "DMA initialization failed."
        //          << endl;

        //     return -1;
        // }

        /*
         * ---------------------------------------------
         * Map DMA buffer
         * ---------------------------------------------
         */

        const uint8_t *dma_data =
            dma_map_buffer(
                dma_buffer_addr,
                dma_length
            );

        if (dma_data == nullptr)
        {
            cout << "Failed to map DMA buffer."
                << endl;

            return -1;
        }

        /*
         * ---------------------------------------------
         * Send IQ buffer through Layer-2
         * ---------------------------------------------
         */

        cout << "Sending IQ buffer over USB0..."
             << endl;

        bool send_ok =
            l2_send_packet(
                "usb0",
                L2_ETHERTYPE_IQ,
                L2_ID_IQ,
                dma_data,
                dma_length);

        /*
         * ---------------------------------------------
         * Unmap DMA buffer
         * ---------------------------------------------
         */

        dma_unmap_buffer(
            dma_data,
            dma_length);

        /*
         * ---------------------------------------------
         * Close DMA
         * ---------------------------------------------
         */

        //dma_close();

        if (!send_ok)
        {
            cout << "IQ packet transmission failed."
                 << endl;

            return -1;
        }

        cout << "IQ packet transmitted successfully."
             << endl;

        return 0;
    }

    /*
    * =====================================================
    * Network Layer-2 RX Command
    * =====================================================
    */

    if (argc >= 3 &&
        string(argv[1]) == "net" &&
        string(argv[2]) == "rx")
    {
        cout << endl;
        cout << "====================================" << endl;
        cout << " L2 RX - Configuration Packet" << endl;
        cout << "====================================" << endl;

        cout << "Interface      : usb0" << endl;
        cout << "Expected Type  : 0x"
            << hex
            << uppercase
            << L2_RX_PACKETLEN
            << dec
            << endl;

        cout << endl;
        cout << "Continuous RX mode enabled." << endl;
        cout << "Waiting for packets from GUI..." << endl;


        /*
        * =====================================================
        * Continuous L2 RX Loop
        * =====================================================
        */

        while (true)
        {
            uint16_t fpga_id = 0;
            uint8_t reg_addr = 0;
            uint8_t data[L2_RX_DATA_SIZE] = {};

            bool receive_ok =
                l2_receive_packet(
                    "usb0",
                    L2_RX_PACKETLEN,
                    &fpga_id,
                    &reg_addr,
                    data);

            if (!receive_ok)
            {
                cout << "L2 RX failed." << endl;
                return -1;
            }


            /*
            * =================================================
            * Apply GUI Register Command
            * =================================================
            */

            if (!applyRegisterCommand(
                    fpga_id,
                    reg_addr,
                    data))
            {
                cout << "Failed to apply GUI register command."
                    << endl;

                continue;
            }

            cout << endl;
            cout << "GUI command processed successfully."
                << endl;

            cout << endl;
            cout << "Waiting for next GUI command..."
                << endl;
        }

        return 0;
    }

    /*
    * =====================================================
    * Network Layer-2 RX Command
    * =====================================================
    */

    // if (argc >= 3 &&
    //     string(argv[1]) == "net" &&
    //     string(argv[2]) == "rx")
    // {
    //     uint16_t fpga_id = 0;
    //     uint8_t reg_addr = 0;
    //     uint8_t data[L2_RX_DATA_SIZE] = {};

    //     cout << endl;
    //     cout << "====================================" << endl;
    //     cout << " L2 RX - Configuration Packet" << endl;
    //     cout << "====================================" << endl;

    //     cout << "Interface      : usb0" << endl;
    //     cout << "Expected Type  : 0x"
    //         << hex
    //         << L2_RX_PACKETLEN
    //         << dec
    //         << endl;

    //     cout << endl;
    //     cout << "Waiting for packet from GUI..." << endl;

    //     bool receive_ok =
    //         l2_receive_packet(
    //             "usb0",
    //             L2_RX_PACKETLEN,
    //             &fpga_id,
    //             &reg_addr,
    //             data);

    //     if (!receive_ok)
    //     {
    //         cout << "L2 RX failed." << endl;
    //         return -1;
    //     }

    //     cout << endl;
    //     cout << "Packet received successfully." << endl;

    //     cout << "FPGA ID         : 0x"
    //         << hex
    //         << uppercase
    //         << setw(4)
    //         << setfill('0')
    //         << fpga_id
    //         << dec
    //         << setfill(' ')
    //         << endl;

    //     cout << "Register Addr   : 0x"
    //         << hex
    //         << uppercase
    //         << setw(2)
    //         << setfill('0')
    //         << static_cast<unsigned int>(reg_addr)
    //         << dec
    //         << setfill(' ')
    //         << endl;

    //     cout << "Data            : ";

    //     for (int i = 0; i < L2_RX_DATA_SIZE; i++)
    //     {
    //         cout << hex
    //             << uppercase
    //             << setw(2)
    //             << setfill('0')
    //             << static_cast<unsigned int>(data[i]);

    //         if (i < L2_RX_DATA_SIZE - 1)
    //             cout << " ";
    //     }

    //     cout << dec << setfill(' ') << endl;


    //     /*
    //     * =====================================================
    //     * Apply GUI Register Command
    //     * =====================================================
    //     */

    //     if (!applyRegisterCommand(
    //             fpga_id,
    //             reg_addr,
    //             data))
    //     {
    //         cout << "Failed to apply GUI register command."
    //             << endl;

    //         return -1;
    //     }

    //     cout << endl;
    //     cout << "GUI command processed successfully."
    //         << endl;

    //     return 0;
    // }    

    RegisterController fpga;

    /*
    * =========================================================
    * SDR STARTUP
    * =========================================================
    */
   if (cmd == "startup")
    {
        if (!initializeSDR())
        {
            return -1;
        }

        return 0;
    }

    /*if (cmd == "startup")
    {
        AD9361Controller ad9361;

        if (!ad9361.init())
        {
            cout << "AD9361 initialization failed." << endl;
            return -1;
        }

        const uint64_t DEFAULT_RX_SAMPLE_RATE = 2083340ULL;
        const uint64_t DEFAULT_RX_BANDWIDTH  = 200000ULL;

        if (!ad9361.setRxSampleRate(DEFAULT_RX_SAMPLE_RATE))
        {
            cout << "Failed to set default RX Sample Rate."
                << endl;
            return -1;
        }

        if (!ad9361.setRxBandwidth(DEFAULT_RX_BANDWIDTH))
        {
            cout << "Failed to set default RX Bandwidth."
                << endl;
            return -1;
        }        

        cout << endl;
        cout << "====================================" << endl;
        cout << "       SDR Startup Configuration" << endl;
        cout << "====================================" << endl;

        cout << "RX Sample Rate : "
            << DEFAULT_RX_SAMPLE_RATE
            << " Hz" << endl;

        cout << "RX Bandwidth   : "
            << DEFAULT_RX_BANDWIDTH
            << " Hz" << endl;

        cout << "Startup configuration completed."
            << endl;

        return 0;
    }    */

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

            // if (parameter == "frequency")
            // {
            //     uint64_t value = 0;

            //     if (argc == 4)
            //     {
            //         if (!ad9361.getRxFrequency(value))
            //             return -1;

            //         cout << "RX LO = "
            //              << value
            //              << " Hz" << endl;

            //         return 0;
            //     }

            //     value = strtoull(
            //         argv[4],
            //         nullptr,
            //         0);

            //     if (!ad9361.setRxFrequency(value))
            //     {
            //         cout << "Failed to set RX frequency"
            //              << endl;
            //         return -1;
            //     }

            //     cout << "RX LO = "
            //          << value
            //          << " Hz" << endl;

            //     return 0;
            // }

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

                // User input is in MHz
                uint64_t frequency_mhz =
                    strtoull(
                        argv[4],
                        nullptr,
                        0);

                value = frequency_mhz * 1000000ULL;

                if (!ad9361.setRxFrequency(value))
                {
                    cout << "Failed to set RX frequency"
                        << endl;
                    return -1;
                }

                cout << "RX LO = "
                    << value
                    << " Hz ("
                    << frequency_mhz
                    << " MHz)"
                    << endl;

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

            cout << "Channel : RX1" << endl;

            cout << "Samples : "
                << samples
                << " IQ samples"
                << endl;

            cout << "Format  : I/Q, 16-bit" << endl;

            cout << "Data    : 32-bit per IQ sample" << endl;

            cout << "Bytes   : "
                << samples * sizeof(int16_t) * 2
                << endl;

            cout << "Output  : "
                << filename
                << endl;


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


        // -----------------------------------------------------
        // Send captured IQ data over USB0
        // -----------------------------------------------------

        size_t iq_bytes =
            samples * sizeof(int16_t) * 2;

        cout << endl;
        cout << "====================================" << endl;
        cout << " Sending IQ Capture over USB0" << endl;
        cout << "====================================" << endl;

        cout << "IQ samples : "
            << samples
            << endl;

        cout << "IQ bytes   : "
            << iq_bytes
            << endl;


        // Maximum L2 payload = 8192 bytes
        if (iq_bytes > L2_MAX_PAYLOAD_SIZE)
        {
            cout << "IQ capture is larger than one L2 packet."
                << endl;

            cout << "Maximum payload : "
                << L2_MAX_PAYLOAD_SIZE
                << " bytes"
                << endl;

            capture.close();
            return -1;
        }


        // Open captured IQ file
        std::ifstream iq_file(
            filename,
            std::ios::binary);

        if (!iq_file)
        {
            cout << "Failed to open IQ file for Ethernet transmission."
                << endl;

            capture.close();
            return -1;
        }


        // Read complete IQ data
        std::vector<uint8_t> iq_data(iq_bytes);

        iq_file.read(
            reinterpret_cast<char*>(iq_data.data()),
            iq_bytes);

        if (iq_file.gcount() !=
            static_cast<std::streamsize>(iq_bytes))
        {
            cout << "Failed to read complete IQ file."
                << endl;

            iq_file.close();
            capture.close();

            return -1;
        }

        iq_file.close();


        // Send IQ packet
        cout << "Sending IQ buffer over USB0..."
            << endl;

        bool send_ok =
            l2_send_packet(
                "usb0",
                L2_ETHERTYPE_IQ,
                L2_ID_IQ,
                iq_data.data(),
                static_cast<uint16_t>(iq_bytes));


        // Close IIO
        capture.close();


        if (!send_ok)
        {
            cout << "IQ packet transmission failed."
                << endl;

            return -1;
        }

        cout << "IQ packet transmitted successfully."
            << endl;

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
        * sdr_app dma
        */
        if (argc == 2)
        {
            if (dma_init(&psd_dma, PSD_DMA_BASE_ADDR) != 0)
            {
                cout << "PSD DMA initialization failed." << endl;
                return -1;
            }

            dma_print_status(&psd_dma);
            dma_close(&psd_dma);
        }

        /*
        * sdr_app dma <subcommand>
        */
        else if (argc == 3)
        {
            string subcmd(argv[2]);


            /*
            * DMA RESET
            */
            if (subcmd == "reset")
            {
                if (dma_init(&psd_dma, PSD_DMA_BASE_ADDR) != 0)
                {
                    cout << "PSD DMA initialization failed." << endl;
                    return -1;
                }

                dma_reset(&psd_dma);

                cout << "DMA Reset Done" << endl;

                dma_print_status(&psd_dma);

                dma_close(&psd_dma);
            }


            /*
            * DMA CAPTURE
            */
            else if (subcmd == "capture")
            {

                if (dma_init(&psd_dma, PSD_DMA_BASE_ADDR) != 0)
                {
                    cout << "PSD DMA initialization failed." << endl;
                    return -1;
                }                
                const uint32_t dma_buffer_addr = 0x3F000000;
                const uint32_t dma_length = 8192;


                cout << endl;
                cout << "====================================" << endl;
                cout << " AXI DMA S2MM Capture - PSD" << endl;
                cout << "====================================" << endl;

                cout << "DMA Base      : 0x"
                    << hex << PSD_DMA_BASE_ADDR
                    << dec << endl;

                cout << "UIO           : /dev/uio0" << endl;

                cout << "Buffer Address: 0x"
                    << hex << dma_buffer_addr
                    << dec << endl;

                cout << "Transfer Length: "
                    << dma_length << " bytes"
                    << endl;


                /*
                * Open FPGA register controller
                */
                if (!fpga.open())
                {
                    cout << "FPGA Open Failed" << endl;

                    dma_close(&psd_dma);

                    return -1;
                }


                /*
                * Open UIO interrupt
                */
                if (dma_irq_init(
                        &psd_dma,
                        "/dev/uio0") != 0)
                {
                    cout << "DMA UIO initialization failed."
                        << endl;

                    fpga.close();

                    dma_close(&psd_dma);

                    return -1;
                }


                /*
                * Start DMA first
                */
                dma_start_s2mm(
                    &psd_dma,
                    dma_buffer_addr,
                    dma_length
                );


                /*
                * Enable AXI-Stream data generation
                */
                fpga.setSoftStart(true);


                cout << endl;
                cout << "Waiting for DMA IRQ..."
                    << endl;


                /*
                * Wait for DMA interrupt
                */
                bool capture_ok =
                    dma_wait_for_completion(
                        &psd_dma,
                        5000
                    );


                /*
                * Stop AXI-Stream data generation
                */
                fpga.setSoftStart(false);


                /*
                * Report result and dump buffer
                */
                if (capture_ok)
                {
                    cout << "DMA Capture Completed"
                        << endl;


                    dma_dump_buffer(
                        dma_buffer_addr,
                        dma_length
                    );


                    cout << endl;
                    cout << "Sending DMA buffer over USB0..."
                        << endl;


                    const uint8_t *dma_data =
                        dma_map_buffer(
                            dma_buffer_addr,
                            dma_length
                        );


                    if (dma_data == nullptr)
                    {
                        cout << "Failed to map DMA buffer for PSD send."
                            << endl;
                    }
                    else
                    {
                        bool send_ok =
                            l2_send_packet(
                                "usb0",
                                L2_ETHERTYPE_PSD,
                                L2_ID_PSD,
                                dma_data,
                                dma_length
                            );


                        dma_unmap_buffer(
                            dma_data,
                            dma_length
                        );


                        if (send_ok)
                        {
                            cout << "PSD packet transmitted successfully."
                                << endl;
                        }
                        else
                        {
                            cout << "PSD packet transmission failed."
                                << endl;
                        }
                    }
                }
                else
                {
                    cout << "DMA Capture Failed"
                        << endl;
                }


                /*
                * Print final DMA status
                */
                dma_print_status(&psd_dma);


                /*
                * Close UIO / DMA
                */
                dma_irq_close(&psd_dma);
                dma_close(&psd_dma);

                fpga.close();
            }

    /*
    * -----------------------------------------------------
    * DMA AUDIO CAPTURE
    * -----------------------------------------------------
    */
    else if (subcmd == "audio")
    {
        const uint32_t audio_buffer_addr = 0x3F002000;
        const uint32_t audio_dma_length  = 1024;

        cout << endl;
        cout << "====================================" << endl;
        cout << " AXI DMA S2MM Capture - AUDIO" << endl;
        cout << "====================================" << endl;

        cout << "DMA Base       : 0x"
            << hex << AUDIO_DMA_BASE_ADDR
            << dec << endl;

        cout << "UIO            : /dev/uio1" << endl;

        cout << "Buffer Address : 0x"
            << hex << audio_buffer_addr
            << dec << endl;

        cout << "Transfer Length: "
            << audio_dma_length
            << " bytes"
            << endl;

        cout << "Samples        : 512 x 16-bit"
            << endl;


        /*
        * ---------------------------------------------
        * Initialize Audio AXI DMA
        * ---------------------------------------------
        */
        if (dma_init(
                &audio_dma,
                AUDIO_DMA_BASE_ADDR) != 0)
        {
            cout << "Audio DMA initialization failed."
                << endl;

            return -1;
        }


        /*
        * ---------------------------------------------
        * Open FPGA register controller
        * ---------------------------------------------
        */
        if (!fpga.open())
        {
            cout << "FPGA Open Failed"
                << endl;

            dma_close(&audio_dma);

            return -1;
        }


        /*
        * ---------------------------------------------
        * Open UIO interrupt
        * ---------------------------------------------
        */
        if (dma_irq_init(
                &audio_dma,
                "/dev/uio1") != 0)
        {
            cout << "Audio DMA UIO initialization failed."
                << endl;

            fpga.close();
            dma_close(&audio_dma);

            return -1;
        }


        /*
        * ---------------------------------------------
        * Start Audio DMA first
        * ---------------------------------------------
        */
        dma_start_s2mm(
            &audio_dma,
            audio_buffer_addr,
            audio_dma_length
        );

        /*
        * ---------------------------------------------
        * Enable Audio Modulator
        *
        * Register Addr 4
        * ---------------------------------------------
        */
        //fpga.setModEnable(true);


        /*
        * ---------------------------------------------
        * Enable AXI-Stream Audio data generation
        *
        * Same trigger as PSD for now.
        * ---------------------------------------------
        */
        fpga.setSendPacket(true);


        cout << endl;
        cout << "Waiting for Audio DMA IRQ..."
            << endl;


        /*
        * ---------------------------------------------
        * Wait for DMA completion
        * ---------------------------------------------
        */
        bool capture_ok =
            dma_wait_for_completion(
                &audio_dma,
                5000
            );


        /*
        * ---------------------------------------------
        * Stop AXI-Stream data generation
        * ---------------------------------------------
        */
        fpga.setSendPacket(false);

        /*
        * ---------------------------------------------
        * Disable Audio Modulator
        *
        * Register Addr 4
        * ---------------------------------------------
        */
        //fpga.setModEnable(false);


        /*
        * ---------------------------------------------
        * Report result
        * ---------------------------------------------
        */
        // if (capture_ok)
        // {
        //     cout << "Audio DMA Capture Completed"
        //         << endl;

        //     cout << "Dumping Audio buffer..."
        //         << endl;

        //     dma_dump_buffer(
        //         audio_buffer_addr,
        //         audio_dma_length
        //     );
        // }

        if (capture_ok)
        {
            dma_dump_buffer(
                audio_buffer_addr,
                audio_dma_length);

            const uint8_t *audio_data =
                dma_map_buffer(
                    audio_buffer_addr,
                    audio_dma_length);

            if (audio_data == nullptr)
            {
                cout << "Failed to map Audio DMA buffer." << endl;
            }
            else
            {
                bool tx_ok = l2_send_packet(
                    "usb0",
                    L2_ETHERTYPE_AUDIO,
                    L2_ID_AUDIO,
                    audio_data,
                    audio_dma_length);

                if (tx_ok)
                {
                    cout << "Audio L2 frame sent successfully."
                        << endl;
                }
                else
                {
                    cout << "Audio L2 frame send failed."
                        << endl;
                }

                dma_unmap_buffer(
                    audio_data,
                    audio_dma_length);
            }
        }
        else
        {
            cout << "Audio DMA Capture Failed"
                << endl;
        }


        /*
        * ---------------------------------------------
        * Final DMA status
        * ---------------------------------------------
        */
        dma_print_status(&audio_dma);


        /*
        * ---------------------------------------------
        * Close UIO / DMA
        * ---------------------------------------------
        */
        dma_irq_close(&audio_dma);
        dma_close(&audio_dma);

        fpga.close();
    }            


            /*
            * UNKNOWN DMA COMMAND
            */
            else
            {
                cout << "Unknown DMA command: "
                    << subcmd
                    << endl;

                printHelp();
            }
        }


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
    * SOFT RESET
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app reset
    *
    */
    if (cmd == "reset")
    {
        fpga.setSoftReset(true);

        cout << "Soft Reset pulse generated." << endl;
    }

    /*
    * ---------------------------------------------------------
    * SOFT START
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app soft-start <0|1>
    *
    */
    else if (cmd == "soft-start")
    {
        if (argc < 3)
        {
            cout << "Missing Soft Start value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);
            //strtoul(argv[2], nullptr, 10);  // Decimal input

        if (value > 1)
        {
            cout << "Invalid Soft Start value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setSoftStart(value);

        cout << "Soft Start = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * SEND PACKET
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app send-packet <0|1>
    *
    */
    else if (cmd == "send-packet")
    {
        if (argc < 3)
        {
            cout << "Missing Send Packet value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);
            //strtoul(argv[2], nullptr, 10);  // Decimal input

        if (value > 1)
        {
            cout << "Invalid Send Packet value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setSendPacket(value);

        cout << "Send Packet = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * MODULATION ENABLE
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app mod-enable <0|1>
    *
    */
    else if (cmd == "mod-enable")
    {
        if (argc < 3)
        {
            cout << "Missing Modulation Enable value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);
            //strtoul(argv[2], nullptr, 10);  // Decimal input

        if (value > 1)
        {
            cout << "Invalid Modulation Enable value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setModEnable(value);

        cout << "Modulation Enable = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * SQUELCH ENABLE
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app squelch-enable <0|1>
    *
    */
    else if (cmd == "squelch-enable")
    {
        if (argc < 3)
        {
            cout << "Missing Squelch Enable value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);
           //strtoul(argv[2], nullptr, 10);  // Decimal input

        if (value > 1)
        {
            cout << "Invalid Squelch Enable value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setSquelchEnable(value);

        cout << "Squelch Enable = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * MGC2 ON
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app mgc2-on <0|1>
    *
    */
    else if (cmd == "mgc2-on")
    {
        if (argc < 3)
        {
            cout << "Missing MGC2 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 1)
        {
            cout << "Invalid MGC2 value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setMGC2On(value);

        cout << "MGC2 On = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * AGC1 ON
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app agc1-on <0|1>
    *
    */
    else if (cmd == "agc1-on")
    {
        if (argc < 3)
        {
            cout << "Missing AGC1 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 1)
        {
            cout << "Invalid AGC1 value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setAGC1On(value);

        cout << "AGC1 On = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * AGC2 ON
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app agc2-on <0|1>
    *
    */
    else if (cmd == "agc2-on")
    {
        if (argc < 3)
        {
            cout << "Missing AGC2 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 1)
        {
            cout << "Invalid AGC2 value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setAGC2On(value);

        cout << "AGC2 On = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * AMP OK
    * ---------------------------------------------------------
    *
    * Usage:
    *   ./sdr_app amp-ok <0|1>
    *
    */
    else if (cmd == "amp-ok")
    {
        if (argc < 3)
        {
            cout << "Missing Amp OK value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 1)
        {
            cout << "Invalid Amp OK value." << endl;
            cout << "Allowed values: 0 or 1" << endl;
            return -1;
        }

        fpga.setAmpOK(value);

        cout << "Amp OK = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * BANDWIDTH
    * ---------------------------------------------------------
    *
    * 4-bit value
    *
    * Usage:
    *   ./sdr_app bandwidth <value>
    *
    */
    else if (cmd == "bandwidth")
    {
        if (argc < 3)
        {
            cout << "Missing bandwidth value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xF)
        {
            cout << "Invalid bandwidth value." << endl;
            cout << "Range: 0 to 15" << endl;
            return -1;
        }

        fpga.setBandwidth(value);

        cout << "Bandwidth Select = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * MODE SELECT
    * ---------------------------------------------------------
    *
    * 8-bit value
    *
    * Usage:
    *   ./sdr_app mode-select <value>
    *
    */
    else if (cmd == "mode-select")
    {
        if (argc < 3)
        {
            cout << "Missing Mode Select value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xFF)
        {
            cout << "Invalid Mode Select value." << endl;
            cout << "Range: 0 to 255" << endl;
            return -1;
        }

        fpga.setModeSelect(value);

        cout << "Mode Select = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * LED TIMER
    * ---------------------------------------------------------
    *
    * 32-bit value
    *
    * Usage:
    *   ./sdr_app led-timer <value>
    *
    */
    else if (cmd == "led-timer")
    {
        if (argc < 3)
        {
            cout << "Missing LED timer value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        fpga.setLedTimer(value);

        cout << "LED Timer = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * MESSAGE VALUE
    * ---------------------------------------------------------
    *
    * 32-bit value
    *
    * Usage:
    *   ./sdr_app msg-value <value>
    *
    */
    else if (cmd == "msg-value")
    {
        if (argc < 3)
        {
            cout << "Missing Message Value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        fpga.setMsgValue(value);

        cout << "Message Value = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * A COEFFICIENT 1
    * ---------------------------------------------------------
    *
    * 8-bit value
    *
    * Usage:
    *   ./sdr_app a-coeff1 <value>
    *
    */
    else if (cmd == "a-coeff1")
    {
        if (argc < 3)
        {
            cout << "Missing A Coefficient 1 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xFF)
        {
            cout << "Invalid A Coefficient 1 value." << endl;
            cout << "Range: 0 to 255" << endl;
            return -1;
        }

        fpga.setACoeff1(value);

        cout << "A Coefficient 1 = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * A COEFFICIENT 2
    * ---------------------------------------------------------
    *
    * 8-bit value
    *
    * Usage:
    *   ./sdr_app a-coeff2 <value>
    *
    */
    else if (cmd == "a-coeff2")
    {
        if (argc < 3)
        {
            cout << "Missing A Coefficient 2 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xFF)
        {
            cout << "Invalid A Coefficient 2 value." << endl;
            cout << "Range: 0 to 255" << endl;
            return -1;
        }

        fpga.setACoeff2(value);

        cout << "A Coefficient 2 = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * REF1
    * ---------------------------------------------------------
    *
    * 24-bit value
    *
    * Usage:
    *   ./sdr_app ref1 <value>
    *
    */
    else if (cmd == "ref1")
    {
        if (argc < 3)
        {
            cout << "Missing Ref1 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xFFFFFF)
        {
            cout << "Invalid Ref1 value." << endl;
            cout << "Range: 0 to 16777215" << endl;
            return -1;
        }

        fpga.setRef1(value);

        cout << "Ref1 = "
            << value
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * REF2
    * ---------------------------------------------------------
    *
    * 24-bit value
    *
    * Usage:
    *   ./sdr_app ref2 <value>
    *
    */
    else if (cmd == "ref2")
    {
        if (argc < 3)
        {
            cout << "Missing Ref2 value" << endl;
            return -1;
        }

        uint32_t value =
            strtoul(argv[2], nullptr, 0);

        if (value > 0xFFFFFF)
        {
            cout << "Invalid Ref2 value." << endl;
            cout << "Range: 0 to 16777215" << endl;
            return -1;
        }

        fpga.setRef2(value);

        cout << "Ref2 = "
            << value
            << endl;
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

        uint32_t control =
            fpga.getControl();

        uint32_t bandwidth =
            fpga.getBandwidth();

        uint32_t mode =
            fpga.getModeSelect();

        uint32_t ledTimer =
            fpga.getLedTimer();

        uint32_t message =
            fpga.getMsgValue();

        uint32_t coefficients =
            fpga.getCoefficients();

        uint32_t ref1 =
            fpga.getRef1();

        uint32_t ref2 =
            fpga.getRef2();

        cout << "CONTROL      : "
            << control
            << endl;

        cout << "BANDWIDTH    : "
            << bandwidth
            << endl;

        cout << "MODE         : "
            << mode
            << endl;

        cout << "LED TIMER    : "
            << ledTimer
            << endl;

        cout << "MESSAGE      : "
            << message
            << endl;

        cout << "A COEFF1     : "
            << (coefficients & 0xFF)
            << endl;

        cout << "A COEFF2     : "
            << ((coefficients >> 8) & 0xFF)
            << endl;

        cout << "REF1         : "
            << ref1
            << endl;

        cout << "REF2         : "
            << ref2
            << endl;
    }

    /*
    * ---------------------------------------------------------
    * UNKNOWN COMMAND
    * ---------------------------------------------------------
    */
    else
    {
        cout << "Unknown command: "
            << cmd
            << endl
            << endl;

        printHelp();

        fpga.close();

        return -1;
    }

    fpga.close();

    return 0;
    }