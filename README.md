# BRT

Project to deploy the BRT Library (https://github.com/GrupoDiana/BRTLibrary) on OpenFrameworks over Rapsberry Pi

## Compiling

`make -f <Makefile>`

- **Makefile.win**: Cross Compiling in Windows for Raspberry Pi
- **Makefile.rpi**: Native Compiling in Linux for Raspberry Pi

In these Makefiles the variables **OF** and **BRT** points to OpenFrameworks and BRT Library locations respectively.

## Requeriments

### 1. OpenFrameworks

OpenFrameworks can be obtained from: https://openframeworks.cc/download -> [linux arm64]

```
cd /home/pi
wget https://github.com/openframeworks/openFrameworks/releases/download/0.12.1/of_v0.12.1_linuxaarch64_release.tar.gz
tar -xf of_v0.12.1_linuxaarch64_release.tar.gz
rm of_v0.12.1_linuxaarch64_release.tar.gz
mv of_v0.12.1_linuxaarch64_release openFrameworks
cd /home/pi/openFrameworks/scripts/linux/debian
nano install_dependencies.sh -> [delete "libgconf-2-4"]
sudo ./install_dependencies.sh
make Release -C /home/pi/openFrameworks/libs/openFrameworksCompiled/project
```

### 2. BRT Library

The BRT Library can be obtained from: https://github.com/GrupoDiana/BRTLibrary

```
cd /home/pi
git clone https://github.com/GrupoDiana/BRTLibrary.git
mv BRTLibrary brt
```

The BRT Library requires these **additional libraries** (installed on `include/third_party_libraries`):

#### 2.1. Eigen

The Eigen library can be obtained from: https://gitlab.com/libeigen/eigen

```
cd /home/pi/brt/include/third_party_libraries
rm -rf eigen
git clone https://gitlab.com/libeigen/eigen.git
```

#### 2.2. Libmysofa

The Libmysofa library can be obtained from: https://github.com/GrupoDiana/libmysofa

```
cd /home/pi/brt/include/third_party_libraries
rm -rf libmysofa
git clone https://github.com/GrupoDiana/libmysofa.git
cd libmysofa/build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
make -j8 all
cd ..
mkdir include
cp src/hrtf/mysofa.h include
cp build/src/mysofa_export.h include
mkdir lib
mkdir lib/aarch64
cp build/src/libmysofa.a lib/aarch64
```

#### 2.3. Rabidobj

The Rabidobj library can be obtained from: https://github.com/GrupoDiana/rapidobj

```
cd /home/pi/brt/include/third_party_libraries
rm -rf rapidobj
git clone -b AcousticCoefficients https://github.com/GrupoDiana/rapidobj.git
```

#### 2.4. Boost

The Boost library can be installed with apt

```
sudo apt install libboost-all-dev
```