
set -x

if [[ $TRAVIS_OS_NAME == "linux" ]]; then

    source travis-install-linux.sh

elif [[ $TRAVIS_OS_NAME == "osx" ]]; then

    source travis-install-osx.sh

fi


set +x
