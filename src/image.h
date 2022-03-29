#ifndef IMAGE_H_
#define IMAGE_H_

#include <string>

#include "util/stb_image.h"
#include "util/stb_image_write.h"

/**
 * RGBA color value in the range [0.0 - 1.0]
 */
struct Color {
    double r;
    double g;
    double b;
    double a;
};

/**
 * Image is a pixel sheet that can be both drawn and drawn to
 */
struct Image {
    int width;
    int height;
    int channels;

    unsigned char *data;
    bool from_file;

    Image(int width, int height);
    Image(std::string filename);
    ~Image();

    /**
     * Get the color of a pixel
     */
    Color get_at(int x, int y);

    /**
     * Draw a color over a pixel with alpha blending
     */
    void draw_at(Color color, int x, int y);

    /**
     * Overwrite the color of a pixel
     */
    void set_at(Color color, int x, int y);

    /**
     * Fill the image with a color
     */
    void fill(Color color);

    /**
     * Draw another image from the top left corner
     */
    void draw(Image *image, int x, int y);

    /**
     * Save an image to disk (as a png)
     */
    void save(std::string filename);
};

#endif