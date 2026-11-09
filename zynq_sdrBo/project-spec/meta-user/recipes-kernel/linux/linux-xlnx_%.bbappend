FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://ad9144_fmc_ebz_ad9516.stp \
    file://ad9467_intbypass_ad9517.stp \
    file://ad9517.stp \
    file://ad9517_fmcomms6.stp \
    file://ad9517_fmcomms6_300M.stp \
    file://pzsdr-fmc-ad9517.stp \
    file://adau1761.bin \
    file://imageon_edid.bin \
    file://Mykonos_M3.bin \
    file://Navassa_EvaluationFw.bin \
    file://Navassa_Stream.bin \
    file://TaliseRxArmFirmware.bin \
    file://TaliseStream.bin \
    file://TaliseTDDArmFirmware.bin \
    file://TaliseTxArmFirmware.bin \
    file://RxGainTable.csv \
    file://ORxGainTable.csv \
    file://TxAttenTable.csv \
    file://Navassa_CMOS_profile.json \
    file://Navassa_LVDS_profile.json \
    file://defconfig \
"

do_configure_append() {
    install -d ${S}/firmware
    for f in ${WORKDIR}/*.stp ${WORKDIR}/*.bin ${WORKDIR}/*.csv ${WORKDIR}/*.json; do
        [ -e "$f" ] && install -m 0644 "$f" ${S}/firmware/
    done
}
