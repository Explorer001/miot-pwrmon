# MIOT-Lab Power Monitor

This project provides a power monitoring tool for the MIOT-Lab.

## Building

To build the application use make. It is advised to use the docker
container provided by RIOT OS. This saves the trouble of installing
the esp32 toolchain.

```
# In case of a fresh clone use
git clone --recursive git@github.com:Explorer001/miot-pwrmon.git
cd miot-pwrmon

# To update a existing repo use
git submodule --init --recursive

# Build using docker
# DOCKER="sudo docker" can be omitted if the user has permissions to access the docker daemon
BUILD_IN_DOCKER=1 DOCKER="sudo docker" make
```
