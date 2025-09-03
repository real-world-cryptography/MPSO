# MPSO
An efficient C++ repo implementing the MPSO protocols in our paper "Multi-Party Private Set Operations from Predicative Zero-Sharing" https://eprint.iacr.org/2025/640
## Installations

### Required libraries

This code is built on top of [`Vole-PSI`](https://github.com/Visa-Research/volepsi.git) and requires additional library dependencies including [`OpenSSL`](https://www.openssl.org) and [`OpenMP`](https://www.openmp.org). Our code has been tested on Ubuntu 22.04, with g++ 11.4.0. 

To install the required libraries and tools, run the following shell commands:

```bash
# Update the package list
sudo apt-get update

# Install essential development tools and libraries
sudo apt-get install build-essential libssl-dev libomp-dev libtool
sudo apt install gcc g++ git make cmake
```

### Building Vole-PSI

1. Clone the repository:

```bash
#in MPSO
git clone https://github.com/Visa-Research/volepsi.git
cd volepsi
```

2. Compile and install Vole-PSI:

```bash
python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON -DVOLE_PSI_ENABLE_GMW=ON -DVOLE_PSI_ENABLE_CPSI=OFF -DVOLE_PSI_ENABLE_OPPRF=OFF
python3 build.py --install=../libvolepsi
cp out/build/linux/volePSI/config.h ../libvolepsi/include/volePSI/
```
### Linking Vole-PSI

Vole-PSI can be linked via cmake in our CMakeLists.txt, just change the path to libvolepsi

```bash
find_package(volePSI REQUIRED HINTS "/path/to/libvolepsi")
```
### Building MPSO

```bash
#download our repo
#in MPSO
mkdir build
cd build
mkdir offline
cmake ..
make
```

## Running the code

### Test for MPSO

```bash
cd build
#in MPSO/shell_for_test
cp * ../build/
chmod +x *.sh
#run union test script
#in MPSO/build
./mpsoRunControl.sh
```
