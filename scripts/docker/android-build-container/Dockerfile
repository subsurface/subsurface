FROM ubuntu:24.04 AS base

# set the basic information and paths that everything else is based on
ENV NDK_VERSION=27.2.12479018
ENV SDK_LEVEL=35
ENV SDK_VERSION=35.0.0
ENV ANDROID_PLATFORM=28
ENV ANDROID_BUILD_ABI=arm64-v8a
ENV QT_VERSION=6.10.3
ENV BUILDROOT=/android
ENV ANDROID_SDK_ROOT=/opt/android-sdk

RUN apt-get update  && \
	apt-get full-upgrade -y && \
	apt-get install -y \
	unzip \
	git \
	cmake \
	autoconf \
	libtool-bin \
	libegl-dev \
	libxkbcommon-dev \
	openjdk-17-jdk \
	wget \
	pkg-config \
	ninja-build \
	python3-pip \
	python3-venv \
	p7zip-full && \
	apt-get clean

RUN mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools ${BUILDROOT}/src && \
	wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O /tmp/cmdline-tools.zip && \
	unzip -q /tmp/cmdline-tools.zip -d /tmp/cmdline-tools-tmp && \
	mv /tmp/cmdline-tools-tmp/cmdline-tools ${ANDROID_SDK_ROOT}/cmdline-tools/latest && \
	rm -f /tmp/cmdline-tools.zip && \
	SDKMANAGER="${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" && \
	( yes | ${SDKMANAGER} --licenses > /dev/null 2>&1 || true ) && \
	${SDKMANAGER} "platforms;android-${SDK_LEVEL}" "build-tools;${SDK_VERSION}" "platform-tools" "ndk;${NDK_VERSION}"

FROM base AS lib-base

WORKDIR ${BUILDROOT}

ADD android-native-libs.sh get-dep-lib.sh ${BUILDROOT}/src

# build the native libraries that we need
RUN bash -x ${BUILDROOT}/src/android-native-libs.sh 2>&1 | tee ${BUILDROOT}/native-libs.log

FROM base AS final
WORKDIR ${BUILDROOT}

COPY --from=lib-base ${BUILDROOT}/src/install-root-${ANDROID_BUILD_ABI} ${BUILDROOT}/src/install-root-${ANDROID_BUILD_ABI}/

# install Qt
RUN python3 -m venv ${BUILDROOT}/.v && ${BUILDROOT}/.v/bin/pip3 install aqtinstall && \
	OS_TAG="$(echo ${ANDROID_BUILD_ABI} | tr '-' '_')" && \
	${BUILDROOT}/.v/bin/python3 -m aqt install-qt linux android ${QT_VERSION} android_${OS_TAG} \
		--autodesktop --outputdir ${BUILDROOT}/Qt --modules qtlocation qtpositioning qtconnectivity qt5compat qtshadertools && \
	${BUILDROOT}/.v/bin/python3 -m aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 \
		--autodesktop --outputdir ${BUILDROOT}/Qt --modules qtshadertools --external 7z

ENV PATH=${BUILDROOT}/Qt/${QT_VERSION}/gcc_64/bin:${ANDROID_SDK_ROOT}/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/linux-x86_64/bin:${PATH}
