#include <iostream>
#include <thread>
#include <atomic>
#include <fstream>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <iomanip>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

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
             * Wait for DMA completion.
             */
            bool capture_ok =
                dma_wait_for_completion(
                    &audio_dma,
                    5000);

        /*
        * ---------------------------------------------
        * Stop AXI-Stream data generation
        * ---------------------------------------------
        */
        fpga.setSendPacket(false);                    

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

/*
 * ---------------------------------------------------------
 * Timed Audio Capture -> WAVE File
 * ---------------------------------------------------------
 *
 * Usage:
 *   sdr_app audio capture <time[sec]> [filename]
 *
 * Format:
 *   Mono, PCM 16-bit, 65104 Hz
 */
bool audio_capture_to_wave(
    uint32_t duration_sec,
    const char *filename)
{
    constexpr uint32_t SAMPLE_RATE     = 65104;
    // constexpr uint32_t SAMPLE_RATE     = 32552;
    constexpr uint16_t CHANNELS        = 1;
    constexpr uint16_t BITS_PER_SAMPLE = 16;

#pragma pack(push, 1)
    struct WaveHeader
    {
        char     riff[4];
        uint32_t riff_size;
        char     wave[4];
        char     fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        char     data[4];
        uint32_t data_size;
    };
#pragma pack(pop)

    static_assert(sizeof(WaveHeader) == 44,
                  "Invalid WAVE header size");

    if (duration_sec == 0 ||
        filename == nullptr ||
        filename[0] == '\0')
    {
        cout << "[AUDIO] Invalid capture parameters."
             << endl;
        return false;
    }

    ofstream wave_file(
        filename,
        ios::binary | ios::trunc);

    if (!wave_file.is_open())
    {
        cout << "[AUDIO] Failed to create WAVE file: "
             << filename
             << endl;
        return false;
    }

    WaveHeader header =
    {
        {'R','I','F','F'},
        36,
        {'W','A','V','E'},
        {'f','m','t',' '},
        16,
        1,
        CHANNELS,
        SAMPLE_RATE,
        SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8),
        static_cast<uint16_t>(
            CHANNELS * (BITS_PER_SAMPLE / 8)),
        BITS_PER_SAMPLE,
        {'d','a','t','a'},
        0
    };

    wave_file.write(
        reinterpret_cast<const char *>(&header),
        sizeof(header));

    if (!wave_file.good())
    {
        cout << "[AUDIO] Failed to write WAVE header."
             << endl;
        wave_file.close();
        return false;
    }

    RegisterController fpga;
    DmaContext audio_dma{};

    if (!fpga.open())
    {
        cout << "[AUDIO] FPGA RegisterController open failed."
             << endl;
        wave_file.close();
        return false;
    }


    const char *DMA_LOG_FILE = "/tmp/sdr_audio_capture.log";

    /*
    * Keep original terminal stdout/stderr.
    */
    int terminal_out = dup(STDOUT_FILENO);
    int terminal_err = dup(STDERR_FILENO);

    /*
    * Open DMA debug log.
    */
    int log_fd = open(
        DMA_LOG_FILE,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644);

    if (log_fd < 0)
    {
        cout << "[AUDIO] Failed to open DMA log file." << endl;
        return false;
    }

    /*
    * Message visible to user.
    */
    dprintf(
        terminal_out,
        "[AUDIO] Capturing audio for %u sec -> %s\n",
        duration_sec,
        filename);

    /*
    * Redirect normal stdout/stderr to log file.
    */
    fflush(stdout);
    fflush(stderr);

    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);    

    if (dma_init(
            &audio_dma,
            AUDIO_DMA_BASE_ADDR) != 0)
    {
        cout << "[AUDIO] Audio DMA initialization failed."
             << endl;

        fpga.close();
        wave_file.close();
        return false;
    }

    if (dma_irq_init(
            &audio_dma,
            AUDIO_UIO) != 0)
    {
        cout << "[AUDIO] Audio DMA UIO initialization failed."
             << endl;

        dma_close(&audio_dma);
        fpga.close();
        wave_file.close();
        return false;
    }

    cout << "[AUDIO] Capture started" << endl;
    cout << "[AUDIO] Duration    : "
         << duration_sec << " sec" << endl;
    cout << "[AUDIO] Sample rate : "
         << SAMPLE_RATE << " Hz" << endl;
    cout << "[AUDIO] File        : "
         << filename << endl;

    fpga.setSendPacket(false);
    fpga.setModEnable(true);

    uint32_t data_size = 0;
    bool capture_ok = true;

    const auto end_time =
        chrono::steady_clock::now() +
        chrono::seconds(duration_sec);

    while (chrono::steady_clock::now() < end_time)
    {
        /*
         * Keep the tested Audio DMA trigger sequence unchanged:
         *
         *   DMA start
         *   SendPacket = 1
         *   wait IRQ
         *   SendPacket = 0
         */
        dma_start_s2mm(
            &audio_dma,
            AUDIO_BUFFER_ADDR,
            AUDIO_DMA_LENGTH);

        fpga.setSendPacket(true);

        bool block_ok =
            dma_wait_for_completion(
                &audio_dma,
                5000);

        fpga.setSendPacket(false);

        if (!block_ok)
        {
            cout << "[AUDIO] DMA capture failed."
                 << endl;

            capture_ok = false;
            break;
        }

        const uint8_t *audio_data =
            dma_map_buffer(
                AUDIO_BUFFER_ADDR,
                AUDIO_DMA_LENGTH);

        if (audio_data == nullptr)
        {
            cout << "[AUDIO] Failed to map DMA buffer."
                 << endl;

            capture_ok = false;
            break;
        }

        // wave_file.write(
        //     reinterpret_cast<const char *>(audio_data),
        //     AUDIO_DMA_LENGTH);

        // PCM 16-bit little-endian

        uint8_t wav_buffer[AUDIO_DMA_LENGTH];

        for (uint32_t i = 0; i < AUDIO_DMA_LENGTH; i += 2)
        {
            wav_buffer[i]     = audio_data[i + 1];
            wav_buffer[i + 1] = audio_data[i];
        }

        wave_file.write(
            reinterpret_cast<const char *>(wav_buffer),
            AUDIO_DMA_LENGTH);        

        dma_unmap_buffer(
            audio_data,
            AUDIO_DMA_LENGTH);

        if (!wave_file.good())
        {
            cout << "[AUDIO] Failed to write WAVE data."
                 << endl;

            capture_ok = false;
            break;
        }

        data_size += AUDIO_DMA_LENGTH;
    }

    fpga.setSendPacket(false);
    fpga.setModEnable(false);

    dma_irq_close(&audio_dma);
    dma_close(&audio_dma);
    fpga.close();

    /*
     * Finalize RIFF/WAVE sizes.
     */
    header.data_size = data_size;
    header.riff_size = 36 + data_size;

    wave_file.seekp(0, ios::beg);
    wave_file.write(
        reinterpret_cast<const char *>(&header),
        sizeof(header));

    wave_file.close();

    /*
    * Flush DMA debug log.
    */
    fflush(stdout);
    fflush(stderr);

    /*
    * Restore terminal stdout/stderr.
    */
    dup2(terminal_out, STDOUT_FILENO);
    dup2(terminal_err, STDERR_FILENO);

    close(terminal_out);
    close(terminal_err);
    close(log_fd);


    /*
    * Capture failed
    */
    if (!capture_ok)
    {
        cout << "[AUDIO] Capture failed." << endl;
        cout << "[AUDIO] DMA log     : "
            << DMA_LOG_FILE << endl;

        return false;
    }


    /*
    * Get final WAV file size
    */
    struct stat file_stat{};

    cout << endl;
    cout << "[AUDIO] Capture completed." << endl;
    cout << "[AUDIO] WAV file    : "
        << filename << endl;

    if (stat(filename, &file_stat) == 0)
    {
        double size_kb =
            static_cast<double>(file_stat.st_size) / 1024.0;

        double size_mb =
            size_kb / 1024.0;

        cout << "[AUDIO] File size   : "
            << file_stat.st_size
            << " bytes ("
            << fixed
            << setprecision(2)
            << size_kb
            << " KB / "
            << size_mb
            << " MB)"
            << endl;
    }

    cout << "[AUDIO] DMA log     : "
        << DMA_LOG_FILE << endl;

    return true;
}

/*
 * =============================================================
 * Audio SINAD Measurement
 * =============================================================
 *
 * Expected test condition:
 *
 *      FM modulation tone = 1 kHz
 *
 * Measurement method:
 *
 *      1. Read PCM16 Mono WAVE file
 *      2. Skip initial startup transient
 *      3. Search fundamental around 1 kHz
 *      4. Least-squares fit:
 *
 *             x[n] = DC + A*cos(w*n) + B*sin(w*n)
 *
 *      5. Remove DC + fundamental
 *      6. Remaining signal = Noise + Distortion
 *
 *      SINAD =
 *
 *          10 * log10(
 *              Total_AC_Power /
 *              Noise_And_Distortion_Power
 *          )
 *
 * Harmonics are NOT removed.
 * Therefore they remain part of distortion.
 */


namespace
{

    constexpr double SINAD_PI =
        3.1415926535897932384626433832795;


    struct SinadWaveData
    {
        uint32_t sample_rate = 0;

        vector<int16_t> samples;
    };


    /*
     * ---------------------------------------------------------
     * Read little-endian 16-bit value
     * ---------------------------------------------------------
     */
    bool sinad_read_u16_le(
        ifstream &file,
        uint16_t &value)
    {
        uint8_t b[2];

        file.read(
            reinterpret_cast<char *>(b),
            2);

        if (!file.good())
            return false;

        value =
            static_cast<uint16_t>(b[0]) |
            (static_cast<uint16_t>(b[1]) << 8);

        return true;
    }


    /*
     * ---------------------------------------------------------
     * Read little-endian 32-bit value
     * ---------------------------------------------------------
     */
    bool sinad_read_u32_le(
        ifstream &file,
        uint32_t &value)
    {
        uint8_t b[4];

        file.read(
            reinterpret_cast<char *>(b),
            4);

        if (!file.good())
            return false;

        value =
            static_cast<uint32_t>(b[0]) |
            (static_cast<uint32_t>(b[1]) << 8) |
            (static_cast<uint32_t>(b[2]) << 16) |
            (static_cast<uint32_t>(b[3]) << 24);

        return true;
    }


    /*
     * ---------------------------------------------------------
     * Load PCM16 Mono WAVE
     * ---------------------------------------------------------
     */
    bool sinad_load_wave(
        const char *filename,
        SinadWaveData &wave)
    {
        ifstream file(
            filename,
            ios::binary);

        if (!file.is_open())
        {
            cout << "[AUDIO] Failed to open WAVE file: "
                 << filename
                 << endl;

            return false;
        }


        char riff_id[4];
        char wave_id[4];

        uint32_t riff_size = 0;


        file.read(riff_id, 4);

        if (!sinad_read_u32_le(
                file,
                riff_size))
        {
            cout << "[AUDIO] Invalid RIFF header."
                 << endl;

            return false;
        }

        file.read(wave_id, 4);


        if (!file.good() ||
            memcmp(riff_id, "RIFF", 4) != 0 ||
            memcmp(wave_id, "WAVE", 4) != 0)
        {
            cout << "[AUDIO] Invalid WAVE file."
                 << endl;

            return false;
        }


        bool fmt_found  = false;
        bool data_found = false;


        uint16_t audio_format    = 0;
        uint16_t channels        = 0;
        uint16_t bits_per_sample = 0;

        uint32_t sample_rate = 0;

        uint32_t data_size = 0;

        streampos data_position{};


        /*
         * Search RIFF chunks.
         */
        while (file.good())
        {
            char chunk_id[4];

            uint32_t chunk_size = 0;


            file.read(chunk_id, 4);

            if (file.gcount() != 4)
                break;


            if (!sinad_read_u32_le(
                    file,
                    chunk_size))
            {
                break;
            }


            streampos chunk_data_position =
                file.tellg();


            /*
             * fmt chunk
             */
            if (memcmp(
                    chunk_id,
                    "fmt ",
                    4) == 0)
            {
                if (chunk_size < 16)
                {
                    cout << "[AUDIO] Invalid fmt chunk."
                         << endl;

                    return false;
                }


                uint32_t byte_rate = 0;

                uint16_t block_align = 0;


                if (!sinad_read_u16_le(
                        file,
                        audio_format) ||

                    !sinad_read_u16_le(
                        file,
                        channels) ||

                    !sinad_read_u32_le(
                        file,
                        sample_rate) ||

                    !sinad_read_u32_le(
                        file,
                        byte_rate) ||

                    !sinad_read_u16_le(
                        file,
                        block_align) ||

                    !sinad_read_u16_le(
                        file,
                        bits_per_sample))
                {
                    cout << "[AUDIO] Failed to read fmt chunk."
                         << endl;

                    return false;
                }


                fmt_found = true;
            }


            /*
             * data chunk
             */
            else if (memcmp(
                         chunk_id,
                         "data",
                         4) == 0)
            {
                data_position =
                    chunk_data_position;

                data_size =
                    chunk_size;

                data_found = true;
            }


            /*
             * RIFF chunks are padded to even size.
             */
            streamoff padded_size =
                static_cast<streamoff>(
                    chunk_size +
                    (chunk_size & 1U));


            file.seekg(
                chunk_data_position +
                padded_size);


            if (fmt_found &&
                data_found)
            {
                break;
            }
        }


        if (!fmt_found ||
            !data_found)
        {
            cout << "[AUDIO] Missing fmt/data chunk."
                 << endl;

            return false;
        }


        /*
         * Only support the format generated by sdr_app.
         */
        if (audio_format != 1)
        {
            cout << "[AUDIO] Unsupported WAVE format."
                 << endl;

            cout << "[AUDIO] PCM format required."
                 << endl;

            return false;
        }


        if (channels != 1)
        {
            cout << "[AUDIO] SINAD requires Mono WAVE."
                 << endl;

            return false;
        }


        if (bits_per_sample != 16)
        {
            cout << "[AUDIO] SINAD requires PCM 16-bit WAVE."
                 << endl;

            return false;
        }


        if (sample_rate == 0)
        {
            cout << "[AUDIO] Invalid sample rate."
                 << endl;

            return false;
        }


        if (data_size < 2 ||
            (data_size & 1U) != 0)
        {
            cout << "[AUDIO] Invalid PCM data size."
                 << endl;

            return false;
        }


        /*
         * Read PCM data.
         */
        vector<uint8_t> raw_data(
            data_size);


        file.clear();

        file.seekg(
            data_position);


        file.read(
            reinterpret_cast<char *>(
                raw_data.data()),
            static_cast<streamsize>(
                data_size));


        if (file.gcount() !=
            static_cast<streamsize>(
                data_size))
        {
            cout << "[AUDIO] Failed to read PCM data."
                 << endl;

            return false;
        }


        size_t sample_count =
            data_size / 2;


        wave.samples.resize(
            sample_count);


        /*
         * PCM16 WAVE is little-endian.
         */
        for (size_t i = 0;
             i < sample_count;
             ++i)
        {
            uint16_t u =
                static_cast<uint16_t>(
                    raw_data[2 * i]) |

                (static_cast<uint16_t>(
                    raw_data[2 * i + 1])
                 << 8);


            int32_t signed_value =
                (u & 0x8000U)
                    ? static_cast<int32_t>(u) - 65536
                    : static_cast<int32_t>(u);


            wave.samples[i] =
                static_cast<int16_t>(
                    signed_value);
        }


        wave.sample_rate =
            sample_rate;


        return true;
    }


    /*
     * ---------------------------------------------------------
     * Goertzel power
     *
     * Used only to find the exact fundamental frequency
     * near the expected 1 kHz test tone.
     * ---------------------------------------------------------
     */
    double sinad_goertzel_power(
        const vector<int16_t> &samples,
        size_t start_index,
        double mean,
        double sample_rate,
        double frequency)
    {
        const double omega =
            2.0 *
            SINAD_PI *
            frequency /
            sample_rate;


        const double coeff =
            2.0 *
            cos(omega);


        double s_prev  = 0.0;
        double s_prev2 = 0.0;


        for (size_t i = start_index;
             i < samples.size();
             ++i)
        {
            const double x =
                static_cast<double>(
                    samples[i]) -
                mean;


            const double s =
                x +
                coeff * s_prev -
                s_prev2;


            s_prev2 = s_prev;
            s_prev  = s;
        }


        return
            s_prev2 * s_prev2 +
            s_prev  * s_prev -
            coeff   * s_prev * s_prev2;
    }


    /*
     * ---------------------------------------------------------
     * Solve 3x3 linear system
     *
     * Augmented matrix:
     *
     *      matrix[3][4]
     * ---------------------------------------------------------
     */
    bool sinad_solve_3x3(
        double matrix[3][4],
        double result[3])
    {
        for (int col = 0;
             col < 3;
             ++col)
        {
            int pivot =
                col;


            double pivot_value =
                fabs(
                    matrix[col][col]);


            for (int row = col + 1;
                 row < 3;
                 ++row)
            {
                double value =
                    fabs(
                        matrix[row][col]);


                if (value > pivot_value)
                {
                    pivot_value = value;
                    pivot = row;
                }
            }


            if (pivot_value < 1e-12)
                return false;


            if (pivot != col)
            {
                for (int k = col;
                     k < 4;
                     ++k)
                {
                    swap(
                        matrix[col][k],
                        matrix[pivot][k]);
                }
            }


            const double divisor =
                matrix[col][col];


            for (int k = col;
                 k < 4;
                 ++k)
            {
                matrix[col][k] /=
                    divisor;
            }


            for (int row = 0;
                 row < 3;
                 ++row)
            {
                if (row == col)
                    continue;


                const double factor =
                    matrix[row][col];


                for (int k = col;
                     k < 4;
                     ++k)
                {
                    matrix[row][k] -=
                        factor *
                        matrix[col][k];
                }
            }
        }


        result[0] = matrix[0][3];
        result[1] = matrix[1][3];
        result[2] = matrix[2][3];


        return true;
    }


    /*
     * ---------------------------------------------------------
     * Least-Squares Fundamental Fit
     *
     * x[n] =
     *
     *      DC
     *    + A*cos(w*n)
     *    + B*sin(w*n)
     *
     * ---------------------------------------------------------
     */
    bool sinad_fit_fundamental(
        const vector<int16_t> &samples,
        size_t start_index,
        double sample_rate,
        double frequency,
        double &signal_rms,
        double &residual_rms,
        double &total_rms,
        double &sinad_db)
    {
        const size_t count =
            samples.size() -
            start_index;


        if (count < 16)
            return false;


        const double omega =
            2.0 *
            SINAD_PI *
            frequency /
            sample_rate;


        const double cos_step =
            cos(omega);

        const double sin_step =
            sin(omega);


        double cos_n = 1.0;
        double sin_n = 0.0;


        double sum_x  = 0.0;

        double sum_c  = 0.0;
        double sum_s  = 0.0;

        double sum_cc = 0.0;
        double sum_ss = 0.0;
        double sum_cs = 0.0;

        double sum_xc = 0.0;
        double sum_xs = 0.0;


        for (size_t i = start_index;
             i < samples.size();
             ++i)
        {
            const double x =
                static_cast<double>(
                    samples[i]);


            sum_x += x;

            sum_c += cos_n;
            sum_s += sin_n;

            sum_cc +=
                cos_n * cos_n;

            sum_ss +=
                sin_n * sin_n;

            sum_cs +=
                cos_n * sin_n;

            sum_xc +=
                x * cos_n;

            sum_xs +=
                x * sin_n;


            const double next_cos =
                cos_n * cos_step -
                sin_n * sin_step;


            const double next_sin =
                sin_n * cos_step +
                cos_n * sin_step;


            cos_n = next_cos;
            sin_n = next_sin;
        }


        double matrix[3][4] =
        {
            {
                static_cast<double>(count),
                sum_c,
                sum_s,
                sum_x
            },

            {
                sum_c,
                sum_cc,
                sum_cs,
                sum_xc
            },

            {
                sum_s,
                sum_cs,
                sum_ss,
                sum_xs
            }
        };


        double coefficients[3];


        if (!sinad_solve_3x3(
                matrix,
                coefficients))
        {
            return false;
        }


        const double dc =
            coefficients[0];

        const double a =
            coefficients[1];

        const double b =
            coefficients[2];


        const double mean =
            sum_x /
            static_cast<double>(count);


        cos_n = 1.0;
        sin_n = 0.0;


        double signal_energy   = 0.0;
        double residual_energy = 0.0;
        double total_energy    = 0.0;


        for (size_t i = start_index;
             i < samples.size();
             ++i)
        {
            const double x =
                static_cast<double>(
                    samples[i]);


            const double tone =
                a * cos_n +
                b * sin_n;


            const double fitted =
                dc +
                tone;


            const double residual =
                x -
                fitted;


            const double total_ac =
                x -
                mean;


            signal_energy +=
                tone * tone;


            residual_energy +=
                residual * residual;


            total_energy +=
                total_ac * total_ac;


            const double next_cos =
                cos_n * cos_step -
                sin_n * sin_step;


            const double next_sin =
                sin_n * cos_step +
                cos_n * sin_step;


            cos_n = next_cos;
            sin_n = next_sin;
        }


        if (total_energy <= 0.0 ||
            residual_energy <= 0.0)
        {
            return false;
        }


        signal_rms =
            sqrt(
                signal_energy /
                static_cast<double>(count));


        residual_rms =
            sqrt(
                residual_energy /
                static_cast<double>(count));


        total_rms =
            sqrt(
                total_energy /
                static_cast<double>(count));


        sinad_db =
            10.0 *
            log10(
                total_energy /
                residual_energy);


        return true;
    }

}


/*
 * =============================================================
 * Public SINAD API
 * =============================================================
 *
 * Usage:
 *
 *      sdr_app audio sinad /tmp/test3.wave
 */
bool audio_measure_sinad(
    const char *filename)
{
    if (filename == nullptr ||
        filename[0] == '\0')
    {
        cout << "[AUDIO] Invalid WAVE filename."
             << endl;

        return false;
    }


    SinadWaveData wave;


    if (!sinad_load_wave(
            filename,
            wave))
    {
        return false;
    }


    /*
     * Ignore the first 10 ms.
     *
     * This removes startup transient from the receiver /
     * demodulator / capture path.
     */
    const size_t startup_skip =
        static_cast<size_t>(
            wave.sample_rate /
            100);


    if (wave.samples.size() <=
        startup_skip + 4096)
    {
        cout << "[AUDIO] WAVE file is too short for SINAD measurement."
             << endl;

        return false;
    }


    const size_t start_index =
        startup_skip;


    const size_t sample_count =
        wave.samples.size() -
        start_index;


    /*
     * Calculate mean for tone search.
     */
    double mean = 0.0;


    for (size_t i = start_index;
         i < wave.samples.size();
         ++i)
    {
        mean +=
            static_cast<double>(
                wave.samples[i]);
    }


    mean /=
        static_cast<double>(
            sample_count);


    /*
     * ---------------------------------------------------------
     * Fundamental search
     *
     * Expected modulation tone:
     *
     *      1 kHz
     *
     * Coarse search:
     *
     *      900 ... 1100 Hz
     *
     * Fine search:
     *
     *      +/- 1 Hz around coarse peak
     *      resolution = 0.02 Hz
     * ---------------------------------------------------------
     */

    double best_frequency =
        1000.0;


    double best_power =
        -numeric_limits<double>::infinity();


    for (int f = 900;
         f <= 1100;
         ++f)
    {
        const double frequency =
            static_cast<double>(f);


        const double power =
            sinad_goertzel_power(
                wave.samples,
                start_index,
                mean,
                static_cast<double>(
                    wave.sample_rate),
                frequency);


        if (power > best_power)
        {
            best_power =
                power;

            best_frequency =
                frequency;
        }
    }


    const double coarse_frequency =
        best_frequency;


    best_power =
        -numeric_limits<double>::infinity();


    for (int step = -50;
         step <= 50;
         ++step)
    {
        const double frequency =
            coarse_frequency +
            static_cast<double>(step) *
            0.02;


        const double power =
            sinad_goertzel_power(
                wave.samples,
                start_index,
                mean,
                static_cast<double>(
                    wave.sample_rate),
                frequency);


        if (power > best_power)
        {
            best_power =
                power;

            best_frequency =
                frequency;
        }
    }


    /*
     * Fit the detected fundamental and calculate SINAD.
     */
    double signal_rms   = 0.0;
    double residual_rms = 0.0;
    double total_rms    = 0.0;
    double sinad_db     = 0.0;


    if (!sinad_fit_fundamental(
            wave.samples,
            start_index,
            static_cast<double>(
                wave.sample_rate),
            best_frequency,
            signal_rms,
            residual_rms,
            total_rms,
            sinad_db))
    {
        cout << "[AUDIO] SINAD calculation failed."
             << endl;

        return false;
    }


    /*
     * ---------------------------------------------------------
     * Report
     * ---------------------------------------------------------
     */

    cout << endl;

    cout << "[AUDIO] SINAD Measurement"
         << endl;

    cout << endl;


    cout << "File          : "
         << filename
         << endl;


    cout << "Sample Rate   : "
         << wave.sample_rate
         << " Hz"
         << endl;


    cout << "Format        : PCM 16-bit Mono"
         << endl;


    cout << "Startup Skip  : "
         << fixed
         << setprecision(2)
         << (
                1000.0 *
                static_cast<double>(
                    startup_skip) /
                static_cast<double>(
                    wave.sample_rate)
            )
         << " ms"
         << endl;


    cout << "Samples Used  : "
         << sample_count
         << endl;


    cout << "Tone          : "
         << fixed
         << setprecision(3)
         << best_frequency
         << " Hz"
         << endl;


    cout << endl;


    cout << "Signal RMS    : "
         << fixed
         << setprecision(3)
         << signal_rms
         << endl;


    cout << "N+D RMS       : "
         << fixed
         << setprecision(3)
         << residual_rms
         << endl;


    cout << "Total RMS     : "
         << fixed
         << setprecision(3)
         << total_rms
         << endl;


    cout << endl;


    cout << "SINAD         : "
         << fixed
         << setprecision(2)
         << sinad_db
         << " dB"
         << endl;


    return true;
}

