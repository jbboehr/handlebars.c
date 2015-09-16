
set -e

if [ ! -f $HOME/build/stamp ]; then
    mkdir -p $HOME/build

    wget http://downloads.sourceforge.net/project/check/check/0.9.14/check-0.9.14.tar.gz
    tar xfv check-0.9.14.tar.gz
    cd check-0.9.14
    ./configure --prefix=$HOME/build
    make
    make install
    cd ..
    
    wget http://gnu.mirror.iweb.com/bison/bison-3.0.2.tar.gz
    tar xfv bison-3.0.2.tar.gz
    cd bison-3.0.2
    ./configure --prefix=$HOME/build
    make
    make install
    cd ..
    
    touch $HOME/build/stamp
fi
