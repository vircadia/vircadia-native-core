FROM openjdk:8

RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

RUN apt-get update && apt-get -y install \
    curl \
    gnupg \
    software-properties-common \
    unzip \
    -

# --- Versions and Download paths
ENV ANDROID_HOME="/usr/local/android-sdk" \
    ANDROID_NDK_HOME="/usr/local/android-ndk" \
    ANDROID_SDK_HOME="/usr/local/android-sdk-home" \
    ANDROID_VERSION=26 \
    ANDROID_BUILD_TOOLS_VERSION=28.0.3 \
    ANDROID_NDK_VERSION=r18
    
ENV SDK_URL="https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip" \
    NDK_URL="https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip" 

# --- Android SDK
RUN mkdir -p "$ANDROID_HOME" "$ANDROID_SDK_HOME" && \
    cd "$ANDROID_HOME" && \
    curl -s -S -o sdk.zip -L "${SDK_URL}" && \
    unzip sdk.zip && \
    rm sdk.zip && \
    yes | $ANDROID_HOME/tools/bin/sdkmanager --licenses
RUN yes | $ANDROID_HOME/tools/bin/sdkmanager --licenses && yes | $ANDROID_HOME/tools/bin/sdkmanager --update
# Install Android Build Tool and Libraries
RUN $ANDROID_HOME/tools/bin/sdkmanager --update
RUN $ANDROID_HOME/tools/bin/sdkmanager "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" \
    "platforms;android-${ANDROID_VERSION}" \
    "platform-tools"

RUN chmod -R a+w "${ANDROID_HOME}"
RUN chmod -R a+w "${ANDROID_SDK_HOME}"

# --- Android NDK
# download
RUN mkdir /usr/local/android-ndk-tmp && \
    cd /usr/local/android-ndk-tmp && \
    curl -s -S -o ndk.zip -L "${NDK_URL}" && \
    unzip -q ndk.zip && \
    mv ./android-ndk-${ANDROID_NDK_VERSION} ${ANDROID_NDK_HOME} && \
    cd ${ANDROID_NDK_HOME} && \
    rm -rf /usr/local/android-ndk-tmp

ENV PATH ${PATH}:${ANDROID_NDK_HOME}

RUN apt-get -y install \
    g++ \
    gcc \
    -

# --- Gradle
ARG BUILD_UID=1001
RUN useradd -ms /bin/bash -u $BUILD_UID jenkins
USER jenkins
WORKDIR /home/jenkins

# Hifi dependencies
ENV HIFI_BASE="/home/jenkins/hifi_android"
ENV HIFI_ANDROID_PRECOMPILED="$HIFI_BASE/dependencies"    
ENV HIFI_VCPKG_BASE="$HIFI_BASE/vcpkg"

RUN mkdir "$HIFI_BASE" && \
    mkdir "$HIFI_VCPKG_BASE" && \ 
    mkdir "$HIFI_ANDROID_PRECOMPILED"

RUN git clone https://github.com/jherico/hifi.git && \
    cd ~/hifi && \
    git checkout feature/build/gradle-wrapper 


WORKDIR /home/jenkins/hifi

RUN touch .test4 && \ 
    git fetch && git reset origin/feature/build/gradle-wrapper --hard

RUN mkdir build

# Pre-cache the vcpkg managed dependencies
WORKDIR /home/jenkins/hifi/build
RUN python3 ../prebuild.py --build-root `pwd` --android

# Pre-cache the gradle dependencies
WORKDIR /home/jenkins/hifi/android
RUN ./gradlew -m tasks -PHIFI_ANDROID_PRECOMPILED=$HIFI_ANDROID_PRECOMPILED
RUN ./gradlew extractDependencies -PHIFI_ANDROID_PRECOMPILED=$HIFI_ANDROID_PRECOMPILED 

