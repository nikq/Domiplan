
#define _USE_MATH_DEFINES

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

class Surface
{
  public:
    typedef enum
    {
        NONE,               // 空間.
        APERTURE_HEXAGONAL, // 六角絞り
        APERTURE_CIRCLE,    // 円絞り. xyアスペクトで楕円も.
        STANDARD,           // 球面レンズ
        EVENASPH,           // 偶数次非球面,
        ODDASPH,            // 奇数次非球面
        CYLINDER_X,         // シリンドリカルレンズX.
        CYLINDER_Y,         // シリンドリカルレンズY. anamo.
        CYLINDER_Z,         // シリンドリカルレンズZ.
    } TYPE;

    TYPE   type_;
    double center_;
    double radius_;
    double diameter_;
    double thickness_; // 次の面までの距離.
    double irisX_;
    double irisY_; // 円絞り楕円率.
    double ior_;
    double abbeVd_;
    double reflection_; // 反射率.

    bool   isCoated_;
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
        diameter_   = 0.;
        irisX_      = 1.;
        irisY_      = 1.;
        thickness_  = 0.;
        roughness_  = 0.; // 面の荒れ.
        ior_        = 1.; // at D light.
        abbeVd_     = 1.; // at D light.
        reflection_ = 0.1;
        isCoated_   = false;
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

    // coat_thickness must be in nm.
    // lambda is in nm.
    double single_coat_reflect(double lambda, double ior_in, double coat_ior, double coat_thickness, const Vector &dir, const Vector &norm)
    {
        double cosTheta1     = dir.dot(norm);
        double cosTheta2     = (ior_in / coat_ior) * sqrt((coat_ior * coat_ior) / (ior_in * ior_in) - (1. - cosTheta1 * cosTheta1));
        double distance_diff = 2. * coat_thickness * (coat_ior / ior_in) * cosTheta2;
        double m             = fmod(distance_diff, lambda) / lambda; // ズレ幅0-1,
        return cos(m * M_PI);                                        // 0.5のとき0になるような値.
    }

    double reflection(double lambda, double ior_now, double ior_next, const Vector &dir, const Vector &norm, double &Re, double &Tr)
    {
        if (isCoated_) // シングルコート.
            return single_coat_reflect(lambda, ior_now, coatIor_, coatThickness_, dir, norm);
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
};

typedef std::vector<Surface> SurfaceSet;

class Body
{
  public:
    SurfaceSet surfaces_;
    double     imageSurfaceZ_;
    double     imageSurfaceR_; // 像面高さ.
    double     irisScale_;

    Body()
    {
        surfaces_.clear();
        imageSurfaceZ_ = 100.;
        imageSurfaceR_ = 100.;
        irisScale_     = 1.;
    }

    double getImageSurfaceR(void)
    {
        return imageSurfaceR_;
    }
    void setImageSurfaceR(double r)
    {
        imageSurfaceR_ = r;
    }
    double getImageSurfaceZ(void)
    {
        return imageSurfaceZ_;
    }
    void setImageSurfaceZ(double z)
    {
        imageSurfaceZ_ = z;
    }
    void setIrisScale(double i)
    {
        irisScale_ = i;
    }

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
        }
    }
};

namespace LOADER
{
    namespace LENS
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
    } // namespace LENS

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
                return lens;

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
                            surface.center_ = surface.center_ + surface.radius_;                         // 中心位置を調整しておく.
                            surface.type_   = isAperture ? Surface::APERTURE_CIRCLE : Surface::STANDARD; // STOPは絞り.
                            lens.surfaces_.push_back(surface);                                           // レンズフラッシュ
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
                    else
                    {
                        printf("unknown surface: %s\n", token);
                    }
                }
                if (strcmp(token, "STOP") == 0)
                    isAperture = true;

                if (strcmp(token, "CURV") == 0)
                {
                    p            = tokenize(p, token);
                    double curve = atof(token);
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
                    surface.diameter_ = atof(token);
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
            return lens;
        }

    } // namespace ZEMAX
} // namespace LOADER

} // namespace Lens