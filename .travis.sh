
set -e

wget http://downloads.sourceforge.net/project/check/check/0.9.14/check-0.9.14.tar.gz
tar xfv check-0.9.14.tar.gz
cd check-0.9.14
./configure --prefix=$PREFIX
make
make install
cd ..
rm check-0.9.14.tar.gz
