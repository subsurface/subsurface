FROM ubuntu:26.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  autoconf \
  automake \
  bzip2 \
  ca-certificates \
  cmake \
  g++ \
  git \
  libbluetooth-dev \
  libcrypto++-dev \
  libcurl4-gnutls-dev \
  libgit2-dev \
  libmtp-dev \
  libraw-dev \
  libsqlite3-dev \
  libssh2-1-dev \
  libssl-dev \
  libtool \
  libusb-1.0-0-dev \
  libxml2-dev \
  libxslt1-dev \
  libzip-dev \
  locales \
  make \
  mdbtools-dev \
  ninja-build \
  pkg-config \
  qmake6 \
  qml6-module-qtlocation \
  qml6-module-qtpositioning \
  qml6-module-qtquick \
  qt6-5compat-dev \
  qt6-base-private-dev \
  qt6-connectivity-dev \
  qt6-declarative-dev \
  qt6-declarative-private-dev \
  qt6-location-dev \
  qt6-positioning-dev \
  qt6-shadertools-dev \
  qt6-svg-dev \
  qt6-tools-dev \
  qt6-tools-dev-tools \
  libqt6qml6 \
  libqt6quick6 \
  libqt6svg6 \
  && locale-gen C.UTF-8 \
  && rm -rf /var/lib/apt/lists/*

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

WORKDIR /src

COPY . /src/subsurface

RUN ./subsurface/scripts/build.sh \
  -desktop \
  -build-with-qt6 \
  -release

# Stage only the application runtime files.
# Remove debug symbols and development artifacts.
RUN rm -rf /src/runtime-root && \
  mkdir -p /src/runtime-root && \
  cp -a /src/install-root/bin /src/runtime-root/ && \
  cp -a /src/install-root/lib /src/runtime-root/ && \
  cp -a /src/install-root/share /src/runtime-root/ && \
  cp -a /src/install-root/usr /src/runtime-root/ && \
  find /src/runtime-root/bin \
  -type f -name '*.debug' -delete && \
  find /src/runtime-root/lib \
  -type f \
  \( -name '*.a' -o -name '*.la' -o -name '*.pc' \) \
  -delete && \
  rm -rf \
  /src/runtime-root/lib/pkgconfig \
  /src/runtime-root/share/man


FROM ubuntu:26.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV HOME=/home/subsurface
ENV XDG_CONFIG_HOME=/home/subsurface/.config
ENV XDG_DATA_HOME=/home/subsurface/.local/share
ENV DISPLAY=:99
ENV QT_QUICK_BACKEND=software
ENV SUBSURFACE_PREFIX=/opt/subsurface
ENV LD_LIBRARY_PATH=/opt/subsurface/lib:/opt/subsurface/usr/lib

# Install the application runtime and graphical session.
RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  ca-certificates \
  libgit2-1.9 \
  libmtp9 \
  libqt6bluetooth6 \
  libqt6core5compat6 \
  libqt6core6t64 \
  libqt6gui6 \
  libqt6network6 \
  libqt6opengl6 \
  libqt6positioning6 \
  libqt6printsupport6 \
  libqt6qml6 \
  libqt6qmlmeta6 \
  libqt6qmlmodels6 \
  libqt6qmlworkerscript6 \
  libqt6quick6 \
  libqt6quickwidgets6 \
  libqt6svg6 \
  libqt6widgets6 \
  libraw23 \
  libsqlite3-0 \
  libusb-1.0-0 \
  libxml2-16 \
  libxslt1.1 \
  libzip5 \
  locales \
  novnc \
  openbox \
  python3 \
  python3-websockify \
  python3-requests \
  qml6-module-qtlocation \
  qml6-module-qtpositioning \
  qml6-module-qtquick \
  x11-utils \
  x11vnc \
  xvfb \
  && locale-gen C.UTF-8 \
  && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/runtime-root/ ${SUBSURFACE_PREFIX}/

RUN useradd \
  --create-home \
  --uid 10001 \
  --shell /bin/bash \
  subsurface \
  && mkdir -p \
  /tmp/.X11-unix \
  /data \
  /home/subsurface/.config \
  /home/subsurface/.local/share \
  && chmod 1777 /tmp/.X11-unix \
  && chown -R subsurface:subsurface \
  /home/subsurface \
  /data \
  && chmod 755 ${SUBSURFACE_PREFIX}/bin/subsurface

COPY packaging/docker/entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod 755 /usr/local/bin/entrypoint.sh

VOLUME ["/home/subsurface", "/data"]

EXPOSE 6080

USER subsurface

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
