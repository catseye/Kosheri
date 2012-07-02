#!/bin/sh

cd src
echo "Building 'debug' version..."
make clean debug >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
rm -f ERRORS
./assemble --asmfile ../$1 --vmfile out.kvm
gdb --args ./run --vmfile out.kvm
rm -f out.kvm
