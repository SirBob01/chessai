#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "image.h"

Image::Image(int width, int height) : width(width), height(height) {
    from_file = false;
    data = new unsigned char[(width * height) * 4];
    for (int i = 0; i < (width * height) * 4; i++) {
        data[i] = 0;
    }
}

Image::Image(std::string filename) {
    int desired_channels = 4; // rgba
    data = stbi_load(filename.c_str(),
                     &width,
                     &height,
                     &channels,
                     desired_channels);
    assert(data);
    from_file = true;
}

Image::~Image() {
    if (from_file) {
        stbi_image_free(data);
    } else {
        delete[] data;
    }
}

Color Image::get_at(int x, int y) {
    int start_i = y * (width * 4) + (x * 4);
    if (start_i > (width * height * 4)) return {0, 0, 0, 0};
    return {
        data[start_i] / 255.0,
        data[start_i + 1] / 255.0,
        data[start_i + 2] / 255.0,
        data[start_i + 3] / 255.0,
    };
}

void Image::draw_at(Color color, int x, int y) {
    int start_i = y * (width * 4) + (x * 4);
    if (start_i > (width * height * 4)) return;
    Color current = get_at(x, y);
    double a0 = color.a + current.a * (1 - color.a);
    Color blended = {
        (color.r * color.a + current.r * current.a * (1 - color.a)) / a0,
        (color.g * color.a + current.g * current.a * (1 - color.a)) / a0,
        (color.b * color.a + current.b * current.a * (1 - color.a)) / a0,
        a0,
    };

    data[start_i] = 255 * blended.r;
    data[start_i + 1] = 255 * blended.g;
    data[start_i + 2] = 255 * blended.b;
    data[start_i + 3] = 255 * blended.a;
}

void Image::set_at(Color color, int x, int y) {
    int start_i = y * (width * 4) + (x * 4);
    if (start_i > (width * height * 4)) return;

    data[start_i] = 255 * color.r;
    data[start_i + 1] = 255 * color.g;
    data[start_i + 2] = 255 * color.b;
    data[start_i + 3] = 255 * color.a;
}

void Image::fill(Color color) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            set_at(color, col, row);
        }
    }
}

void Image::draw(Image *image, int x, int y) {
    for (int row = 0; row < image->height; row++) {
        for (int col = 0; col < image->width; col++) {
            draw_at(image->get_at(col, row), x + col, y + row);
        }
    }
}

void Image::save(std::string filename) {
    int success =
        stbi_write_png(filename.c_str(), width, height, 4, data, 4 * width);
    assert(success);
}