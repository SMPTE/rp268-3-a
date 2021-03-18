# Logbook for work on the DPX HDR reference code


## Tasks

1. Move each executable into a separate directory with separate build (make and cmake)

2. Create `.gitignore` in the subdirectories to ignore object files

3. Move source and header files to `src` and `include` directories in root

3. Create CMake files for each of the subdirectories


## Make

1. Create sources.mk files for the constituents of each of the three builds

2. Create common make file targets and defines

Are manually generated make files needed? CMake can generate them.


## Logbook

### 2020-08-20

Got `generate_color_test_pattern` to build by changing the tool chain from Clang to gcc-10.

