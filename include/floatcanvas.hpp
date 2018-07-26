// copyright(c) 2018 Hajime UCHIMURA / nikq
#ifndef __FLOATCANVAS_H
#define __FLOATCANVAS_H
#include <colorsystem.hpp>

#include <vector>

namespace FloatCanvas
{
typedef ColorSystem::Tristimulus Pixel;
typedef ColorSystem::Gamut       Gamut;

class Canvas
{
  public:
    size_t             width_;
    size_t             height_;
    std::vector<Pixel> pixel_;
    Gamut              gamut_;

    Canvas() : width_(0), height_(0), pixel_(0), gamut_(ColorSystem::Rec709) { ; }
    Canvas(size_t w, size_t h, const ColorSystem::Gamut &g = ColorSystem::Rec709) : width_(w), height_(h), pixel_(w * h), gamut_(g) { ; }
    virtual ~Canvas() {}

    const size_t width() { return width_; }
    const size_t height() { return height_; }
    const Gamut &gamut() { return gamut_; }

    void setup(size_t w, size_t h, const ColorSystem::Gamut &g = ColorSystem::Rec709)
    {
        width_  = w;
        height_ = h;
        pixel_.resize(w * h);
        gamut_ = g;
    }

    Pixel &pixel(int x, int y)
    {
        return pixel_[y * width_ + x];
    }
    const Pixel &pixel(int x, int y) const
    {
        return (x < 0 || y < 0 || x >= width_ || y >= height_) ? Pixel() : pixel_[y * width_ + x];
    }

    void fill(const Pixel &pixel)
    {
        for (auto &p : pixel_)
        {
            p = pixel;
        }
    }

    std::vector<uint8_t> getLDR8(
        const ColorSystem::Gamut &   space = ColorSystem::Rec709,
        const ColorSystem::OTF::TYPE otf   = ColorSystem::OTF::SRGB)
    {
        const ColorSystem::Matrix3 mat = ColorSystem::GamutConvert(gamut_, space);

        std::vector<uint8_t> rgb(width() * height() * 3);
        for (int i = 0; i < width_ * height_; i++)
        {
            const Pixel sdr = ColorSystem::OTF::toScreen(otf, pixel_[i].apply(mat).scale(100.f).clip(0.f, 100.f));
            rgb[i * 3 + 0]  = (uint8_t)(sdr[0] * 255);
            rgb[i * 3 + 1]  = (uint8_t)(sdr[1] * 255);
            rgb[i * 3 + 2]  = (uint8_t)(sdr[2] * 255);
        }
        return rgb;
    }

    std::vector<float> getHDR(
        const ColorSystem::Gamut &   space = ColorSystem::Rec709,
        const ColorSystem::OTF::TYPE otf   = ColorSystem::OTF::SRGB)
    {
        const ColorSystem::Matrix3 mat = ColorSystem::GamutConvert(gamut_, space);

        std::vector<float> rgb(width() * height() * 3);
        for (int i = 0; i < width_ * height_; i++)
        {
            const Pixel p  = pixel_[i].apply(mat);
            rgb[i * 3 + 0] = p[0];
            rgb[i * 3 + 1] = p[1];
            rgb[i * 3 + 2] = p[2];
        }
        return rgb;
    }

    void inline setDot(int x, int y, const Pixel &p, float a = 1.f)
    {
        pixel_[y * width_ + x] = pixel_[y * width_ + x] * (1.f - a) + p * a;
    }
    void inline setPixel(int x, int y, const Pixel &p, float a = 1.f)
    {
        if (x >= 0 && y >= 0 && x < width_ && y < height_)
            setDot(x, y, p, a);
    }
    void inline putPixel(const float x, const float y, const Pixel &p, float a = 1.f)
    {
        int   xl = (int)floor(x);
        float xr = x - floor(x);
        int   yl = (int)floor(y);
        float yr = y - floor(y);
        setPixel(xl + 0, yl + 0, p * (1.f - xr) * (1.f - yr), a);
        setPixel(xl + 1, yl + 0, p * xr * (1.f - yr), a);
        setPixel(xl + 0, yl + 1, p * (1.f - xr) * yr, a);
        setPixel(xl + 1, yl + 1, p * xr * yr, a);
    }

    static inline float distanceFromLine(
        const float x1, const float y1,
        const float x2, const float y2,
        const float x, const float y)
    {
        const float a  = x2 - x1;
        const float b  = y2 - y1;
        const float a2 = a * a;
        const float b2 = b * b;
        const float r2 = a2 + b2;
        const float tt = a * (x - x1) + b * (y - y1);
        const float f1 = a * (y1 - y) - b * (x1 - x);
        const float l  = (tt < 0.f) ? (x - x1) * (x - x1) + (y - y1) * (y - y1) : (tt > r2) ? (x - x2) * (x - x2) + (y - y2) * (y - y2) : (f1 * f1) / r2;
        return sqrtf(l);
    }

    static constexpr float f_max(const float a, const float b) { return (a > b) ? a : b; }
    static constexpr float f_min(const float a, const float b) { return (a < b) ? a : b; }

    void drawLine(float x1, float y1, float x2, float y2,
        const Pixel &color, float s, float g = 1.0, float a2 = 1.0)
    {
        const float a = y2 - y1;
        const float b = x1 - x2;
        const float c = x2 * y1 - x1 * y2;
        const float l = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);

        const int xa = (int)f_max(f_min(x1, x2) - s - 2.0f, 0.0f);
        const int ya = (int)f_max(f_min(y1, y2) - s - 2.0f, 0.0f);
        const int xb = (int)f_min(f_max(x1, x2) + s + 2.0f, width_ - 1.f);
        const int yb = (int)f_min(f_max(y1, y2) + s + 2.0f, height_ - 1.f);

        if (xa > xb || ya > yb)
            return;

        const float r = 1.0f / sqrt(a * a + b * b);
        for (int iy = ya; iy <= yb; iy++)
        {
            for (int ix = xa; ix <= xb; ix++)
            {
                const float d_l = fabs(ix * a + iy * b + c) * r; //sqrt( a*a + b*b );
                if (d_l > s + 1.0f)
                    continue;

                const float d_a = (ix - x1) * (ix - x1) + (iy - y1) * (iy - y1);
                const float d_b = (ix - x2) * (ix - x2) + (iy - y2) * (iy - y2);

                float d = (d_a + d_b) - l;
                if (d > 0.0f)
                    d = sqrt(f_min(d_a, d_b));
                else
                    d = d_l;

                float alpha = cos(pow(f_min(1.0f, f_max(0.0f, d / s)), g) * 3.14159265359f / 2.0f) * a2;

                const size_t k = (ix + iy * width_) * 3;

                pixel_[k] = pixel_[k] * (1.0f - alpha) + color * alpha;
            }
        }
    }

    class Triangle
    {
      public:
        typedef struct _point
        {
            float x_, y_;
            _point(const float x, const float y) : x_(x), y_(y) { ; }
        } Point;

        Point p0_, p1_, p2_;

        Triangle(const float x0, const float y0, const float x1, const float y1, const float x2, const float y2) : p0_(x0, y0), p1_(x1, y1), p2_(x2, y2)
        {
            ;
        }
        virtual ~Triangle() { ; }

        constexpr float cross(const Point &a, const Point &b)
        {
            return a.x_ * b.y_ - a.y_ * b.x_;
        }
        bool inside(const Point &p, float &u, float &v) const
        {
            //http://stackoverflow.com/questions/2049582/how-to-determine-a-point-in-a-2d-triangle
            float A    = (-p1_.y_ * p2_.x_ + p0_.y_ * (-p1_.x_ + p2_.x_) + p0_.x_ * (p1_.y_ - p2_.y_) + p1_.x_ * p2_.y_) / 2.f;
            float sign = A < 0.f ? -1.f : 1.f;
            float s    = (p0_.y_ * p2_.x_ - p0_.x_ * p2_.y_ + (p2_.y_ - p0_.y_) * p.x_ + (p0_.x_ - p2_.x_) * p.y_) * sign;
            float t    = (p0_.x_ * p1_.y_ - p0_.y_ * p1_.x_ + (p0_.y_ - p1_.y_) * p.x_ + (p1_.x_ - p0_.x_) * p.y_) * sign;
            float d    = 2.f * A * sign;
            u          = s / d;
            v          = t / d;
            return s >= 0 && t >= 0 && (s + t) < d;
        }
    };

    void drawTriangle(const float x0, const float y0, const float x1, const float y1, const float x2, const float y2, const Pixel &color, float a = 1.f)
    {
        int xl = (int)floor(std::max(0.f, std::min(std::min(x0, x1), x2)));
        int xh = (int)ceil(std::min((float)width_ - 1, std::max(std::max(x0, x1), x2)));
        int yl = (int)floor(std::max(0.f, std::min(std::min(y0, y1), y2)));
        int yh = (int)ceil(std::min((float)height_ - 1.f, std::max(std::max(y0, y1), y2)));

        Triangle triangle(x0, y0, x1, y1, x2, y2);

        for (int y = yl; y <= yh; y++)
        {
            for (int x = xl; x <= xh; x++)
            {
                float u, v;
                bool  inside = triangle.inside(Triangle::Point((float)x, (float)y), u, v);
                if (inside)
                {
                    setDot(x, y, color, a);
                }
            }
        }
    }
};
} // namespace FloatCanvas
#endif