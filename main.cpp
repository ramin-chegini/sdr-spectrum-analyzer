#include "ad9361.h"

#include <cstdio>
#include <cstdint>

int main()
{
    std::printf("====================================\n");
    std::printf("      SDR App - AD9361 Control\n");
    std::printf("====================================\n");

    AD9361Controller ad9361;

    if (!ad9361.init()) {
        std::fprintf(stderr,
                     "ERROR: AD9361 initialization failed\n");
        return 1;
    }

    std::printf("\nAD9361 initialization: OK\n");


    // ========================================================
    // 1. RX FREQUENCY
    // ========================================================

    uint64_t rxFreq = 0;

    if (ad9361.getRxFrequency(rxFreq)) {
        std::printf(
            "\nRX LO before = %llu Hz\n",
            (unsigned long long)rxFreq
        );
    }

    const uint64_t newRxFreq = 2450000000ULL;

    std::printf(
        "Setting RX LO = %llu Hz\n",
        (unsigned long long)newRxFreq
    );

    if (!ad9361.setRxFrequency(newRxFreq)) {
        std::fprintf(stderr,
                     "ERROR: failed to set RX frequency\n");
        return 1;
    }

    if (ad9361.getRxFrequency(rxFreq)) {
        std::printf(
            "RX LO after  = %llu Hz\n",
            (unsigned long long)rxFreq
        );
    }


    // ========================================================
    // 2. TX FREQUENCY
    // ========================================================

    uint64_t txFreq = 0;

    if (ad9361.getTxFrequency(txFreq)) {
        std::printf(
            "\nTX LO before = %llu Hz\n",
            (unsigned long long)txFreq
        );
    }

    const uint64_t newTxFreq = 2500000000ULL;

    std::printf(
        "Setting TX LO = %llu Hz\n",
        (unsigned long long)newTxFreq
    );

    if (!ad9361.setTxFrequency(newTxFreq)) {
        std::fprintf(stderr,
                     "ERROR: failed to set TX frequency\n");
        return 1;
    }

    if (ad9361.getTxFrequency(txFreq)) {
        std::printf(
            "TX LO after  = %llu Hz\n",
            (unsigned long long)txFreq
        );
    }


    // ========================================================
    // 3. RX1 GAIN
    // ========================================================

    double rx1Gain = 0.0;

    if (ad9361.getRxGain(1, rx1Gain)) {
        std::printf(
            "\nRX1 gain before = %.2f dB\n",
            rx1Gain
        );
    }

    std::printf("Setting RX1 gain mode = manual\n");

    if (!ad9361.setRxGainMode(1, "manual")) {
        std::fprintf(stderr,
                     "ERROR: failed to set RX1 gain mode\n");
        return 1;
    }

    const double newRx1Gain = 30.0;

    std::printf(
        "Setting RX1 gain = %.2f dB\n",
        newRx1Gain
    );

    if (!ad9361.setRxGain(1, newRx1Gain)) {
        std::fprintf(stderr,
                     "ERROR: failed to set RX1 gain\n");
        return 1;
    }

    if (ad9361.getRxGain(1, rx1Gain)) {
        std::printf(
            "RX1 gain after  = %.2f dB\n",
            rx1Gain
        );
    }


    // ========================================================
    // 4. RX2 GAIN
    // ========================================================

    double rx2Gain = 0.0;

    if (ad9361.getRxGain(2, rx2Gain)) {
        std::printf(
            "\nRX2 gain before = %.2f dB\n",
            rx2Gain
        );
    }

    std::printf("Setting RX2 gain mode = manual\n");

    if (!ad9361.setRxGainMode(2, "manual")) {
        std::fprintf(stderr,
                     "ERROR: failed to set RX2 gain mode\n");
        return 1;
    }

    const double newRx2Gain = 25.0;

    std::printf(
        "Setting RX2 gain = %.2f dB\n",
        newRx2Gain
    );

    if (!ad9361.setRxGain(2, newRx2Gain)) {
        std::fprintf(stderr,
                     "ERROR: failed to set RX2 gain\n");
        return 1;
    }

    if (ad9361.getRxGain(2, rx2Gain)) {
        std::printf(
            "RX2 gain after  = %.2f dB\n",
            rx2Gain
        );
    }


    // ========================================================
    // Final status
    // ========================================================

    std::printf("\n====================================\n");
    std::printf("       AD9361 control test done\n");
    std::printf("====================================\n");

    return 0;
}