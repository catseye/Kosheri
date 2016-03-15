#!/bin/sh

error() {
    if [ $0 != 0 ]; then
        cat ERRORS
        rm -f ERRORS
        exit 1
    fi
}

cd src

make clean all >ERRORS 2>&1 || error $?

echo "Testing default build..."
TESTS="../tests/Assembly.markdown ../tests/Term.markdown"
falderal -b test $TESTS >ERRORS 2>&1 || error $?

make clean all CFLAGS=-DDIRECT_THREADING >ERRORS 2>&1 || error $?

echo "Testing direct threading build..."
falderal -b test $TESTS >ERRORS 2>&1 || error $?

make clean tool >ERRORS 2>&1 || error $?

echo "Testing 'tool' build..."
falderal -b test $TESTS >ERRORS 2>&1 || error $?

make clean static >ERRORS 2>&1 || error $?

echo "Testing 'static' build..."
falderal -b test $TESTS >ERRORS 2>&1 || error $?

echo "Building 'debug' version..."
make clean debug >ERRORS 2>&1 || error $?

make clean
rm -f ERRORS
