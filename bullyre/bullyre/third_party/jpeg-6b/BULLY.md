# libjpeg 6b, as Bully built it

The game's photo album encodes and decodes its photos with the IJG JPEG library, release 6b (JPEG_LIB_VERSION 62). The game's copy is at 0x57A4F0..0x586390 and 0x586780..0x586820 in Bully.unpacked.exe. This copy is the release from ijg.org (`jpegsrc.v6b.tar.gz`, md5 dbd5f3b47ed13132f04c685d608a7547), with the configuration read from the game's code. `src/game/JpegLibrary.cpp` registers it at the game's addresses. The photo codec over it is game code: `src/game/JpegPhotoCodec.cpp`.

## Configuration

- **`jconfig.h`** is the release's `jconfig.vc`, unchanged. `boolean` is an unsigned char, as on Windows. The game's structures have byte booleans: JQUANT_TBL is 0x82 bytes, and optimize_coding is at +0xB2.
- **`jmorecfg.h`** sets the features in the game's build. Each edit is marked `Bully`:
  - RGB_PIXELSIZE is 4. The photos are RGBx: rgb_ycc_convert and ycc_rgb_convert step 4 bytes.
  - DCT: the float DCT only. The game's start_pass_fdctmgr accepts JDCT_FLOAT and nothing else, and its photos ask for it. JDCT_DEFAULT stays JDCT_ISLOW, as jpeg_set_defaults writes 0.
  - Left out, because the game's build has none of their code:
    - progressive (jcphuff, jdphuff);
    - input smoothing;
    - the multi-scan decompressor and block smoothing (its jdcoefct has only the one-pass coefficient controller);
    - IDCT scaling (its jpeg_calc_output_dimensions is the short one);
    - merged upsampling (jdmerge);
    - both colour quantizers.
  - Kept: the multi-scan compressor (compress_first_pass is there) and entropy optimisation (the gather pass is there).
- **Modules:** the ones the game has, as listed in CMakeLists' glob. That's the jc*, jd* and jfdctflt/jidctflt files here, plus jerror, jmemmgr, jutils, and jmemnobs (malloc and free; the game's calls its C runtime's).

JpegLibrary.cpp checks the structure sizes and offsets against the game's: jpeg_compress_struct is 0x168 bytes, jpeg_decompress_struct is 0x1B0, and so on.

Not reproduced: the game's jpeg_std_error has a one-entry message table (0x91C5EC). The messages are only text, for errors the photos never hit; ours has the library's full table.

## Checked

`tools/jpegparity.py` runs the game's photo compressor (0x5864D0) and ours in unicorn on test images, and compares the JPEGs byte for byte. It then decompresses each JPEG with the game's (0x586610) and with ours and compares the pixels. Everything is identical.
