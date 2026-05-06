#include <gc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "Images.h"

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif


struct ob_6Images_ImageHandle
{
    int64_t width;
    int64_t height;
    uint8_t* pixels;
};

typedef ob_6Images_Image Image;


Image Create(int64_t width, int64_t height) OB_SYMBOL("6Images_Create");
int64_t Width(Image image) OB_SYMBOL("6Images_Width");
int64_t Height(Image image) OB_SYMBOL("6Images_Height");
void SetRGB(Image image, int64_t x, int64_t y, uint8_t r, uint8_t g, uint8_t b) OB_SYMBOL("6Images_SetRGB");
bool SavePNG(Image image, const char* path, int64_t path_len) OB_SYMBOL("6Images_SavePNG");
void Images(void) OB_SYMBOL("6Images_6Images");


static char* to_c_string(const char* s, int64_t s_len)
{
    if (s_len < 0) s_len = 0;
    size_t max = (size_t)s_len;

    if (s == NULL) {
        char* out = GC_MALLOC_ATOMIC(1);
        if (out) out[0] = '\0';
        return out;
    }

    size_t actual = 0;
    while (actual < max && s[actual] != '\0') {
        actual++;
    }

    char* out = GC_MALLOC_ATOMIC(actual + 1);
    if (!out) return NULL;
    if (actual > 0) {
        memcpy(out, s, actual);
    }
    out[actual] = '\0';
    return out;
}

Image Create(int64_t width, int64_t height)
{
    if (width <= 0 || height <= 0) return NULL;
    Image img = GC_MALLOC(sizeof(*img));
    if (!img) return NULL;
    size_t size = (size_t)width * (size_t)height * 3;
    img->pixels = GC_MALLOC_ATOMIC(size);
    if (!img->pixels) return NULL;
    memset(img->pixels, 0, size);
    img->width = width;
    img->height = height;
    return img;
}

int64_t Width(Image image)
{
    return image == NULL ? 0 : image->width;
}

int64_t Height(Image image)
{
    return image == NULL ? 0 : image->height;
}

void SetRGB(Image image, int64_t x, int64_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (!image || x < 0 || y < 0 || x >= image->width || y >= image->height) return;
    int64_t offset = (y * image->width + x) * 3;
    image->pixels[offset] = r;
    image->pixels[offset + 1] = g;
    image->pixels[offset + 2] = b;
}


static uint32_t crc32_update(uint32_t crc, uint8_t byte)
{
    crc ^= (uint32_t)byte;
    for (int bit = 0; bit < 8; bit++) {
        if ((crc & 1) != 0) {
            crc = (crc >> 1) ^ 0xEDB88320;
        }
        else {
            crc >>= 1;
        }
    }
    return crc;
}

static uint32_t crc32_bytes(const char type[4], const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < 4; i++) {
        crc = crc32_update(crc, (uint8_t)type[i]);
    }
    for (size_t i = 0; i < len; i++) {
        crc = crc32_update(crc, data[i]);
    }
    return crc ^ 0xFFFFFFFF;
}

static uint32_t adler32_bytes(const uint8_t* data, size_t len)
{
    uint32_t a = 1U;
    uint32_t b = 0U;
    size_t i;

    for (i = 0; i < len; i++) {
        a = (a + data[i]) % 65521U;
        b = (b + a) % 65521U;
    }
    return (b << 16) | a;
}

static uint8_t* zlib_store(const uint8_t* data, size_t len, size_t* out_len) // TODO: реализовать сжатие
{
    size_t blocks = (len + 65534) / 65535;
    size_t capacity = 2 + len + blocks * 5 + 4;
    uint8_t* out = GC_MALLOC_ATOMIC(capacity);
    if (out == NULL || out_len == NULL) {
        return NULL;
    }

    size_t in_offset = 0;
    size_t out_offset = 0;


    out[out_offset++] = 0x78U; // алгоритм deflate, окно 32КБ
    out[out_offset++] = 0x01U; // минимальный уровень сжатия

    while (in_offset < len) {
        size_t remaining = len - in_offset;
        uint16_t block_len = (uint16_t)(remaining > 65535 ? 65535 : remaining);
        uint16_t nlen = (uint16_t)~block_len;
        int final_block = remaining <= 65535U;

        out[out_offset++] = (uint8_t)(final_block ? 0x01 : 0x00); // без сжатия
        out[out_offset++] = (uint8_t)(block_len & 0xFF);
        out[out_offset++] = (uint8_t)((block_len >> 8) & 0xFF);
        out[out_offset++] = (uint8_t)(nlen & 0xFF);
        out[out_offset++] = (uint8_t)((nlen >> 8) & 0xFF);
        memcpy(out + out_offset, data + in_offset, block_len);
        out_offset += block_len;
        in_offset += block_len;
    }

    {
        uint32_t adler = adler32_bytes(data, len);
        out[out_offset++] = (uint8_t)((adler >> 24) & 0xFF);
        out[out_offset++] = (uint8_t)((adler >> 16) & 0xFF);
        out[out_offset++] = (uint8_t)((adler >> 8) & 0xFF);
        out[out_offset++] = (uint8_t)(adler & 0xFF);
    }

    *out_len = out_offset;
    return out;
}

static uint8_t* build_scanlines(Image image, size_t* out_len)
{
    size_t row_size = 1 + image->width * 3;
    size_t total_size = row_size * image->height;
    uint8_t* raw = GC_MALLOC_ATOMIC(total_size);
    if (!raw) return NULL;

    for (int64_t y = 0; y < image->height; y++) {
        size_t row_offset = y * row_size;
        raw[row_offset] = 0; // без фильтрации
        memcpy(&raw[row_offset + 1], &image->pixels[y * image->width * 3], image->width * 3);
    }
    *out_len = total_size;
    return raw;
}

static void write_u32_be(FILE* out, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (uint8_t)((value >> 24) & 0xFFU);
    bytes[1] = (uint8_t)((value >> 16) & 0xFFU);
    bytes[2] = (uint8_t)((value >> 8) & 0xFFU);
    bytes[3] = (uint8_t)(value & 0xFFU);
    fwrite(bytes, 1, 4, out);
}

static bool write_chunk(FILE* out, const char type[4], const uint8_t* data, size_t len)
{
    uint32_t crc;

    write_u32_be(out, (uint32_t)len);
    if (fwrite(type, 1, 4, out) != 4) {
        return false;
    }
    if (fwrite(data, 1, len, out) != len) {
        return false;
    }

    crc = crc32_bytes(type, data, len);
    write_u32_be(out, crc);
    return !ferror(out);
}

bool SavePNG(Image image, const char* path, int64_t path_len)
{
    size_t scanlines_len;
    size_t compressed_len;

    if (image == NULL) return false;

    char* path_c = to_c_string(path, path_len);
    if (path_c == NULL) {
        return false;
    }

    FILE* out = fopen(path_c, "wb");
    if (out == NULL) {
        return false;
    }

    static const uint8_t png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (fwrite(png_signature, 1, sizeof(png_signature), out) != sizeof(png_signature)) {
        fclose(out);
        return false;
    }

    uint8_t ihdr[13];
    ihdr[0] = (uint8_t)((image->width >> 24) & 0xFF);
    ihdr[1] = (uint8_t)((image->width >> 16) & 0xFF);
    ihdr[2] = (uint8_t)((image->width >> 8) & 0xFF);
    ihdr[3] = (uint8_t)(image->width & 0xFF);
    ihdr[4] = (uint8_t)((image->height >> 24) & 0xFF);
    ihdr[5] = (uint8_t)((image->height >> 16) & 0xFF);
    ihdr[6] = (uint8_t)((image->height >> 8) & 0xFF);
    ihdr[7] = (uint8_t)(image->height & 0xFF);
    ihdr[8] = 8; // 8 бит на цвет
    ihdr[9] = 2; // не монохромное
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;

    if (!write_chunk(out, "IHDR", ihdr, sizeof(ihdr))) {
        fclose(out);
        return false;
    }

    uint8_t* scanlines = build_scanlines(image, &scanlines_len);
    uint8_t* compressed = zlib_store(scanlines, scanlines_len, &compressed_len);
    if (scanlines == NULL || compressed == NULL) {
        fclose(out);
        return false;
    }

    if (!write_chunk(out, "IDAT", compressed, compressed_len)) {
        fclose(out);
        return false;
    }
    if (!write_chunk(out, "IEND", NULL, 0)) {
        fclose(out);
        return false;
    }

    return !ferror(out) && fclose(out) == 0;
}

void Images(void) {}
