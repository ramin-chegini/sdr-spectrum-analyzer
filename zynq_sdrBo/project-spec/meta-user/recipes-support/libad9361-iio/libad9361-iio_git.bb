SUMMARY = "AD9361 IIO library"
LICENSE = "LGPLv2.1+"
LIC_FILES_CHKSUM = "file://LICENSE;md5=40d2542b8c43a3ec2b7f5da31a697b88"


DEPENDS = "libiio"

SRCREV = "be7eb026436f6b68eb718a01cdd249acd85b05c5"
SRC_URI = "git://github.com/analogdevicesinc/libad9361-iio;protocol=https;branch=2019_R2"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

FILES_${PN} += "${libdir}/*.so.*"
FILES_${PN}-dev += "${libdir}/*.so ${includedir}/*"
