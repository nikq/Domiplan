﻿// copyright(c) 2018 Hajime UCHIMURA / nikq

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <vector>

#include <floatcanvas.hpp>
#include <vectormath.hpp>

namespace Lens
{
typedef VECTORMATH::Vector3<double> Vector;

inline Vector reflect(const Vector &I, const Vector &N)
{
    return I - N * 2. * N.dot(I);
}

bool refract(const Vector &I, Vector &N, double eta, Vector &result)
{
    if (I.dot(N) > 0.)
        N = N * -1.;
    double cosi  = -I.dot(N); // dot(-i, n);
    double cost2 = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (cost2 < 0.)
        return false; // 全反射.
    result = I * eta + (N * (eta * cosi - sqrt(cost2)));
    return true;
}

// coat_thickness must be in nm.
// lambda is in nm.
double single_coat_reflectance(double lambda, double ior_in, double coat_ior, double coat_thickness, const Vector &dir, const Vector &norm)
{
    double cosTheta1     = dir.dot(norm);
    double cosTheta2     = (ior_in / coat_ior) * sqrt((coat_ior * coat_ior) / (ior_in * ior_in) - (1. - cosTheta1 * cosTheta1));
    double distance_diff = 2. * coat_thickness * (coat_ior / ior_in) * cosTheta2;
    double m             = fmod(distance_diff, lambda) / lambda; // ズレ幅0-1,
    return cos(m * M_PI);                                        // 0.5のとき0になるような値.
}

class Surface
{
  public:
    typedef enum
    {
        NONE,       // 空間.
        STANDARD,   // 球面レンズ
        EVENASPH,   // 偶数次非球面,
        CYLINDER_X, // シリンドリカルレンズX.
        CYLINDER_Y, // シリンドリカルレンズY. anamo.
        CYLINDER_Z, // シリンドリカルレンズZ.
    } TYPE;

    static constexpr int N_Aspherical = 8;

    TYPE   type_;
    double center_;                   // 球の中心
    double curve_;                    // 曲率
    double curve2_;                   // 曲率^2
    double radius_;                   // 球の半径
    double radius2_;                  // 球の半径^2
    double diameter_;                 // レンズ半径
    double diam2_;                    // レンズ半径^2
    double thickness_;                // 次の面までの距離.
    double irisX_;                    //絞りサイズ
    double irisY_;                    // 円絞り楕円率.
    double ior_;                      // 媒体屈折率
    double abbeVd_;                   // d線あっべすう
    double reflection_;               // 反射率.
    double conic_;                    // コーニック係数
    double aspherical_[N_Aspherical]; // 非球面パラメータ

    bool   isCoated_;
    bool   isStop_;
    double coatThickness_; // 275nm = 550nmの半波長.
    double coatIor_;       // MgF2で1.38
    double roughness_;

    Surface()
    {
        init();
    }

    void init()
    {
        type_       = NONE;
        center_     = 0.;
        radius_     = 0.;
        radius2_    = 0.;
        diameter_   = 0.;
        diam2_      = 0.;
        irisX_      = 1.;
        irisY_      = 1.;
        thickness_  = 0.;
        roughness_  = 0.; // 面の荒れ.
        ior_        = 1.; // at D light.
        abbeVd_     = 1.; // at D light.
        reflection_ = 0.1;
        isCoated_   = false;
        conic_      = 0.;
    }

    void setup()
    {
        radius2_ = radius_ * radius_;
        diam2_   = diameter_ * diameter_;
        curve2_  = curve_ * curve_;
    }

    double ior(double lambda) const
    {
        // コーシーの式.
        double B   = (ior_ - 1.) / abbeVd_ * 0.52345;
        double A   = ior_ - B / 0.34522792;
        double C   = lambda / 1000.;
        double ret = A + B / (C * C);
        assert(ret == ret);
        return ret;
    }

    double reflection(double lambda, double ior_now, double ior_next, const Vector &dir, const Vector &norm, double &Re, double &Tr)
    {
        if (isCoated_) // シングルコート.
            return single_coat_reflectance(lambda, ior_now, coatIor_, coatThickness_, dir, norm);

        // コーティング無し
        double a  = ior_now - ior_next;
        double b  = ior_now + ior_next;
        double R0 = (a * a) / (b * b);
        double t  = dir.dot(norm);
        double c  = (t < 0.) ? 1.0 + t : 1.0 - t;
        //assert( 0. <= c && c <= 1. );
        Re          = R0 + (1.0 - R0) * pow(c, 5.); // 反射からの寄与.
        double nnt2 = (ior_now / ior_next) * (ior_now / ior_next);
        Tr          = (1. - Re) * nnt2; // 屈折からの寄与.
        return Re;
    }

    // norm : d/dx, d/dy, d/dz
    const double sag(double x, double y, Vector &norm) const
    {
        const double r2 = x * x + y * y;
        if (r2 > diam2_)
            return 0.f;

        //https://forum.zemax.com/12954/Zemax
        //https://www.desmos.com/calculator/wftkimsvv4 :: normal

        const double     r   = sqrt(r2);
        constexpr double eps = 1e-6;

        double rr  = r;
        double rr2 = r2;
        double z   = (curve_ * r2) / (1. + sqrt(1. - (conic_ + 1.) * curve2_ * r2));
        double n   = curve_ * r / sqrt(1. - curve2_ * (1. + conic_) * r2);
        if (type_ == EVENASPH)
        {
            //非球面のときだけ高次もevalする.
            for (int i = 0; i < N_Aspherical; i++)
            {
                z += aspherical_[i] * rr2;
                n += 2. * (i + 1.) * aspherical_[i] * rr;
                rr2 *= r2;
            }
        }
        const double nx = (r < eps) ? 0. : (n * x / r);
        const double ny = (r < eps) ? 0. : (n * y / r);

        norm = Vector(nx, ny, -1.).normal();
        return z;
    }

    const double sag(const Vector &v, Vector &norm) const
    {
        return sag(v.x, v.y, norm);
    }

    // 原点はレンズの中心. x=y=sag=0
    const bool intersect(const Vector &orig, const Vector &dir, double &t, Vector &point, Vector &norm) const
    {
        constexpr double eps = 1e-6;

        // solve equation:
        // orig.z + dir.z * dist == sag( orig.x + dir.x * dist, orig.y + dir.y * dist) for dist.
        double t0   = 0.;
        double t1   = radius_;
        int    iter = 256;

        // initial range
        Vector n0, n1;
        double z0 = sag(orig + dir * t0, n0) - (orig.z + dir.z * t0);
        double z1 = sag(orig + dir * t1, n1) - (orig.z + dir.z * t1);
        bool   s0 = signbit(z0);
        bool   s1 = signbit(z1);

        if (fabs(z0) < eps)
        {
            // converged.
            t     = t0;
            point = Vector(orig.x + dir.x * t, orig.y + dir.y * t, center_ - radius_ + sag(orig + dir * t, norm));
            return true;
        }

        if (s0 == s1)
        {
            // 範囲内に解を持たない.
            return false;
        }

        while (iter-- > 0)
        {
            double tm = (t0 + t1) / 2.;

            Vector       nm;
            const double zm = sag(orig + dir * tm, nm) - (orig.z + dir.z * tm);

            if (fabs(t1 - t0) < eps)
            {
                // converged.
                t     = tm;
                point = Vector(orig.x + dir.x * tm, orig.y + dir.y * tm, center_ - radius_ + sag(orig + dir * tm, norm));
                return true;
            }

            const bool sm = signbit(zm);
            if (sm == s0)
            {
                t0 = tm;
                s0 = sm;
                z0 = zm;
            }
            else
            {
                t1 = tm;
                s1 = sm;
                z1 = zm;
            }
        }
        // not converged.
        return false;
    }
};

typedef std::vector<Surface> SurfaceSet;

class Body
{
  public:
    SurfaceSet surfaces_;
    double     imageSurfaceZ_;
    double     imageSurfaceR_; // 像面高さ.
    double     irisScale_;
    double     maxDiameter_; //最大レンズ半径

    Body()
    {
        surfaces_.clear();
        imageSurfaceZ_ = 100.;
        imageSurfaceR_ = 100.;
        irisScale_     = 1.;
        maxDiameter_   = 0.f;
    }

    void setup(void)
    {
        for (auto &surf : surfaces_)
        {
            surf.setup();
            maxDiameter_ = std::max(maxDiameter_, surf.diameter_);
        }
    }

    double maxDiameter() const { return maxDiameter_; }
    double getImageSurfaceR(void) const { return imageSurfaceR_; }
    void   setImageSurfaceR(double r) { imageSurfaceR_ = r; }
    double getImageSurfaceZ(void) const { return imageSurfaceZ_; }
    void   setImageSurfaceZ(double z) { imageSurfaceZ_ = z; }
    void   setIrisScale(double i) { irisScale_ = i; }

    void dump(void)
    {
        for (int i = 0; i < surfaces_.size(); i++)
        {
            printf("%d type %d %f %f %f %f %f %f %f %f %f\n", i,
                surfaces_[i].type_,
                surfaces_[i].diameter_,
                surfaces_[i].radius_,
                surfaces_[i].center_, surfaces_[i].ior_, surfaces_[i].abbeVd_,
                surfaces_[i].roughness_,
                surfaces_[i].reflection_,
                surfaces_[i].irisX_, surfaces_[i].irisY_);
            if (surfaces_[i].type_ == Surface::EVENASPH)
            {
                printf(" coni %f, ", surfaces_[i].conic_);
                for (int t = 0; t < Surface::N_Aspherical; t++)
                {
                    printf(" %e ", surfaces_[i].aspherical_[t]);
                }
                printf("\n");
            }
        }
    }
};

namespace Loader
{
    namespace ZEMAX
    {
        char *tokenize(char *s, char *token)
        {
            char *p = (char *)s;
            while (*p && (*p == ' ' || *p == '\t'))
                p++;
            if (*p == 0)
                return NULL;
            while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p)
            {
                //printf("%c\n",*p);
                *token = *p;
                token++;
                p++;
            }
            *token = 0;
            if (*p == '\r' || *p == '\n' || *p == 0)
                return NULL;
            return p;
        }

        Body load(const char *filename)
        {
            Body  lens;
            FILE *fp;

            fopen_s(&fp, filename, "rb");
            if (!fp)
            {
                printf("lens %s open fail\n", filename);
                return lens;
            }

            int     surfaceIndex = -1;
            Surface surface;

            bool   isAperture = false;
            double sumz       = 0.;

            while (!feof(fp))
            {
                char line[1024], *p, token[256];

                fgets(line, 1024, fp);
                p = line;
                p = tokenize(p, token);

                if (strcmp(token, "SURF") == 0)
                {
                    if (surfaceIndex >= 0)
                    {
                        if (surface.diameter_ > 0.)
                        {
                            surface.center_ = surface.center_ + surface.radius_; // 中心位置を調整しておく.
                            surface.isStop_ = isAperture;
                            lens.surfaces_.push_back(surface); // レンズフラッシュ
                        }
                    }
                    p            = tokenize(p, token);
                    surfaceIndex = atoi(token);
                    surface.init();
                    isAperture = false;
                }

                if (strcmp(token, "TYPE") == 0)
                {
                    p = tokenize(p, token);
                    if (strcmp(token, "STANDARD") == 0)
                        surface.type_ = Surface::STANDARD;
                    else if (strcmp(token, "EVENASPH") == 0)
                        surface.type_ = Surface::EVENASPH;
                    else
                    {
                        printf("unknown surface: %s\n", token);
                    }
                }
                if (strcmp(token, "STOP") == 0)
                    isAperture = true;

                if (strcmp(token, "CURV") == 0)
                {
                    p              = tokenize(p, token);
                    double curve   = atof(token);
                    surface.curve_ = curve;
                    if (curve != 0.)
                        surface.radius_ = 1. / curve;
                    else
                        surface.radius_ = 0.;
                }

                if (strcmp(token, "DISZ") == 0)
                {
                    p           = tokenize(p, token);
                    double disz = atof(token);
                    if (strcmp(token, "INFINITY") == 0)
                        disz = 0.;
                    surface.thickness_ = disz;
                    surface.center_    = sumz / 2.;
                    sumz += disz;
                }
                if (strcmp(token, "DIAM") == 0)
                {
                    p                 = tokenize(p, token);
                    double d          = strtod(token, NULL);
                    surface.diameter_ = d;
                }

                if (strcmp(token, "PARM") == 0)
                {
                    // evenasph param.
                    p            = tokenize(p, token);
                    int index    = atoi(token);
                    p            = tokenize(p, token);
                    double value = strtod(token, NULL); //atof(token);
                    assert(index >= 1 && index <= 8);
                    //assert(surface.type_ == Surface::EVENASPH);
                    surface.aspherical_[index - 1] = value;
                }
                if (strcmp(token, "CONI") == 0)
                {
                    // evenasph conic
                    p            = tokenize(p, token);
                    double value = atoi(token);
                    //assert(surface.type_ == Surface::EVENASPH);
                    surface.conic_ = value;
                }

                if (strcmp(token, "GLAS") == 0)
                {
                    p = tokenize(p, token); // name
                    p = tokenize(p, token); // nazo1
                    p = tokenize(p, token); // nazo2

                    p               = tokenize(p, token); // ior
                    surface.ior_    = atof(token);
                    p               = tokenize(p, token); // abbe
                    surface.abbeVd_ = atof(token);        // fitting was done in inverted.
                    if (surface.abbeVd_ <= 0.)
                        surface.abbeVd_ = 1.;
                }
            }
            // lens.surfaces_.push_back( surface ); // レンズフラッシュ
            lens.imageSurfaceZ_ = surface.center_;
            fclose(fp);
            lens.setup();
            return lens;
        }

    } // namespace ZEMAX
} // namespace Loader

class Plotter
{
  public:
    FloatCanvas::Canvas canvas_;

    void draw(const Body &body)
    {
        //const float scale         = 16.f;
        const float imageSurfaceZ = (float)body.imageSurfaceZ_;

        const int   width  = 2048; //(int)(imageSurfaceZ * 2.f * scale);
        const float scale  = (float)width / (imageSurfaceZ * 2.f);
        const int   height = (int)(body.maxDiameter() * scale * 2.f) + 256;

        canvas_.setup(width, height);
        canvas_.fill(FloatCanvas::Pixel(0, 0, 0));

        const auto lensX = [scale, width](float x) { return (float)(x - width / 2) / scale; };
        const auto lensY = [scale, height](float y) { return (float)(y - height / 2) / scale; };

        const auto canvasX = [scale, width](float x) { return (float)(x * scale + width / 2); };
        const auto canvasY = [scale, height](float y) { return (float)(y * scale + height / 2); };

        /*constexpr float EPS = 1e-4f;
        assert(fabs(lensX(canvasX(0.f)) - 0.f) < EPS);
        assert(fabs(lensY(canvasY(0.f)) - 0.f) < EPS);
        assert(fabs(canvasX(lensX(0.f)) - 0.f) < EPS);
        assert(fabs(canvasY(lensY(0.f)) - 0.f) < EPS);
        assert(fabs(lensX(canvasX(1.f)) - 1.f) < EPS);
        assert(fabs(lensY(canvasY(1.f)) - 1.f) < EPS);
        assert(fabs(canvasX(lensX(1.f)) - 1.f) < EPS);
        assert(fabs(canvasY(lensY(1.f)) - 1.f) < EPS);*/

        for (const auto &surface : body.surfaces_)
        {
            Vector norm;

            int dia = (int)(surface.diameter_ * scale);

            for (int y = height / 2 - dia; y < height / 2 + dia; y++)
            {
                Vector normal;
                double sag = surface.sag(0.f, lensY((float)y), normal);

                canvas_.putPixel(
                    canvasX((float)(surface.center_ - surface.radius_ + sag)),
                    (float)y,
                    FloatCanvas::Pixel(1, 1, 1));
            }
        }

        for (float y = 0.f; y < body.maxDiameter(); y += 1.f)
        {
            const float cy = canvasY(y); //plotting y
            canvas_.drawLine(
                canvasX(0.f), canvasY(y),
                canvasX(imageSurfaceZ), canvasY(y),
                FloatCanvas::Pixel(0.1f, 0.1f, 0.1f), 1);

            for (const auto &surface : body.surfaces_)
            {
                Vector orig = Vector(0.f, y, 0.f);
                Vector dir  = Vector(0.f, 0.f, 1.f);
                {
                    Vector norm, point;
                    double t;
                    bool   hit = surface.intersect(orig, dir, t, point, norm);
                    if (hit)
                    {
                        canvas_.drawLine(
                            canvasX((float)point.z),
                            canvasY((float)point.y),
                            canvasX((float)(point.z + norm.z)),
                            canvasY((float)(point.y + norm.y)),
                            FloatCanvas::Pixel(1.0f, 0.0f, 0.0f), 1);
                    }
                }
            }
        }
    }
};

} // namespace Lens