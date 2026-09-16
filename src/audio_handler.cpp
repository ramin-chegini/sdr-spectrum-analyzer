#include <iostream>
#include <thread>
#include <atomic>
#include <fstream>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "audio_handler.h"
#include "l2_protocol.h"
#include "network_config.h"
#include "RegisterController.h"

using namespace std;

namespace
{
    constexpr uint32_t AUDIO_BUFFER_ADDR = 0x3F002000;
    constexpr uint32_t AUDIO_DMA_LENGTH  = 1024;

    constexpr const char *AUDIO_UIO = "/dev/uio1";

    /*
     * PID file for background Audio streaming process.
     */
    constexpr const char *AUDIO_PID_FILE = "/tmp/sdr_audio.pid";

    /*
     * Stop flag used by the Audio worker process.
     *
     * Signal handler only changes this flag.
     * Actual cleanup is performed by the worker.
     */
    volatile sig_atomic_t audio_stop_requested = 0;


    /*
     * -----------------------------------------------------
     * SIGTERM handler
     * -----------------------------------------------------
     */
    void audio_signal_handler(int)
    {
        audio_stop_requested = 1;
    }


    /*
     * -----------------------------------------------------
     * Check whether a process is alive
     * -----------------------------------------------------
     */
    bool process_is_alive(pid_t pid)
    {
        if (pid <= 0)
            return false;

        if (::kill(pid, 0) == 0)
            return true;

        return (errno == EPERM);
    }


    /*
     * -----------------------------------------------------
     * Read Audio worker PID
     * -----------------------------------------------------
     */
    pid_t read_audio_pid()
    {
        ifstream pid_file(AUDIO_PID_FILE);

        if (!pid_file.is_open())
            return -1;

        long pid_value = -1;
        pid_file >> pid_value;

        if (!pid_file.good() && !pid_file.eof())
            return -1;

        if (pid_value <= 0)
            return -1;

        return static_cast<pid_t>(pid_value);
    }


    /*
     * -----------------------------------------------------
     * Write Audio worker PID
     * -----------------------------------------------------
     */
    bool write_audio_pid(pid_t pid)
    {
        ofstream pid_file(AUDIO_PID_FILE,
                          ios::trunc);

        if (!pid_file.is_open())
        {
            cerr << "[AUDIO] Failed to create PID file: "
                 << AUDIO_PID_FILE
                 << endl;

            return false;
        }

        pid_file << pid << endl;

        return pid_file.good();
    }


    /*
     * -----------------------------------------------------
     * Remove Audio PID file
     * -----------------------------------------------------
     */
    void remove_audio_pid()
    {
        ::unlink(AUDIO_PID_FILE);
    }


    /*
     * -----------------------------------------------------
     * One-Shot Audio DMA Capture + L2 Send
     * -----------------------------------------------------
     */
    bool audio_dma_capture_and_send_internal(
        DmaContext *audio_dma)
    {
        cout << endl;
        cout << "====================================" << endl;
        cout << " AXI DMA S2MM Capture - AUDIO" << endl;
        cout << "====================================" << endl;

        if (dma_init(
                audio_dma,
                AUDIO_DMA_BASE_ADDR) != 0)
        {
            cout << "Audio DMA initialization failed."
                 << endl;

            return false;
        }

        if (dma_irq_init(
                audio_dma,
                AUDIO_UIO) != 0)
        {
            cout << "Audio DMA UIO initialization failed."
                 << endl;

            dma_close(audio_dma);
            return false;
        }

        dma_start_s2mm(
            audio_dma,
            AUDIO_BUFFER_ADDR,
            AUDIO_DMA_LENGTH);

        cout << "Waiting for Audio DMA IRQ..."
             << endl;

        bool capture_ok =
            dma_wait_for_completion(
                audio_dma,
                5000);

        bool send_ok = false;

        if (capture_ok)
        {
            cout << "Audio DMA Capture Completed"
                 << endl;

            const uint8_t *audio_data =
                dma_map_buffer(
                    AUDIO_BUFFER_ADDR,
                    AUDIO_DMA_LENGTH);

            if (audio_data == nullptr)
            {
                cout << "Failed to map Audio DMA buffer."
                     << endl;
            }
            else
            {
                send_ok =
                    l2_send_packet(
                        SDR_NET_INTERFACE,
                        L2_ETHERTYPE_AUDIO,
                        L2_ID_AUDIO,
                        audio_data,
                        AUDIO_DMA_LENGTH);

                dma_unmap_buffer(
                    audio_data,
                    AUDIO_DMA_LENGTH);

                if (send_ok)
                {
                    cout << "Audio L2 frame sent successfully."
                         << endl;
                }
                else
                {
                    cout << "Audio L2 frame send failed."
                         << endl;
                }
            }
        }
        else
        {
            cout << "Audio DMA Capture Failed"
                 << endl;
        }

        dma_irq_close(audio_dma);
        dma_close(audio_dma);

        return capture_ok && send_ok;
    }


    /*
     * -----------------------------------------------------
     * Background Audio Worker
     * -----------------------------------------------------
     */
    void audio_background_worker()
    {
        DmaContext audio_dma{};

        RegisterController fpga;

        bool fpga_opened = false;
        bool dma_initialized = false;
        bool irq_initialized = false;

        uint64_t packet_count = 0;

        cout << endl;
        cout << "====================================" << endl;
        cout << "     AUDIO STREAMING STARTED" << endl;
        cout << " PID : " << getpid() << endl;
        cout << "====================================" << endl;


        /*
         * -------------------------------------------------
         * Open FPGA Register Controller
         * -------------------------------------------------
         */
        if (!fpga.open())
        {
            cout << "[AUDIO] FPGA RegisterController open failed."
                 << endl;

            remove_audio_pid();
            return;
        }

        fpga_opened = true;


        /*
         * -------------------------------------------------
         * Enable Modulation / Audio Source
         * -------------------------------------------------
         */
        cout << "[AUDIO] Setting ModEnable = 1"
             << endl;

        fpga.setModEnable(true);


        /*
         * -------------------------------------------------
         * Initialize Audio AXI DMA
         * -------------------------------------------------
         */
        if (dma_init(
                &audio_dma,
                AUDIO_DMA_BASE_ADDR) != 0)
        {
            cout << "[AUDIO] Audio DMA initialization failed."
                 << endl;

            fpga.setModEnable(false);

            fpga.close();

            remove_audio_pid();

            return;
        }

        dma_initialized = true;


        /*
         * -------------------------------------------------
         * Initialize Audio DMA IRQ
         * -------------------------------------------------
         */
        if (dma_irq_init(
                &audio_dma,
                AUDIO_UIO) != 0)
        {
            cout << "[AUDIO] Audio DMA UIO initialization failed."
                 << endl;

            dma_close(&audio_dma);

            fpga.setModEnable(false);

            fpga.close();

            remove_audio_pid();

            return;
        }

        irq_initialized = true;


        /*
         * -------------------------------------------------
         * Install signal handlers
         * -------------------------------------------------
         */
        audio_stop_requested = 0;

        struct sigaction sa{};

        sa.sa_handler = audio_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT,  &sa, nullptr);


        /*
         * -------------------------------------------------
         * Continuous Audio Streaming
         * -------------------------------------------------
         */
        while (!audio_stop_requested)
        {
            /*
             * Start one DMA transfer.
             */
            dma_start_s2mm(
                &audio_dma,
                AUDIO_BUFFER_ADDR,
                AUDIO_DMA_LENGTH);


            /*
             * Wait for DMA completion.
             */
            bool capture_ok =
                dma_wait_for_completion(
                    &audio_dma,
                    5000);

            /*
             * Check stop request after DMA wait.
             */
            if (audio_stop_requested)
                break;


            if (!capture_ok)
            {
                cout << "[AUDIO] DMA Capture Failed."
                     << endl;

                break;
            }


            /*
             * Map captured Audio buffer.
             */
            const uint8_t *audio_data =
                dma_map_buffer(
                    AUDIO_BUFFER_ADDR,
                    AUDIO_DMA_LENGTH);

            if (audio_data == nullptr)
            {
                cout << "[AUDIO] Failed to map DMA buffer."
                     << endl;

                break;
            }


            /*
             * Send Audio packet to GUI.
             */
            bool send_ok =
                l2_send_packet(
                    SDR_NET_INTERFACE,
                    L2_ETHERTYPE_AUDIO,
                    L2_ID_AUDIO,
                    audio_data,
                    AUDIO_DMA_LENGTH);


            dma_unmap_buffer(
                audio_data,
                AUDIO_DMA_LENGTH);


            if (!send_ok)
            {
                cout << "[AUDIO] L2 frame send failed."
                     << endl;

                break;
            }


            packet_count++;


            /*
             * Print status every 100 packets.
             */
            if ((packet_count % 100) == 0)
            {
                cout << "[AUDIO] Packets sent = "
                     << packet_count
                     << endl;
            }
        }


        /*
         * -------------------------------------------------
         * Cleanup
         * -------------------------------------------------
         */
        cout << endl;
        cout << "[AUDIO] Cleaning up..."
             << endl;


        if (irq_initialized)
        {
            dma_irq_close(&audio_dma);
        }

        if (dma_initialized)
        {
            dma_close(&audio_dma);
        }


        /*
         * IMPORTANT:
         * Always disable ModEnable during normal cleanup.
         */
        if (fpga_opened)
        {
            cout << "[AUDIO] Setting ModEnable = 0"
                 << endl;

            fpga.setModEnable(false);

            fpga.close();
        }


        remove_audio_pid();


        cout << endl;
        cout << "====================================" << endl;
        cout << "      AUDIO STREAMING STOPPED" << endl;
        cout << "Packets sent : "
             << packet_count
             << endl;
        cout << "====================================" << endl;
    }

}


/*
 * ---------------------------------------------------------
 * Public One-Shot API
 * ---------------------------------------------------------
 */
bool audio_dma_capture_and_send(
    DmaContext *audio_dma)
{
    if (audio_dma == nullptr)
    {
        cout << "[AUDIO] Invalid DMA context."
             << endl;

        return false;
    }

    return audio_dma_capture_and_send_internal(
        audio_dma);
}


/*
 * ---------------------------------------------------------
 * Start Continuous Audio Streaming
 * ---------------------------------------------------------
 *
 * Creates an independent background process.
 *
 * The parent process returns immediately.
 */
bool audio_stream_start(
    DmaContext *)
{
    /*
     * Check existing PID file.
     */
    pid_t old_pid = read_audio_pid();

    if (old_pid > 0)
    {
        if (process_is_alive(old_pid))
        {
            cout << "[AUDIO] Stream already running."
                 << endl;

            cout << "[AUDIO] PID = "
                 << old_pid
                 << endl;

            return true;
        }

        /*
         * Stale PID file.
         */
        remove_audio_pid();
    }


    /*
     * Create background process.
     */
    pid_t pid = fork();

    if (pid < 0)
    {
        cerr << "[AUDIO] fork() failed: "
             << strerror(errno)
             << endl;

        return false;
    }


    /*
     * -----------------------------------------------------
     * Child
     * -----------------------------------------------------
     */
    // if (pid == 0)
    // {
    //     /*
    //      * Create a new session.
    //      */
    //     if (setsid() < 0)
    //     {
    //         _exit(EXIT_FAILURE);
    //     }


    //     /*
    //      * Store our PID.
    //      */
    //     pid_t child_pid = getpid();

    //     if (!write_audio_pid(child_pid))
    //     {
    //         _exit(EXIT_FAILURE);
    //     }


    //     /*
    //      * Run Audio worker.
    //      */
    //     audio_background_worker();


    //     /*
    //      * Child must terminate here.
    //      */
    //     _exit(EXIT_SUCCESS);
    // }

    if (pid == 0)
    {
        /*
        * Create a new session.
        */
        if (setsid() < 0)
        {
            _exit(EXIT_FAILURE);
        }

        /*
        * Redirect worker output to a log file.
        * This prevents the terminal from being flooded.
        */
        freopen("/tmp/sdr_audio.log", "a", stdout);
        freopen("/tmp/sdr_audio.log", "a", stderr);

        /*
        * Store our PID.
        */
        pid_t child_pid = getpid();

        if (!write_audio_pid(child_pid))
        {
            _exit(EXIT_FAILURE);
        }

        /*
        * Run Audio worker.
        */
        audio_background_worker();

        /*
        * Child must terminate here.
        */
        _exit(EXIT_SUCCESS);
    }


    /*
     * -----------------------------------------------------
     * Parent
     * -----------------------------------------------------
     */

    cout << "[AUDIO] Audio streaming started."
         << endl;

    cout << "[AUDIO] Background PID = "
         << pid
         << endl;

    return true;
}


/*
 * ---------------------------------------------------------
 * Stop Continuous Audio Streaming
 * ---------------------------------------------------------
 */
bool audio_stream_stop()
{
    pid_t pid = read_audio_pid();

    if (pid <= 0)
    {
        cout << "[AUDIO] Stream is not running."
             << endl;

        return true;
    }


    /*
     * Check whether process is alive.
     */
    if (!process_is_alive(pid))
    {
        cout << "[AUDIO] Audio process is not running."
             << endl;

        remove_audio_pid();

        return true;
    }


    cout << "[AUDIO] Stopping Audio stream..."
         << endl;

    cout << "[AUDIO] Sending SIGTERM to PID "
         << pid
         << endl;


    /*
     * Ask worker to stop.
     */
    if (::kill(pid, SIGTERM) != 0)
    {
        cerr << "[AUDIO] Failed to send SIGTERM: "
             << strerror(errno)
             << endl;

        return false;
    }


    /*
     * Wait for the worker to terminate.
     *
     * Since this process is not the parent of the
     * background worker, use kill(pid, 0) to monitor it.
     */
    constexpr int MAX_WAIT_MS = 6000;
    constexpr int POLL_MS = 100;

    int waited_ms = 0;

    while (waited_ms < MAX_WAIT_MS)
    {
        if (!process_is_alive(pid))
            break;

        usleep(POLL_MS * 1000);
        waited_ms += POLL_MS;
    }


    if (process_is_alive(pid))
    {
        cout << "[AUDIO] Worker did not stop within timeout."
             << endl;

        return false;
    }


    remove_audio_pid();

    cout << "[AUDIO] Audio stream stopped."
         << endl;

    return true;
}


/*
 * ---------------------------------------------------------
 * Streaming State
 * ---------------------------------------------------------
 */
bool audio_stream_is_running()
{
    pid_t pid = read_audio_pid();

    if (pid <= 0)
        return false;

    if (!process_is_alive(pid))
    {
        remove_audio_pid();
        return false;
    }

    return true;
}