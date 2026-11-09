SUMMARY = "SDR control and capture application with startup service"
SECTION = "PETALINUX/apps"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://src \
    file://include \
    file://sdr-app \
"

S = "${WORKDIR}"
B = "${S}"
DEPENDS = "libiio"

require sdr-app-sources.inc

inherit update-rc.d

INITSCRIPT_NAME = "sdr-app"
INITSCRIPT_PARAMS = "defaults 99"

do_compile() {
    ${CXX} ${CXXFLAGS} -std=c++17 -pthread -I${S}/include \
        ${SDR_APP_SOURCES} ${LDFLAGS} -liio -o sdr_app
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${WORKDIR}/sdr_app \
        ${D}${bindir}/sdr_app

    install -d ${D}${sysconfdir}/init.d

    install -m 0755 ${WORKDIR}/sdr-app \
        ${D}${sysconfdir}/init.d/sdr-app
}

FILES_${PN} += " \
    ${bindir}/sdr_app \
    ${sysconfdir}/init.d/sdr-app \
"
