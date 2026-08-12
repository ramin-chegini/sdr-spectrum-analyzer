#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>

#include "RegisterController.h"
#include "dma.h"

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