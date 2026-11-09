SUMMARY = "USB RNDIS startup service"
SECTION = "PETALINUX/apps"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://rndis"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "rndis"
INITSCRIPT_PARAMS = "defaults 99"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/rndis \
        ${D}${sysconfdir}/init.d/rndis
}

FILES_${PN} += "${sysconfdir}/init.d/rndis"
