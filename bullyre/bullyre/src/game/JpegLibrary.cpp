// The IJG JPEG library, release 6b, from its source (third_party/jpeg-6b, configured as the
// game built it: see its BULLY.md) in place of the game's copy at 0x57A4F0..0x586820 (the
// photo album's encoder and decoder, used by JpegPhotoCodec.cpp). Every function of the
// game's copy that the source exports is registered here with the source's; the rest are
// the library's own statics, which the source's jinit_* functions install as methods. The
// memory system (jmemnobs: 0x586360..0x586390) isn't registered: each copy keeps its own
// pair of allocate and free.
//
// The structures are the game's: these sizes are what its code allocates and passes in
// (jpeg_CreateCompress checks the one it's given).

#include <cstddef>
#include <cstdio>

// the library's own modules' entry points too (jpegint.h, jchuff.h, jdhuff.h)
#define JPEG_INTERNALS
#include "game/Jpeg6b.h"
#include "reversed.h"
extern "C" {
#include "jchuff.h"
#include "jdhuff.h"
}

static_assert(sizeof(boolean) == 1);
static_assert(sizeof(JQUANT_TBL) == 0x82);
static_assert(sizeof(JHUFF_TBL) == 0x112);
#if !defined(_WIN64)   // (the game's libjpeg layouts: the 64-bit build's are the library's own)
static_assert(sizeof(jpeg_component_info) == 0x54);
static_assert(sizeof(jpeg_error_mgr) == 0x84);
static_assert(sizeof(jpeg_memory_mgr) == 0x34);
static_assert(sizeof(jpeg_compress_struct) == 0x168);
static_assert(sizeof(jpeg_decompress_struct) == 0x1B0);
static_assert(offsetof(jpeg_compress_struct, dest) == 0x18);
static_assert(offsetof(jpeg_compress_struct, input_components) == 0x24);
static_assert(offsetof(jpeg_compress_struct, optimize_coding) == 0xB2);
static_assert(offsetof(jpeg_compress_struct, dct_method) == 0xB8);
static_assert(offsetof(jpeg_decompress_struct, src) == 0x18);
static_assert(offsetof(jpeg_decompress_struct, dct_method) == 0x44);
static_assert(offsetof(jpeg_decompress_struct, output_scanline) == 0x78);
#endif

#define JPEG_REVERSED(fn, address) \
    REVERSED("libjpeg::" #fn, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// compression (jcapimin.c .. jcsample.c)
JPEG_REVERSED(jpeg_CreateCompress, 0x0057A4F0);
JPEG_REVERSED(jpeg_suppress_tables, 0x0057A5C0);
JPEG_REVERSED(jpeg_finish_compress, 0x0057A630);
JPEG_REVERSED(jpeg_start_compress, 0x0057A750);
JPEG_REVERSED(jpeg_write_scanlines, 0x0057A7D0);
JPEG_REVERSED(jinit_c_coef_controller, 0x0057B050);
JPEG_REVERSED(jinit_color_converter, 0x0057B640);
JPEG_REVERSED(jinit_forward_dct, 0x0057C1A0);
JPEG_REVERSED(jpeg_make_c_derived_tbl, 0x0057C210);
JPEG_REVERSED(jpeg_gen_optimal_table, 0x0057CB40);
JPEG_REVERSED(jinit_huff_encoder, 0x0057D020);
JPEG_REVERSED(jinit_compress_master, 0x0057D080);
JPEG_REVERSED(jinit_c_main_controller, 0x0057D240);
JPEG_REVERSED(jinit_marker_writer, 0x0057E140);
JPEG_REVERSED(jinit_c_master_control, 0x0057E830);
JPEG_REVERSED(jpeg_abort, 0x0057E8F0);
JPEG_REVERSED(jpeg_destroy, 0x0057E930);
JPEG_REVERSED(jpeg_alloc_quant_table, 0x0057E960);
JPEG_REVERSED(jpeg_alloc_huff_table, 0x0057E980);
JPEG_REVERSED(jpeg_add_quant_table, 0x0057E9A0);
JPEG_REVERSED(jpeg_set_colorspace, 0x0057EC80);
JPEG_REVERSED(jpeg_default_colorspace, 0x0057EF70);
JPEG_REVERSED(jpeg_set_defaults, 0x0057F230);
JPEG_REVERSED(jinit_c_prep_controller, 0x0057F5A0);
JPEG_REVERSED(jinit_downsampler, 0x0057FA40);

// decompression (jdapimin.c .. jdsample.c)
JPEG_REVERSED(jpeg_CreateDecompress, 0x0057FB50);
JPEG_REVERSED(jpeg_consume_input, 0x0057FE20);
JPEG_REVERSED(jpeg_finish_decompress, 0x0057FEF0);
JPEG_REVERSED(jpeg_read_header, 0x0057FFC0);
JPEG_REVERSED(jpeg_read_scanlines, 0x005800B0);
JPEG_REVERSED(jpeg_start_decompress, 0x00580150);
JPEG_REVERSED(jinit_d_coef_controller, 0x005804E0);
JPEG_REVERSED(jinit_color_deconverter, 0x00580A30);
JPEG_REVERSED(jinit_inverse_dct, 0x00580D70);
JPEG_REVERSED(jpeg_make_d_derived_tbl, 0x00580DF0);
JPEG_REVERSED(jpeg_fill_bit_buffer, 0x005810C0);
JPEG_REVERSED(jpeg_huff_decode, 0x005811F0);
JPEG_REVERSED(jinit_huff_decoder, 0x005818D0);
JPEG_REVERSED(jinit_input_controller, 0x00581EE0);
JPEG_REVERSED(jinit_d_main_controller, 0x005824E0);
JPEG_REVERSED(jpeg_resync_to_restart, 0x00583E70);
JPEG_REVERSED(jinit_marker_reader, 0x00583FA0);
JPEG_REVERSED(jpeg_calc_output_dimensions, 0x00584040);
JPEG_REVERSED(jinit_master_decompress, 0x00584460);
JPEG_REVERSED(jinit_d_post_controller, 0x005845A0);
JPEG_REVERSED(jinit_upsampler, 0x00584AF0);

// both (jerror.c, jmemmgr.c, jutils.c)
JPEG_REVERSED(jpeg_std_error, 0x00584E50);
JPEG_REVERSED(jinit_memory_mgr, 0x00586230);
JPEG_REVERSED(jdiv_round_up, 0x00586780);
JPEG_REVERSED(jround_up, 0x00586790);
JPEG_REVERSED(jcopy_sample_rows, 0x005867B0);
JPEG_REVERSED(jzero_far, 0x00586800);
