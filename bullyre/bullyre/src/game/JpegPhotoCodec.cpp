// The game's JPEG codec for the photo album (0x5863B0..0x586780): 32-bit RGBx pixels to a
// JPEG in memory and back, over libjpeg 6b (Jpeg6b.h, built from its source) with memory
// source and destination managers whose state is in globals, through a 1 KB staging buffer.

#include <cstdint>
#include <cstring>

#include "game/Jpeg6b.h"
#include "reversed.h"

namespace {

GAME_FN(void, __cdecl, Jpeg_Compress, 0x005864D0, const uint8_t* pixels, int width, int height, int pitch,
        jpeg_destination_mgr* dest);
GAME_FN(boolean, __cdecl, Jpeg_Decompress, 0x00586610, uint8_t* pixels, int width, int height, int pitch,
        jpeg_source_mgr* src);

constexpr uint32_t kStagingSize = 0x400;
GAME_ARRAY(JOCTET, g_jpegStaging, 0x00CF3588);  // [0x400]
// the memory destination's state
GAME_VAR(uint8_t*, g_jpegDestBuffer, 0x00CF3988);
GAME_VAR(uint32_t, g_jpegDestCapacity, 0x00CF398C);
GAME_VAR(uint32_t, g_jpegDestWritten, 0x00CF3990);
// the memory source's
GAME_VAR(const uint8_t*, g_jpegSourceData, 0x00CF3994);
GAME_VAR(uint32_t, g_jpegSourceOffset, 0x00CF3580);

// 0x5863B0: init_source (the staging buffer, empty)
void __cdecl SourceInit(j_decompress_ptr cinfo) {
    cinfo->src->bytes_in_buffer = 0;
    cinfo->src->next_input_byte = g_jpegStaging;
}

// 0x5863D0: fill_input_buffer (the next 1 KB of the JPEG, whatever is there: no end check)
boolean __cdecl SourceFill(j_decompress_ptr cinfo) {
    std::memcpy(g_jpegStaging, g_jpegSourceData + g_jpegSourceOffset, kStagingSize);
    cinfo->src->next_input_byte = g_jpegStaging;
    g_jpegSourceOffset += kStagingSize;
    cinfo->src->bytes_in_buffer = kStagingSize;
    return TRUE;
}

// skip_input_data and term_source: nothing (the game's shared empty function, 0x49A650)
void __cdecl SourceSkip(j_decompress_ptr, long) {}
void __cdecl SourceTerm(j_decompress_ptr) {}

// 0x586420: init_destination
void __cdecl DestInit(j_compress_ptr cinfo) {
    cinfo->dest->next_output_byte = g_jpegStaging;
    cinfo->dest->free_in_buffer = kStagingSize;
}

// 0x586440: empty_output_buffer (the staging buffer appended while it fits with room to
// spare, else dropped)
boolean __cdecl DestEmpty(j_compress_ptr cinfo) {
    if (g_jpegDestWritten + kStagingSize < g_jpegDestCapacity) {
        std::memcpy(g_jpegDestBuffer + g_jpegDestWritten, g_jpegStaging, kStagingSize);
        g_jpegDestWritten += kStagingSize;
    }
    cinfo->dest->next_output_byte = g_jpegStaging;
    cinfo->dest->free_in_buffer = kStagingSize;
    return TRUE;
}

// 0x586490: term_destination (what's left in the staging buffer appended; the count grows by
// a whole 1 KB however much it was, as the game's: the size reported is rounded up)
void __cdecl DestTerm(j_compress_ptr cinfo) {
    uint32_t used = uint32_t(cinfo->dest->next_output_byte - g_jpegStaging);
    if (g_jpegDestWritten + used < g_jpegDestCapacity) {
        std::memcpy(g_jpegDestBuffer + g_jpegDestWritten, g_jpegStaging, used);
        g_jpegDestWritten += kStagingSize;
    }
}

// 0x5864D0: compresses rows of 4-byte RGBx pixels (quality 75, the float DCT) to `dest`
// (the width isn't the pitch's: pixels are read `width` at a time)
void __cdecl Compress(const uint8_t* pixels, int width, int height, int pitch, jpeg_destination_mgr* dest) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    cinfo.image_width = JDIMENSION(width);
    cinfo.image_height = JDIMENSION(height);
    cinfo.input_components = 4;
    cinfo.dct_method = JDCT_FLOAT;
    cinfo.dest = dest;
    jpeg_start_compress(&cinfo, TRUE);
    JSAMPROW row = const_cast<JSAMPROW>(pixels);
    for (int y = height; y > 0; y--) {
        jpeg_write_scanlines(&cinfo, &row, 1);
        row += pitch;
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

// 0x5865A0: JpegEncode (into `out`, *size bytes of room; *size set to the JPEG's size).
// Returns nothing (eax is left from 0x5864D0)
void __cdecl JpegEncode(const uint8_t* pixels, int width, int height, int pitch, uint8_t* out, uint32_t* size) {
    g_jpegDestBuffer = out;
    g_jpegDestCapacity = *size;
    g_jpegDestWritten = 0;
    jpeg_destination_mgr dest;  // (the buffer pointer and count are set by init_destination)
    dest.init_destination = &DestInit;
    dest.empty_output_buffer = &DestEmpty;
    dest.term_destination = &DestTerm;
    Jpeg_Compress(pixels, width, height, pitch, &dest);
    *size = g_jpegDestWritten;
}

// 0x586610: decompresses into rows of 4-byte pixels (the float IDCT); FALSE when the
// header or the start fails, or a row doesn't come
boolean __cdecl Decompress(uint8_t* pixels, int, int, int pitch, jpeg_source_mgr* src) {
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    cinfo.src = src;
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        return FALSE;
    }
    cinfo.dct_method = JDCT_FLOAT;
    if (!jpeg_start_decompress(&cinfo)) {
        jpeg_destroy_decompress(&cinfo);
        return FALSE;
    }
    JSAMPROW row = pixels;
    for (JDIMENSION y = 0; y < cinfo.image_height;) {
        JDIMENSION got = jpeg_read_scanlines(&cinfo, &row, 1);
        if (got != 1) {
            jpeg_destroy_decompress(&cinfo);
            return FALSE;
        }
        row += pitch;
        y += got;
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return TRUE;
}

// 0x586710: JpegDecode (from the JPEG at `jpeg`; TRUE unless it's null, whether or not
// the decoding worked)
uint8_t __cdecl JpegDecode(uint8_t* pixels, int width, int height, int pitch, const uint8_t* jpeg) {
    if (!jpeg) {
        return 0;
    }
    g_jpegSourceData = jpeg;
    g_jpegSourceOffset = 0;
    jpeg_source_mgr src;  // (the buffer pointer and count are set by init_source)
    src.init_source = &SourceInit;
    src.fill_input_buffer = &SourceFill;
    src.skip_input_data = &SourceSkip;  // (APPn data is read as markers: nothing is skipped)
    src.resync_to_restart = &jpeg_resync_to_restart;
    src.term_source = &SourceTerm;
    Jpeg_Decompress(pixels, width, height, pitch, &src);
    return 1;
}

} // namespace

#define JPEGPHOTO_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(jpegphoto_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(jpegphoto_original_, __LINE__), \
             re::Mode::Original)

JPEGPHOTO_REVERSED("PhotoJpeg::SourceInit", 0x005863B0, &SourceInit);
JPEGPHOTO_REVERSED("PhotoJpeg::SourceFill", 0x005863D0, &SourceFill);
JPEGPHOTO_REVERSED("PhotoJpeg::DestInit", 0x00586420, &DestInit);
JPEGPHOTO_REVERSED("PhotoJpeg::DestEmpty", 0x00586440, &DestEmpty);
JPEGPHOTO_REVERSED("PhotoJpeg::DestTerm", 0x00586490, &DestTerm);
JPEGPHOTO_REVERSED("PhotoJpeg::Compress", 0x005864D0, &Compress);
JPEGPHOTO_REVERSED("PhotoJpeg::Encode", 0x005865A0, &JpegEncode);
JPEGPHOTO_REVERSED("PhotoJpeg::Decompress", 0x00586610, &Decompress);
JPEGPHOTO_REVERSED("PhotoJpeg::Decode", 0x00586710, &JpegDecode);
