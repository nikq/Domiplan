
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <lens.hpp>

int main(int argc, char *argv[])
{
    Lens::Body lens = Lens::Loader::ZEMAX::load(argv[1]);
    lens.dump();
    Lens::Plotter plot;
    plot.draw(lens);

    stbi_write_png(argv[2], plot.canvas_.width(), plot.canvas_.height(), 3,
        plot.canvas_.getLDR8().data(),
        static_cast<int>(plot.canvas_.width()) * 3);

    return 0;
}