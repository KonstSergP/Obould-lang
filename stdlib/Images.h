#ifndef IMAGES_H
#define IMAGES_H
#include <stdbool.h>
#include <stdint.h>

typedef struct ob_6Images_ImageHandle ob_6Images_ImageHandle;
typedef ob_6Images_ImageHandle* ob_6Images_Image;

ob_6Images_Image ob_6Images_Create(int64_t width, int64_t height);
int64_t ob_6Images_Width(ob_6Images_Image image);
int64_t ob_6Images_Height(ob_6Images_Image image);
void ob_6Images_SetRGB(ob_6Images_Image image, int64_t x, int64_t y, uint8_t r, uint8_t g, uint8_t b);
bool ob_6Images_SavePNG(ob_6Images_Image image, const char* path, int64_t path_len);
void ob_6Images_6Images(void);

#endif
