
set -x



if [[ $TRAVIS_OS_NAME == "linux" ]]; then

    source test/travis/travis-install-linux.sh

elif [[ $TRAVIS_OS_NAME == "osx" ]]; then

    source test/travis/travis-install-osx.sh

fi


set +x
