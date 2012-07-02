#!/bin/sh

cd src

make clean all >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
echo "Testing default build..."
falderal -b test ../tests/Assembly.markdown ../tests/Term.markdown

make clean all CFLAGS=-DDIRECT_THREADING >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
echo "Testing direct threading build..."
falderal -b test ../tests/Assembly.markdown ../tests/Term.markdown

make clean tool >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
echo "Testing 'tool' build..."
falderal -b test ../tests/Assembly.markdown ../tests/Term.markdown

make clean static >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
echo "Testing 'static' build..."
falderal -b test ../tests/Assembly.markdown ../tests/Term.markdown

echo "Building 'debug' version..."
make clean debug >ERRORS 2>&1
if [ $? != 0 ]; then
	cat ERRORS
	rm -f ERRORS
	exit 1
fi
make clean
rm -f ERRORS
