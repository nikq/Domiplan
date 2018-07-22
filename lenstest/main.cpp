
#include <lens.hpp>

int main(int argc, char *argv[])
{
    Lens::Body lens = Lens::Loader::ZEMAX::load(argv[1]);
    lens.dump();

    return 0;
}