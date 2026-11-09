# Supplied libiio dependency

The public header and three identical ARM shared-library files were copied from
the current `sdr_app` project. The original bundled source archive identifies
libiio **0.21** in its CMake configuration. `libiio.so` provides the linker name
used by the Makefile; the versioned names support the existing target setup.

Upstream: <https://github.com/analogdevicesinc/libiio>

License texts `COPYING.txt`, `COPYING_GPL.txt` and `LICENSE-libini.txt` are retained
from that supplied archive. Refer to the original notices and the LGPL terms
for libiio. The public header also retains its original LGPL 2.1-or-later notice.

The original archive and a second source tree are not needed to build this
application against these supplied files. Rebuilding libiio itself requires
its source and the target toolchain; this repository does not rebuild it.
