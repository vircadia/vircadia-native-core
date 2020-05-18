#!/bin/bash -l
LOCAL_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
prefix=${1:-""}
declare -a packages_systemd=("assignment-client" "domain-server" "ice-server")
cd ./build

for package_name in "${packages_systemd[@]}"
do
    SOURCE_DESTINATION_LIST="${WORKSPACE}/build/${package_name}/${package_name}=/usr/share/hifi/${package_name}/ "
    if [ "$package_name" == "domain-server" ]; then
        DESCRIPTION="High Fidelity Domain server."
        SOURCE_DESTINATION_LIST+="${WORKSPACE}/build/${package_name}/resources/=/usr/share/hifi/${package_name}/resources "
        SOURCE_DESTINATION_LIST+="${WORKSPACE}/build/ext/makefiles/quazip/project/build/libquazip5.so.1.0.0=/usr/share/hifi/${package_name}/libquazip5.so.1"
    elif [ "$package_name" == "assignment-client" ]; then
        DESCRIPTION="High Fidelity Assignment clients. Services target a local domain server. Different assignment clients are managed independently with systemd."
        SOURCE_DESTINATION_LIST+="${WORKSPACE}/build/${package_name}/plugins/=/usr/share/hifi/${package_name}/plugins "
        SOURCE_DESTINATION_LIST+="${WORKSPACE}/build/ext/makefiles/quazip/project/build/libquazip5.so.1.0.0=/usr/share/hifi/${package_name}/libquazip5.so.1 "
        SOURCE_DESTINATION_LIST+="${WORKSPACE}/build/${package_name}/oven=/usr/share/hifi/${package_name}/oven"
    elif [ "$package_name" == "ice-server" ]; then
        DESCRIPTION="High Fidelity ICE server."
    fi

    fpm -s dir -t deb -n hifi-${prefix}${package_name} -v ${VERSION} -d hifiqt5.12.3 --vendor "High Fidelity Inc" -m "<ops@highfidelity.com>" \
        --url "https://highfidelity.com" --license "Apache License 2.0" --description "${DESCRIPTION}" -d libgomp1 -d libtbb2 -d libgl1-mesa-glx -d libnss3 \
        -d libxi6 -d libxcursor1 -d libxcomposite1 -d libasound2 -d libxtst6 -d libxslt1.1 --template-scripts --template-value "service"="hifi-${package_name}" \
        --deb-systemd ${LOCAL_PATH}/hifi-${package_name} --before-install ${LOCAL_PATH}/${package_name}-before-install.sh \
        --after-install ${LOCAL_PATH}/after-install.sh --before-remove ${LOCAL_PATH}/before-remove.sh \
         ${SOURCE_DESTINATION_LIST}

    dpkg-sig --sign builder -k 15FF1AAE -g '--no-tty --passphrase "${MASKED_DEB_REPO_SIGN_PASSPHRASE}"' ./hifi-${prefix}${package_name}_${VERSION}_amd64.deb

    deb-s3 upload ./hifi-${prefix}${package_name}_${VERSION}_amd64.deb --bucket debian.highfidelity.com \
           --sign=15FF1AAE --gpg-options='--no-tty --passphrase "${MASKED_DEB_REPO_SIGN_PASSPHRASE}"' \
           --origin hifi --suite stable -p

done
