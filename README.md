# SMPTE RP 268-3-a

Element A of RP 268-3 Reference Materials for DPX V2.0HDR Implementations

_This repository is public._

Please consult [CONTRIBUTING.md](./CONTRIBUTING.md), [CONFIDENTIALITY.md](./CONFIDENTIALITY.md) and [PATENTS.md](./PATENTS.md) for important notices.

# License

Unless specified otherwise, the contents of this element are licensed as indicated at [LICENSE.md](./LICENSE.md).

# Reporting issues

Please report issues at https://github.com/SMPTE/rp268-3-a/issues or at mailto:31fs-chair@smpte.org.

# How to build

You need to install [CMake](http://cmake.org) in order to build the reference code.

## Linux
Go to the directory corresponding to the example you would like to build (convert_descriptor, generate_color_test_pattern, or dump_dpx). Then:

```
cmake .
make
```

## Windows
Building under Windows requires an installation of Microsoft Visual Studio. Go to the directory corresponding to the example you would like to build (convert_descriptor, generate_color_test_pattern, or dump_dpx). Then:

```
cmake .
```

CMake will create a .sln file that can be loaded using the MS Visual Studio IDE.

# Brief description of examples

## convert_descriptor

The convert_descriptor example takes in a DPX file with R, G, B, and optionally A components and rearranges the component and image element structure according to the setting on the command line.

Usage:

```
convert_descriptor input.dpx output.dpx <neworder> <planar>
```

* input.dpx - Name of input DPX file
* output.dpx - Name of output DPX file
* neworder - (e.g., BGR, BGRA, ARGB, etc.) order of components in output file. For an interleaved file, only some orders are supported. A is optional.
* planar - set to 0 for interleaved components within a single image element; set to 1 for planes of components, each in its own image element
 
  
## dump_dpx
  
The dump_dpx example takes in a DPX file, prints the header information, and dumps all of the samples as either text or raw binary files.

Usage:
```
dump_dpx <dpxfile> (-rawout <out_rawfile_base> (-plane_order <plane_order>) (-bit_depth_conv <bit_depth_conv>) (-dump_full_range))
```

* dpxfile - Name of DPX file to dump
* out_rawfile_base - Output filename base name (if specified, then the tool outputs to files using this base name rather than as text). The output extension of each file will be .#.ext, where # is the image element number and ext is an extension indicating the component type (y,u,v,r,g,b,a, or g0-7).
* bit_depth_conv - If specified, converts the samples to the specified bit depth before dumping.
* -dump_full_range - If this flag is present, the output raw files will be full range. If absent, output raw files will be limited range. Note that float/double output is always scaled from 0-1.0 based on the high & low code values if a bit depth conversion is specified.

A sample in an output file is stored within the smallest of (1, 2, 4, or 8) bytes that fits the sample (or if bit_depth_conv is specified, that fits within bit_depth_conv bits). For example, if there is an 8-bit DPX image element and bit_depth_conv = 12, then the image element is converted from 8 to 12 bits and each 12-bit sample is placed in a 2-byte (16-bit) field in the raw file output (right-justified). The byte order for raw files always matches the byte order of the machine

One way to use dump_dpx to sanity check images would be to specify the flags: -rawout out -bitdepth_conv 8. The resulting file(s) can be concatenated as needed and interpreted by tools that interpret raw pixel data. For example ffmpeg can be used with the -pix_fmt option set to yuv420p, yuv422p, yuv444p, yuva420p, yuva422p, yuva444p, rgb, or rgba based on the image data that is present and whether it is planar or interleaved.

## generate_color_test_pattern

The generate_color_test_pattern example creates a color bar style pattern for certain combinations of transfer function and bits per sample.

Usage:

```
generate_color_test_pattern -o out.dpx -tf (BT709|HLG|PQ) -corder (corder) -userfr (1|0) -bpc (8|10|12) -planar (1|0) -chroma (444|422|420) -w (width) -h (height) -dmd (l2r|r2l) -order (msbf|lsbf) -packing (packed|ma|mb) -encoding (1|0)
```

* out.dpx - Name of output file
* tf - Transfer function (BT709 = ITU-R BT.709, HLG = ITU-R BT.2100 HLG, PQ = ITU_R BT.2100 PQ)
* corder - component order of output file (e.g., CbYCr or BGR)
* usefr - 0 = use limited range; 1 = use full range
* bpc - Specifies bit depth of output file
* planar - 0 = interleaved samples, all components in 1 image element; 1 = planar samples, each component in separate image element
* chroma - 444 = 4:4:4; 422 = 4:2:2; 420 = 4:2:0
* w - output width
* h - output height
* dmd - datum mapping direction; either left to right (l2r) or right to left (r2l)
* order - byte order; either most-significant byte first (msbf) or least-significant byte first (lsbf)
* packing - packing to use; either packed (packed), Method A (ma), or Method B (mb). Note that ma and mb are only valid for 10 and 12 bpc
* encoding - 0 = no compression; 1 = RLE encoding used

The following is a list of supported formats: CbYCr, CbYCrA, CbYCrY422, CbYACrYA422, YCbCr422p, YCbCrA422p, CYY420, CYAYA420, YCbCr420p, YCbCrA420p, BGR, BGRA, ARGB RGB, RGBA, ABGR
