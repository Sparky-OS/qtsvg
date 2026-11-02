// Copyright (C) 2026 Daniel Campos Ramos <Capitain_Jack@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// CSS colour values for Qt SVG: the <color> grammar of CSS Color 4 (CRD 1 November 2026) sections 4 to 10,
// CSS Color 5 (WD 29 October 2026) sections 2 to 8 and CSS Color HDR 1 (WD 26 October 2026) sections 4 to 8.
// The conversions port the specifications' own sample code (CSS Color 4 sections 7.1, 7.2, 8.1, 8.2, 19 and 20.3,
// CSS Color HDR section 11, CSS Color 5 section 6.1), which W3C publishes under its software and document licence.
// Gamut mapping is CSS Color 4 section 14.2.2; interpolation CSS Color 4 section 13 and CSS Color 5 section 3.3.

#include "qsvgcsscolor_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qcolorspace.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qimage.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>
#include <QtGui/qstylehints.h>

#include <cmath>
#include <limits>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSvgHandler)

namespace {

using Vec3 = std::array<double, 3>;
using Mat3 = double[3][3];

Vec3 mul(const Mat3 &m, const Vec3 &v)
{
    return {m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2]};
}

double spow(double base, double e)
{
    return std::copysign(std::pow(std::abs(base), e), base);
}

// CSS Color 4 section 19
const double D50[3] = {0.3457 / 0.3585, 1.0, (1.0 - 0.3457 - 0.3585) / 0.3585};

const Mat3 srgbToXyz = {{506752.0 / 1228815, 87881.0 / 245763, 12673.0 / 70218},
                        {87098.0 / 409605, 175762.0 / 245763, 12673.0 / 175545},
                        {7918.0 / 409605, 87881.0 / 737289, 1001167.0 / 1053270}};
const Mat3 xyzToSrgb = {{12831.0 / 3959, -329.0 / 214, -1974.0 / 3959},
                        {-851781.0 / 878810, 1648619.0 / 878810, 36519.0 / 878810},
                        {705.0 / 12673, -2585.0 / 12673, 705.0 / 667}};
const Mat3 p3ToXyz = {{608311.0 / 1250200, 189793.0 / 714400, 198249.0 / 1000160},
                      {35783.0 / 156275, 247089.0 / 357200, 198249.0 / 2500400},
                      {0.0, 32229.0 / 714400, 5220557.0 / 5000800}};
const Mat3 xyzToP3 = {{446124.0 / 178915, -333277.0 / 357830, -72051.0 / 178915},
                      {-14852.0 / 17905, 63121.0 / 35810, 423.0 / 17905},
                      {11844.0 / 330415, -50337.0 / 660830, 316169.0 / 330415}};
const Mat3 prophotoToXyz = {{0.79776664490064230, 0.13518129740053308, 0.03134773412839220},
                            {0.28807482881940130, 0.71183523424187300, 0.00008993693872564},
                            {0.00000000000000000, 0.00000000000000000, 0.82510460251046020}};
const Mat3 xyzToProphoto = {{1.34578688164715830, -0.25557208737979464, -0.05110186497554526},
                            {-0.54463070512490190, 1.50824774284514680, 0.02052744743642139},
                            {0.00000000000000000, 0.00000000000000000, 1.21196754563894520}};
const Mat3 a98ToXyz = {{573536.0 / 994567, 263643.0 / 1420810, 187206.0 / 994567},
                       {591459.0 / 1989134, 6239551.0 / 9945670, 374412.0 / 4972835},
                       {53769.0 / 1989134, 351524.0 / 4972835, 4929758.0 / 4972835}};
const Mat3 xyzToA98 = {{1829569.0 / 896150, -506331.0 / 896150, -308931.0 / 896150},
                       {-851781.0 / 878810, 1648619.0 / 878810, 36519.0 / 878810},
                       {16779.0 / 1248040, -147721.0 / 1248040, 1266979.0 / 1248040}};
const Mat3 rec2020ToXyz = {{63426534.0 / 99577255, 20160776.0 / 139408157, 47086771.0 / 278816314},
                           {26158966.0 / 99577255, 472592308.0 / 697040785, 8267143.0 / 139408157},
                           {0.0, 19567812.0 / 697040785, 295819943.0 / 278816314}};
const Mat3 xyzToRec2020 = {{30757411.0 / 17917100, -6372589.0 / 17917100, -4539589.0 / 17917100},
                           {-19765991.0 / 29648200, 47925759.0 / 29648200, 467509.0 / 29648200},
                           {792561.0 / 44930125, -1921689.0 / 44930125, 42328811.0 / 44930125}};
const Mat3 d65ToD50 = {{1.0479297925449969, 0.022946870601609652, -0.05019226628920524},
                       {0.02962780877005599, 0.9904344267538799, -0.017073799063418826},
                       {-0.009243040646204504, 0.015055191490298152, 0.7518742814281371}};
const Mat3 d50ToD65 = {{0.955473421488075, -0.02309845494876471, 0.06325924320057072},
                       {-0.0283697093338637, 1.0099953980813041, 0.021041441191917323},
                       {0.012314014864481998, -0.020507649298898964, 1.330365926242124}};
const Mat3 xyzToLms = {{0.8190224379967030, 0.3619062600528904, -0.1288737815209879},
                       {0.0329836539323885, 0.9292868615863434, 0.0361446663506424},
                       {0.0481771893596242, 0.2642395317527308, 0.6335478284694309}};
const Mat3 lmsToOklab = {{0.2104542683093140, 0.7936177747023054, -0.0040720430116193},
                         {1.9779985324311684, -2.4285922420485799, 0.4505937096174110},
                         {0.0259040424655478, 0.7827717124575296, -0.8086757549230774}};
const Mat3 lmsToXyz = {{1.2268798758459243, -0.5578149944602171, 0.2813910456659647},
                       {-0.0405757452148008, 1.1122868032803170, -0.0717110580655164},
                       {-0.0763729366746601, -0.4214933324022432, 1.5869240198367816}};
const Mat3 oklabToLms = {{1.0000000000000000, 0.3963377773761749, 0.2158037573099136},
                         {1.0000000000000000, -0.1055613458156586, -0.0638541728258133},
                         {1.0000000000000000, -0.0894841775298119, -1.2914855480194092}};

Vec3 linSrgb(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i) {
        const double a = std::abs(v[i]);
        o[i] = a <= 0.04045 ? v[i] / 12.92 : std::copysign(std::pow((a + 0.055) / 1.055, 2.4), v[i]);
    }
    return o;
}

Vec3 gamSrgb(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i) {
        const double a = std::abs(v[i]);
        o[i] = a > 0.0031308 ? std::copysign(1.055 * std::pow(a, 1 / 2.4) - 0.055, v[i]) : 12.92 * v[i];
    }
    return o;
}

Vec3 linProphoto(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i)
        o[i] = std::abs(v[i]) <= 16.0 / 512 ? v[i] / 16 : spow(v[i], 1.8);
    return o;
}

Vec3 gamProphoto(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i)
        o[i] = std::abs(v[i]) >= 1.0 / 512 ? spow(v[i], 1 / 1.8) : 16 * v[i];
    return o;
}

Vec3 powEach(const Vec3 &v, double e)
{
    return {spow(v[0], e), spow(v[1], e), spow(v[2], e)};
}

Vec3 xyzToLab(const Vec3 &xyzD50)
{
    const double eps = 216.0 / 24389, kappa = 24389.0 / 27;
    double f[3];
    for (int i = 0; i < 3; ++i) {
        const double v = xyzD50[i] / D50[i];
        f[i] = v > eps ? std::cbrt(v) : (kappa * v + 16) / 116;
    }
    return {116 * f[1] - 16, 500 * (f[0] - f[1]), 200 * (f[1] - f[2])};
}

Vec3 labToXyz(const Vec3 &lab)
{
    const double kappa = 24389.0 / 27, eps = 216.0 / 24389;
    const double l116 = lab[0] / 116, a500 = lab[1] / 500, b200 = lab[2] / 200;
    const double f1 = l116 + 16.0 / 116, f0 = f1 + a500, f2 = f1 - b200;
    const Vec3 xyz = {f0 * f0 * f0 > eps ? f0 * f0 * f0 : (l116 + a500) * (116 / kappa),
                      lab[0] > kappa * eps ? std::pow((lab[0] + 16) / 116, 3) : lab[0] / kappa,
                      f2 * f2 * f2 > eps ? f2 * f2 * f2 : (l116 - b200) * (116 / kappa)};
    return {xyz[0] * D50[0], xyz[1] * D50[1], xyz[2] * D50[2]};
}

// rectangular to polar; returns the hue as NaN when powerless (chroma <= epsilon)
Vec3 toPolar(const Vec3 &lab, double epsilon)
{
    const double chroma = std::hypot(lab[1], lab[2]);
    double hue = std::atan2(lab[2], lab[1]) * 180 / M_PI;
    if (hue < 0)
        hue += 360;
    if (chroma <= epsilon)
        hue = std::numeric_limits<double>::quiet_NaN();
    return {lab[0], chroma, hue};
}

Vec3 fromPolar(const Vec3 &lch)
{
    const double h = std::isnan(lch[2]) ? 0.0 : lch[2] * M_PI / 180;
    return {lch[0], lch[1] * std::cos(h), lch[1] * std::sin(h)};
}

Vec3 xyzToOklab(const Vec3 &xyz)
{
    const Vec3 lms = mul(xyzToLms, xyz);
    return mul(lmsToOklab, {std::cbrt(lms[0]), std::cbrt(lms[1]), std::cbrt(lms[2])});
}

Vec3 oklabToXyz(const Vec3 &lab)
{
    const Vec3 lms = mul(oklabToLms, lab);
    return mul(lmsToXyz, {lms[0] * lms[0] * lms[0], lms[1] * lms[1] * lms[1], lms[2] * lms[2] * lms[2]});
}

// CSS Color HDR section 11
const double Yw = 203.0;
const double pqC1 = 3424.0 / 4096, pqC2 = 2413.0 / 128, pqC3 = 2392.0 / 128, pqN = 2610.0 / 16384, pqM = 2523.0 / 32;

Vec3 pqEncode(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i) {
        const double x = std::pow(std::max(v[i] * Yw / 10000, 0.0), pqN);
        o[i] = std::pow((pqC1 + pqC2 * x) / (1 + pqC3 * x), pqM);
    }
    return o;
}

Vec3 pqDecode(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i) {
        const double vm = spow(v[i], 1 / pqM);
        o[i] = std::pow(std::max(vm - pqC1, 0.0) / (pqC2 - pqC3 * vm), 1 / pqN) * 10000 / Yw;
    }
    return o;
}

const double hlgA = 0.17883277, hlgB = 0.28466892, hlgC = 0.55991073, hlgScale = 3.7743;

Vec3 hlgEncode(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i) {
        const double x = v[i] / hlgScale;
        o[i] = x <= 1.0 / 12 ? spow(3 * x, 0.5) : hlgA * std::log(12 * x - hlgB) + hlgC;
    }
    return o;
}

Vec3 hlgDecode(const Vec3 &v)
{
    Vec3 o;
    for (int i = 0; i < 3; ++i)
        o[i] = v[i] <= 0.5 ? v[i] * v[i] / 3 * hlgScale : (std::exp((v[i] - hlgC) / hlgA) + hlgB) / 12 * hlgScale;
    return o;
}

const Mat3 jzM = {{0.41478972, 0.579999, 0.0146480}, {-0.2015100, 1.120649, 0.0531008}, {-0.0166008, 0.264800, 0.6684799}};
const Mat3 jzLmsToIab = {{0.5, 0.5, 0}, {3.524000, -4.066708, 0.542708}, {0.199076, 1.096799, -1.295875}};
const Mat3 jzLmsToXyz = {{1.9242264357876067, -1.0047923125953657, 0.037651404030618},
                         {0.35031676209499907, 0.7264811939316552, -0.06538442294808501},
                         {-0.09098281098284752, -0.3127282905230739, 1.5227665613052603}};
const Mat3 jzIabToLms = {{1, 0.13860504327153927, 0.05804731615611883},
                         {1, -0.1386050432715393, -0.058047316156118904},
                         {1, -0.09601924202631895, -0.81189189605603900}};
const double jzB = 1.15, jzG = 0.66, jzD = -0.56, jzD0 = 1.6295499532821566e-11, jzP = 1.7 * 2523 / 32;

Vec3 xyzToJzazbz(const Vec3 &xyz)
{
    const double Xa = xyz[0] * Yw, Ya = xyz[1] * Yw, Za = xyz[2] * Yw;
    const Vec3 lms = mul(jzM, {jzB * Xa - (jzB - 1) * Za, jzG * Ya - (jzG - 1) * Xa, Za});
    Vec3 pq;
    for (int i = 0; i < 3; ++i) {
        const double x = spow(lms[i] / 10000, pqN);
        pq[i] = spow((pqC1 + pqC2 * x) / (1 + pqC3 * x), jzP);
    }
    const Vec3 iab = mul(jzLmsToIab, pq);
    return {(1 + jzD) * iab[0] / (1 + jzD * iab[0]) - jzD0, iab[1], iab[2]};
}

// The sample code returns absolute XYZ; divided by Yw so that diffuse white is jzazbz(0.22207 ...) (HDR section 6.4).
Vec3 jzazbzToXyz(const Vec3 &j)
{
    const double iz = (j[0] + jzD0) / (1 + jzD - jzD * (j[0] + jzD0));
    const Vec3 pq = mul(jzIabToLms, {iz, j[1], j[2]});
    Vec3 lms;
    for (int i = 0; i < 3; ++i) {
        const double p = spow(pq[i], 1 / jzP);
        lms[i] = 10000 * spow((pqC1 - p) / (pqC3 * p - pqC2), 1 / pqN);
    }
    const Vec3 m = mul(jzLmsToXyz, lms);
    const double Xa = (m[0] + (jzB - 1) * m[2]) / jzB;
    const double Ya = (m[1] + (jzG - 1) * Xa) / jzG;
    return {Xa / Yw, Ya / Yw, m[2] / Yw};
}

const Mat3 ictcpXyzToLms = {{0.3592832590121217, 0.6976051147779502, -0.0358915932320290},
                            {-0.1920808463704993, 1.1004767970374321, 0.0753748658519118},
                            {0.0070797844607479, 0.0748396662186362, 0.8433265453898765}};
const Mat3 ictcpLmsToIctcp = {{2048.0 / 4096, 2048.0 / 4096, 0},
                              {6610.0 / 4096, -13613.0 / 4096, 7003.0 / 4096},
                              {17933.0 / 4096, -17390.0 / 4096, -543.0 / 4096}};
const Mat3 ictcpLmsToXyz = {{2.0701522183894223, -1.3263473389671563, 0.2066510476294053},
                            {0.3647385209748072, 0.6805660249472273, -0.0453045459220347},
                            {-0.0497472075358123, -0.0492609666966131, 1.1880659249923042}};
const Mat3 ictcpToLms = {{0.9999999999999998, 0.0086090370379328, 0.1110296250030260},
                         {0.9999999999999998, -0.0086090370379328, -0.1110296250030259},
                         {0.9999999999999998, 0.5600313357106791, -0.3206271749873188}};

Vec3 xyzToIctcp(const Vec3 &xyz)
{
    const Vec3 lms = mul(ictcpXyzToLms, {xyz[0] * Yw, xyz[1] * Yw, xyz[2] * Yw});
    Vec3 pq;
    for (int i = 0; i < 3; ++i) {
        const double x = std::pow(std::max(lms[i], 0.0) / 10000, pqN);
        pq[i] = std::pow((pqC1 + pqC2 * x) / (1 + pqC3 * x), pqM);
    }
    return mul(ictcpLmsToIctcp, pq);
}

// The sample code returns absolute XYZ; divided by Yw so that ictcp(0.58069 0 0) is diffuse white (HDR section 6.3).
Vec3 ictcpToXyz(const Vec3 &i)
{
    const Vec3 pq = mul(ictcpToLms, i);
    Vec3 lms;
    for (int k = 0; k < 3; ++k) {
        const double vm = spow(pq[k], 1 / pqM);
        lms[k] = 10000 * std::pow(std::max(vm - pqC1, 0.0) / (pqC2 - pqC3 * vm), 1 / pqN);
    }
    const Vec3 x = mul(ictcpLmsToXyz, lms);
    return {x[0] / Yw, x[1] / Yw, x[2] / Yw};
}

// CSS Color 4 sections 7.1, 7.2, 8.1 and 8.2
Vec3 hslToRgb(double hue, double sat, double light)
{
    sat /= 100;
    light /= 100;
    auto f = [&](double n) {
        const double k = std::fmod(n + hue / 30, 12.0);
        const double a = sat * std::min(light, 1 - light);
        return light - a * std::max(-1.0, std::min({k - 3, 9 - k, 1.0}));
    };
    return {f(0), f(8), f(4)};
}

Vec3 rgbToHsl(double r, double g, double b)
{
    const double mx = std::max({r, g, b}), mn = std::min({r, g, b});
    double hue = std::numeric_limits<double>::quiet_NaN(), sat = 0, light = (mn + mx) / 2;
    const double d = mx - mn, epsilon = 1.0 / 100000;
    if (d != 0) {
        sat = (std::abs(light) <= epsilon || std::abs(1 - light) < epsilon) ? 0 : (mx - light) / std::min(light, 1 - light);
        if (mx == r)
            hue = (g - b) / d + (g < b ? 6 : 0);
        else if (mx == g)
            hue = (b - r) / d + 2;
        else
            hue = (r - g) / d + 4;
        hue *= 60;
    }
    if (sat < 0) {
        hue = (std::isnan(hue) ? 0 : hue) + 180;
        sat = std::abs(sat);
    }
    if (!std::isnan(hue) && hue >= 360)
        hue -= 360;
    if (sat <= epsilon) {
        hue = std::numeric_limits<double>::quiet_NaN();
        sat = 0;
    }
    return {hue, sat * 100, light * 100};
}

Vec3 hwbToRgb(double hue, double white, double black)
{
    white /= 100;
    black /= 100;
    if (white + black >= 1) {
        const double gray = white / (white + black);
        return {gray, gray, gray};
    }
    Vec3 rgb = hslToRgb(hue, 100, 50);
    for (double &c : rgb)
        c = c * (1 - white - black) + white;
    return rgb;
}

Vec3 rgbToHwb(double r, double g, double b)
{
    const double mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    double hue = std::numeric_limits<double>::quiet_NaN();
    if (d != 0) {
        if (mx == r)
            hue = (g - b) / d + (g < b ? 6 : 0);
        else if (mx == g)
            hue = (b - r) / d + 2;
        else
            hue = (r - g) / d + 4;
        hue *= 60;
    }
    if (!std::isnan(hue) && hue >= 360)
        hue -= 360;
    const double white = mn, black = 1 - mx;
    if (white + black >= 1 - 1.0 / 100000)
        hue = std::numeric_limits<double>::quiet_NaN();
    return {hue, white * 100, black * 100};
}

double positiveMod(double a, double m)
{
    const double r = std::fmod(a, m);
    return r < 0 ? r + m : r;
}

using S = QSvgCssColor;

Vec3 toXyz(S::Space space, const Vec3 &c)
{
    switch (space) {
    case S::Srgb: return mul(srgbToXyz, linSrgb(c));
    case S::SrgbLinear: return mul(srgbToXyz, c);
    case S::DisplayP3: return mul(p3ToXyz, linSrgb(c));
    case S::DisplayP3Linear: return mul(p3ToXyz, c);
    case S::A98Rgb: return mul(a98ToXyz, powEach(c, 563.0 / 256));
    case S::ProphotoRgb: return mul(d50ToD65, mul(prophotoToXyz, linProphoto(c)));
    case S::Rec2020: return mul(rec2020ToXyz, powEach(c, 2.4));
    case S::Rec2100Linear: return mul(rec2020ToXyz, c);
    case S::Rec2100Pq: return mul(rec2020ToXyz, pqDecode(c));
    case S::Rec2100Hlg: return mul(rec2020ToXyz, hlgDecode(c));
    case S::XyzD65: return c;
    case S::XyzD50: return mul(d50ToD65, c);
    case S::Hsl: return toXyz(S::Srgb, hslToRgb(positiveMod(c[0], 360), c[1], c[2]));
    case S::Hwb: return toXyz(S::Srgb, hwbToRgb(positiveMod(c[0], 360), c[1], c[2]));
    case S::Lab: return mul(d50ToD65, labToXyz(c));
    case S::Lch: return mul(d50ToD65, labToXyz(fromPolar(c)));
    case S::Oklab: return oklabToXyz(c);
    case S::Oklch: return oklabToXyz(fromPolar(c));
    case S::Jzazbz: return jzazbzToXyz(c);
    case S::Jzczhz: return jzazbzToXyz(fromPolar(c));
    case S::Ictcp: return ictcpToXyz(c);
    case S::Invalid: break;
    }
    return {0, 0, 0};
}

Vec3 fromXyz(S::Space space, const Vec3 &xyz)
{
    switch (space) {
    case S::Srgb: return gamSrgb(mul(xyzToSrgb, xyz));
    case S::SrgbLinear: return mul(xyzToSrgb, xyz);
    case S::DisplayP3: return gamSrgb(mul(xyzToP3, xyz));
    case S::DisplayP3Linear: return mul(xyzToP3, xyz);
    case S::A98Rgb: return powEach(mul(xyzToA98, xyz), 256.0 / 563);
    case S::ProphotoRgb: return gamProphoto(mul(xyzToProphoto, mul(d65ToD50, xyz)));
    case S::Rec2020: return powEach(mul(xyzToRec2020, xyz), 1 / 2.4);
    case S::Rec2100Linear: return mul(xyzToRec2020, xyz);
    case S::Rec2100Pq: return pqEncode(mul(xyzToRec2020, xyz));
    case S::Rec2100Hlg: return hlgEncode(mul(xyzToRec2020, xyz));
    case S::XyzD65: return xyz;
    case S::XyzD50: return mul(d65ToD50, xyz);
    case S::Hsl: { const Vec3 r = fromXyz(S::Srgb, xyz); return rgbToHsl(r[0], r[1], r[2]); }
    case S::Hwb: { const Vec3 r = fromXyz(S::Srgb, xyz); return rgbToHwb(r[0], r[1], r[2]); }
    case S::Lab: return xyzToLab(mul(d65ToD50, xyz));
    case S::Lch: return toPolar(xyzToLab(mul(d65ToD50, xyz)), 0.0015);
    case S::Oklab: return xyzToOklab(xyz);
    case S::Oklch: return toPolar(xyzToOklab(xyz), 0.000004);
    case S::Jzazbz: return xyzToJzazbz(xyz);
    case S::Jzczhz: return toPolar(xyzToJzazbz(xyz), 0.0000026);
    case S::Ictcp: return xyzToIctcp(xyz);
    case S::Invalid: break;
    }
    return {0, 0, 0};
}

int hueIndex(S::Space space)
{
    switch (space) {
    case S::Hsl: case S::Hwb: return 0;
    case S::Lch: case S::Oklch: case S::Jzczhz: return 2;
    default: return -1;
    }
}

// CSS Color 4 section 13.3: analogous component categories (0 none, then Reds, Greens, Blues, Lightness,
// Colorfulness, Hue, Opponent a, Opponent b); CSS Color HDR names no analogs for ICtCp and Jzazbz.
enum Analog : quint8 { NoAnalog, Reds, Greens, Blues, Lightness, Colorfulness, HueAngle, OpponentA, OpponentB };

std::array<Analog, 3> analogs(S::Space space)
{
    if (S::isRgbSpace(space) || space == S::XyzD50 || space == S::XyzD65)
        return {Reds, Greens, Blues};
    switch (space) {
    case S::Hsl: return {HueAngle, Colorfulness, Lightness};
    case S::Hwb: return {HueAngle, NoAnalog, NoAnalog};
    case S::Lab: case S::Oklab: return {Lightness, OpponentA, OpponentB};
    case S::Lch: case S::Oklch: return {Lightness, Colorfulness, HueAngle};
    default: return {NoAnalog, NoAnalog, NoAnalog};
    }
}

// CSS Color 4 section 19, powerlessToMissing (and CSS Color HDR section 6.5 for jzczhz)
void powerlessToMissing(S::Space space, Vec3 &c, bool missing[3])
{
    switch (space) {
    case S::Hsl: {
        const double sat = missing[1] ? 0 : c[1];
        if (sat <= 0.001) {
            missing[0] = true;
            if (!missing[1] && c[1] > 0)
                c[1] = 0;
        }
        break;
    }
    case S::Hwb: {
        const double w = missing[1] ? 0 : c[1], b = missing[2] ? 0 : c[2];
        if (w + b >= 99.999) {
            missing[0] = true;
            if (w + b < 100) {
                if (missing[1]) {
                    c[2] = 100;
                } else if (missing[2]) {
                    c[1] = 100;
                } else {
                    c[2] = 100 - c[1];
                }
            }
        }
        break;
    }
    case S::Lch: case S::Oklch: case S::Jzczhz: {
        const double eps = space == S::Lch ? 0.0015 : space == S::Oklch ? 0.000004 : 0.0000026;
        const double ch = missing[1] ? 0 : c[1];
        if (ch <= eps) {
            missing[2] = true;
            if (!missing[1] && c[1] > 0)
                c[1] = 0;
        }
        break;
    }
    default:
        break;
    }
}

} // namespace

const char *QSvgCssColor::name(Space space)
{
    static const char *const names[] = {"invalid", "srgb", "srgb-linear", "display-p3", "display-p3-linear", "a98-rgb",
                                        "prophoto-rgb", "rec2020", "rec2100-pq", "rec2100-hlg", "rec2100-linear",
                                        "xyz-d50", "xyz-d65", "hsl", "hwb", "lab", "lch", "oklab", "oklch", "ictcp",
                                        "jzazbz", "jzczhz"};
    return names[space];
}

std::array<double, 3> QSvgCssColor::toXyzD65() const
{
    Vec3 c = {m_c[0], m_c[1], m_c[2]};
    for (int i = 0; i < 3; ++i) {
        if (isMissing(i) || std::isnan(c[i]))
            c[i] = 0;
    }
    return toXyz(m_space, c);
}

QSvgCssColor QSvgCssColor::fromXyzD65(Space space, const std::array<double, 3> &xyz, double alpha)
{
    Vec3 c = fromXyz(space, xyz);
    bool missing[3] = {false, false, false};
    if (isPolar(space))
        powerlessToMissing(space, c, missing);
    QSvgCssColor out(space, 0, 0, 0, alpha);
    for (int i = 0; i < 3; ++i) {
        if (missing[i] || std::isnan(c[i]))
            out.setMissing(i);
        else
            out.setComponent(i, c[i]);
    }
    return out;
}

QSvgCssColor QSvgCssColor::convertedTo(Space space) const
{
    if (space == m_space || !isValid())
        return *this;
    Vec3 src = {m_c[0], m_c[1], m_c[2]};
    bool srcMissing[3] = {isMissing(0), isMissing(1), isMissing(2)};
    powerlessToMissing(m_space, src, srcMissing);
    Vec3 z = src;
    for (int i = 0; i < 3; ++i) {
        if (srcMissing[i] || std::isnan(z[i]))
            z[i] = 0;
    }
    QSvgCssColor out = fromXyzD65(space, toXyz(m_space, z), m_alpha);
    if (isAlphaMissing())
        out.setAlphaMissing();
    // CSS Color 4 section 13.3: analogous components missing in the source stay missing, and so does the
    // destination's analogous set when every component of the source's set is missing
    const auto sa = analogs(m_space), da = analogs(space);
    bool shared[9] = {};
    for (Analog a : sa) {
        for (Analog b : da) {
            if (a != NoAnalog && a == b)
                shared[a] = true;
        }
    }
    bool allSetMissing = true, haveSet = false, haveDstSet = false;
    for (int i = 0; i < 3; ++i) {
        if (!shared[sa[i]] || sa[i] == NoAnalog) {
            haveSet = true;
            allSetMissing = allSetMissing && srcMissing[i];
        } else if (srcMissing[i]) {
            for (int j = 0; j < 3; ++j) {
                if (da[j] == sa[i])
                    out.setMissing(j);
            }
        }
        if (!shared[da[i]] || da[i] == NoAnalog)
            haveDstSet = true;
    }
    if (haveSet && haveDstSet && allSetMissing) {
        for (int j = 0; j < 3; ++j) {
            if (!shared[da[j]] || da[j] == NoAnalog)
                out.setMissing(j);
        }
    }
    return out;
}

QSvgCssColor QSvgCssColor::fromQColor(const QColor &color)
{
    const QColor c = color.spec() == QColor::ExtendedRgb ? color : color.toRgb();
    QSvgCssColor out(Srgb, c.redF(), c.greenF(), c.blueF(), c.alphaF());
    if (color.spec() == QColor::Rgb) {
        const QRgba64 v = color.rgba64();
        out = QSvgCssColor(Srgb, v.red() / 65535.0, v.green() / 65535.0, v.blue() / 65535.0, v.alpha() / 65535.0);
    }
    out.setLegacy(true);
    return out;
}

QColor QSvgCssColor::toQColor() const
{
    return qsvg_toQColor(qsvg_resolveForTarget(*this, QSvgCssColorTarget()));
}

// ---------------------------------------------------------------- parsing

namespace {

enum class TokKind : quint8 { Eof, Ws, Ident, Function, Number, Percentage, Dimension, Hash, Comma, Delim, LParen, RParen };

struct Tok
{
    TokKind kind = TokKind::Eof;
    double value = 0;
    QString text;   // ident, function name (lower case), hash, unit, delim
    QChar sign;     // the number's sign as written, for the whitespace rule of calc()
};

bool isNameStart(QChar c) { return c.isLetter() || c == u'_' || c.unicode() >= 0x80; }
bool isNameChar(QChar c) { return isNameStart(c) || c.isDigit() || c == u'-'; }

QList<Tok> tokenize(QStringView s)
{
    QList<Tok> out;
    qsizetype i = 0;
    const qsizetype n = s.size();
    auto startsNumber = [&](qsizetype k) {
        if (k >= n)
            return false;
        const QChar c = s[k];
        if (c.isDigit())
            return true;
        if (c == u'.')
            return k + 1 < n && s[k + 1].isDigit();
        if (c == u'+' || c == u'-')
            return k + 1 < n && (s[k + 1].isDigit() || (s[k + 1] == u'.' && k + 2 < n && s[k + 2].isDigit()));
        return false;
    };
    auto startsIdent = [&](qsizetype k) {
        if (k >= n)
            return false;
        if (isNameStart(s[k]))
            return true;
        if (s[k] == u'-' && k + 1 < n)
            return isNameStart(s[k + 1]) || s[k + 1] == u'-';
        return false;
    };
    while (i < n) {
        const QChar c = s[i];
        Tok t;
        if (c.isSpace()) {
            while (i < n && s[i].isSpace())
                ++i;
            t.kind = TokKind::Ws;
            out.append(t);
            continue;
        }
        if (c == u'/' && i + 1 < n && s[i + 1] == u'*') {
            const qsizetype end = s.indexOf(u"*/", i + 2);
            i = end < 0 ? n : end + 2;
            continue;
        }
        if (startsNumber(i)) {
            const qsizetype start = i;
            if (s[i] == u'+' || s[i] == u'-') {
                t.sign = s[i];
                ++i;
            }
            while (i < n && s[i].isDigit())
                ++i;
            if (i + 1 < n && s[i] == u'.' && s[i + 1].isDigit()) {
                ++i;
                while (i < n && s[i].isDigit())
                    ++i;
            }
            if (i + 1 < n && (s[i] == u'e' || s[i] == u'E')) {
                qsizetype k = i + 1;
                if (k < n && (s[k] == u'+' || s[k] == u'-'))
                    ++k;
                if (k < n && s[k].isDigit()) {
                    i = k;
                    while (i < n && s[i].isDigit())
                        ++i;
                }
            }
            t.value = s.sliced(start, i - start).toDouble();
            if (i < n && s[i] == u'%') {
                t.kind = TokKind::Percentage;
                ++i;
            } else if (startsIdent(i)) {
                const qsizetype us = i;
                while (i < n && isNameChar(s[i]))
                    ++i;
                t.kind = TokKind::Dimension;
                t.text = s.sliced(us, i - us).toString().toLower();
            } else {
                t.kind = TokKind::Number;
            }
            out.append(t);
            continue;
        }
        if (c == u'#') {
            qsizetype k = i + 1;
            while (k < n && isNameChar(s[k]))
                ++k;
            if (k > i + 1) {
                t.kind = TokKind::Hash;
                t.text = s.sliced(i + 1, k - i - 1).toString();
                out.append(t);
                i = k;
                continue;
            }
        }
        if (startsIdent(i)) {
            const qsizetype start = i;
            while (i < n && isNameChar(s[i]))
                ++i;
            t.text = s.sliced(start, i - start).toString().toLower();
            if (i < n && s[i] == u'(') {
                t.kind = TokKind::Function;
                ++i;
            } else {
                t.kind = TokKind::Ident;
            }
            out.append(t);
            continue;
        }
        t.kind = c == u'(' ? TokKind::LParen : c == u')' ? TokKind::RParen : c == u',' ? TokKind::Comma : TokKind::Delim;
        t.text = QString(c);
        out.append(t);
        ++i;
    }
    return out;
}

// a calculated value (CSS Values 4 section 10): its canonical value and type
enum class Unit : quint8 { Number, Percentage, Angle };
struct Num
{
    double v;
    Unit t;
};

struct NamedColor
{
    const char *name;
    quint32 rgb;
};

// CSS Color 4 section 6.1
const NamedColor namedColors[] = {
    {"aliceblue", 0xf0f8ff}, {"antiquewhite", 0xfaebd7}, {"aqua", 0x00ffff}, {"aquamarine", 0x7fffd4}, {"azure", 0xf0ffff},
    {"beige", 0xf5f5dc}, {"bisque", 0xffe4c4}, {"black", 0x000000}, {"blanchedalmond", 0xffebcd}, {"blue", 0x0000ff},
    {"blueviolet", 0x8a2be2}, {"brown", 0xa52a2a}, {"burlywood", 0xdeb887}, {"cadetblue", 0x5f9ea0}, {"chartreuse", 0x7fff00},
    {"chocolate", 0xd2691e}, {"coral", 0xff7f50}, {"cornflowerblue", 0x6495ed}, {"cornsilk", 0xfff8dc}, {"crimson", 0xdc143c},
    {"cyan", 0x00ffff}, {"darkblue", 0x00008b}, {"darkcyan", 0x008b8b}, {"darkgoldenrod", 0xb8860b}, {"darkgray", 0xa9a9a9},
    {"darkgreen", 0x006400}, {"darkgrey", 0xa9a9a9}, {"darkkhaki", 0xbdb76b}, {"darkmagenta", 0x8b008b},
    {"darkolivegreen", 0x556b2f}, {"darkorange", 0xff8c00}, {"darkorchid", 0x9932cc}, {"darkred", 0x8b0000},
    {"darksalmon", 0xe9967a}, {"darkseagreen", 0x8fbc8f}, {"darkslateblue", 0x483d8b}, {"darkslategray", 0x2f4f4f},
    {"darkslategrey", 0x2f4f4f}, {"darkturquoise", 0x00ced1}, {"darkviolet", 0x9400d3}, {"deeppink", 0xff1493},
    {"deepskyblue", 0x00bfff}, {"dimgray", 0x696969}, {"dimgrey", 0x696969}, {"dodgerblue", 0x1e90ff},
    {"firebrick", 0xb22222}, {"floralwhite", 0xfffaf0}, {"forestgreen", 0x228b22}, {"fuchsia", 0xff00ff},
    {"gainsboro", 0xdcdcdc}, {"ghostwhite", 0xf8f8ff}, {"gold", 0xffd700}, {"goldenrod", 0xdaa520}, {"gray", 0x808080},
    {"green", 0x008000}, {"greenyellow", 0xadff2f}, {"grey", 0x808080}, {"honeydew", 0xf0fff0}, {"hotpink", 0xff69b4},
    {"indianred", 0xcd5c5c}, {"indigo", 0x4b0082}, {"ivory", 0xfffff0}, {"khaki", 0xf0e68c}, {"lavender", 0xe6e6fa},
    {"lavenderblush", 0xfff0f5}, {"lawngreen", 0x7cfc00}, {"lemonchiffon", 0xfffacd}, {"lightblue", 0xadd8e6},
    {"lightcoral", 0xf08080}, {"lightcyan", 0xe0ffff}, {"lightgoldenrodyellow", 0xfafad2}, {"lightgray", 0xd3d3d3},
    {"lightgreen", 0x90ee90}, {"lightgrey", 0xd3d3d3}, {"lightpink", 0xffb6c1}, {"lightsalmon", 0xffa07a},
    {"lightseagreen", 0x20b2aa}, {"lightskyblue", 0x87cefa}, {"lightslategray", 0x778899}, {"lightslategrey", 0x778899},
    {"lightsteelblue", 0xb0c4de}, {"lightyellow", 0xffffe0}, {"lime", 0x00ff00}, {"limegreen", 0x32cd32},
    {"linen", 0xfaf0e6}, {"magenta", 0xff00ff}, {"maroon", 0x800000}, {"mediumaquamarine", 0x66cdaa},
    {"mediumblue", 0x0000cd}, {"mediumorchid", 0xba55d3}, {"mediumpurple", 0x9370db}, {"mediumseagreen", 0x3cb371},
    {"mediumslateblue", 0x7b68ee}, {"mediumspringgreen", 0x00fa9a}, {"mediumturquoise", 0x48d1cc},
    {"mediumvioletred", 0xc71585}, {"midnightblue", 0x191970}, {"mintcream", 0xf5fffa}, {"mistyrose", 0xffe4e1},
    {"moccasin", 0xffe4b5}, {"navajowhite", 0xffdead}, {"navy", 0x000080}, {"oldlace", 0xfdf5e6}, {"olive", 0x808000},
    {"olivedrab", 0x6b8e23}, {"orange", 0xffa500}, {"orangered", 0xff4500}, {"orchid", 0xda70d6},
    {"palegoldenrod", 0xeee8aa}, {"palegreen", 0x98fb98}, {"paleturquoise", 0xafeeee}, {"palevioletred", 0xdb7093},
    {"papayawhip", 0xffefd5}, {"peachpuff", 0xffdab9}, {"peru", 0xcd853f}, {"pink", 0xffc0cb}, {"plum", 0xdda0dd},
    {"powderblue", 0xb0e0e6}, {"purple", 0x800080}, {"rebeccapurple", 0x663399}, {"red", 0xff0000},
    {"rosybrown", 0xbc8f8f}, {"royalblue", 0x4169e1}, {"saddlebrown", 0x8b4513}, {"salmon", 0xfa8072},
    {"sandybrown", 0xf4a460}, {"seagreen", 0x2e8b57}, {"seashell", 0xfff5ee}, {"sienna", 0xa0522d}, {"silver", 0xc0c0c0},
    {"skyblue", 0x87ceeb}, {"slateblue", 0x6a5acd}, {"slategray", 0x708090}, {"slategrey", 0x708090}, {"snow", 0xfffafa},
    {"springgreen", 0x00ff7f}, {"steelblue", 0x4682b4}, {"tan", 0xd2b48c}, {"teal", 0x008080}, {"thistle", 0xd8bfd8},
    {"tomato", 0xff6347}, {"turquoise", 0x40e0d0}, {"violet", 0xee82ee}, {"wheat", 0xf5deb3}, {"white", 0xffffff},
    {"whitesmoke", 0xf5f5f5}, {"yellow", 0xffff00}, {"yellowgreen", 0x9acd32},
};

S rgb8(quint32 v, double alpha = 1)
{
    S c(S::Srgb, ((v >> 16) & 255) / 255.0, ((v >> 8) & 255) / 255.0, (v & 255) / 255.0, alpha);
    c.setLegacy(true);
    return c;
}

// CSS Color 4 section 6.2 and Appendix A: the system colours, from the application palette
bool systemColor(const QString &name, S *out)
{
    static const struct { const char *css; const char *as; } deprecated[] = {
        {"activeborder", "buttonborder"}, {"activecaption", "canvas"}, {"appworkspace", "canvas"},
        {"background", "canvas"}, {"buttonhighlight", "buttonface"}, {"buttonshadow", "buttonface"},
        {"captiontext", "canvastext"}, {"inactiveborder", "buttonborder"}, {"inactivecaption", "canvas"},
        {"inactivecaptiontext", "graytext"}, {"infobackground", "canvas"}, {"infotext", "canvastext"},
        {"menu", "canvas"}, {"menutext", "canvastext"}, {"scrollbar", "canvas"}, {"threeddarkshadow", "buttonborder"},
        {"threedface", "buttonface"}, {"threedhighlight", "buttonborder"}, {"threedlightshadow", "buttonborder"},
        {"threedshadow", "buttonborder"}, {"window", "canvas"}, {"windowframe", "buttonborder"},
        {"windowtext", "canvastext"}};
    QString key = name;
    for (const auto &d : deprecated) {
        if (key == QLatin1String(d.css)) {
            key = QLatin1String(d.as);
            break;
        }
    }
    static const struct { const char *css; QPalette::ColorGroup group; QPalette::ColorRole role; } roles[] = {
        {"accentcolor", QPalette::Active, QPalette::Accent}, {"accentcolortext", QPalette::Active, QPalette::HighlightedText},
        {"activetext", QPalette::Active, QPalette::Link}, {"buttonborder", QPalette::Active, QPalette::Mid},
        {"buttonface", QPalette::Active, QPalette::Button}, {"buttontext", QPalette::Active, QPalette::ButtonText},
        {"canvas", QPalette::Active, QPalette::Base}, {"canvastext", QPalette::Active, QPalette::Text},
        {"field", QPalette::Active, QPalette::Base}, {"fieldtext", QPalette::Active, QPalette::Text},
        {"graytext", QPalette::Disabled, QPalette::Text}, {"highlight", QPalette::Active, QPalette::Highlight},
        {"highlighttext", QPalette::Active, QPalette::HighlightedText}, {"linktext", QPalette::Active, QPalette::Link},
        {"mark", QPalette::Active, QPalette::Highlight}, {"marktext", QPalette::Active, QPalette::HighlightedText},
        {"selecteditem", QPalette::Active, QPalette::Highlight},
        {"selecteditemtext", QPalette::Active, QPalette::HighlightedText},
        {"visitedtext", QPalette::Active, QPalette::LinkVisited}};
    for (const auto &r : roles) {
        if (key == QLatin1String(r.css)) {
            const QPalette pal = qobject_cast<QGuiApplication *>(QCoreApplication::instance())
                    ? QGuiApplication::palette() : QPalette();
            *out = S::fromQColor(pal.color(r.group, r.role));
            return true;
        }
    }
    return false;
}

const char *const mathFunctions[] = {"calc", "min", "max", "clamp", "round", "mod", "rem", "sin", "cos", "tan", "asin",
                                     "acos", "atan", "atan2", "pow", "sqrt", "hypot", "log", "exp", "abs", "sign"};

bool isMathFunction(const QString &name)
{
    for (const char *f : mathFunctions) {
        if (name == QLatin1String(f))
            return true;
    }
    return false;
}

struct Channels
{
    QVarLengthArray<std::pair<QString, double>, 4> values;   // NaN for a missing (none) component
    bool has(const QString &n) const
    {
        for (const auto &v : values) {
            if (v.first == n)
                return true;
        }
        return false;
    }
    double get(const QString &n) const
    {
        for (const auto &v : values) {
            if (v.first == n)
                return v.second;
        }
        return 0;
    }
};

// a parsed component: value (NaN for none) and its kind
struct Comp
{
    double v;
    Unit t;
    bool none;
};

S interpolate(const S &a, const S &b, double p, S::Space space, int hueMethod);

class Parser
{
public:
    Parser(QList<Tok> toks, const QSvgCssColorContext &ctx) : m_t(std::move(toks)), m_ctx(ctx) {}

    S parseWhole()
    {
        S c = color();
        ws();
        if (!m_failed && m_i < m_t.size())
            fail(QStringLiteral("unexpected text after the colour"));
        return m_failed ? S() : c;
    }

    bool usesCurrentColor = false;

private:
    QList<Tok> m_t;
    qsizetype m_i = 0;
    qsizetype m_argCount = 0;
    const QSvgCssColorContext &m_ctx;
    static inline const Tok eof{};

    // Qt modules are built without exceptions: the first error is kept and the input then reads as ended,
    // so that every rule returns at once
    void fail(const QString &why)
    {
        if (!m_failed) {
            m_failed = true;
            m_error = why;
        }
    }
    const Tok &peek() const { return (!m_failed && m_i < m_t.size()) ? m_t.at(m_i) : eof; }

    // a speculative parse: on failure the position and the error state are restored
    template <typename F>
    bool attempt(F &&f)
    {
        const qsizetype save = m_i;
        f();
        if (!m_failed)
            return true;
        m_failed = false;
        m_error.clear();
        m_i = save;
        return false;
    }

public:
    bool m_failed = false;
    QString m_error;

private:
    const Tok &next()
    {
        const Tok &t = peek();
        ++m_i;
        return t;
    }
    void ws()
    {
        while (peek().kind == TokKind::Ws)
            ++m_i;
    }
    bool at(TokKind k)
    {
        ws();
        return peek().kind == k;
    }
    bool atIdent(const char *name)
    {
        ws();
        return peek().kind == TokKind::Ident && peek().text == QLatin1String(name);
    }
    bool atDelim(QChar c)
    {
        ws();
        return peek().kind == TokKind::Delim && peek().text.at(0) == c;
    }
    void expect(TokKind k, const char *what)
    {
        ws();
        if (next().kind != k)
            fail(QStringLiteral("expected %1").arg(QLatin1String(what)));
    }

    // ---- math functions (CSS Values 4 section 10), with channel keywords as numbers
    Num mathValue(const Channels *ch)
    {
        ws();
        const Tok t = next();
        switch (t.kind) {
        case TokKind::Number: return {t.value, Unit::Number};
        case TokKind::Percentage: return {t.value, Unit::Percentage};
        case TokKind::Dimension: return angle(t);
        case TokKind::Ident: {
            if (ch && ch->has(t.text)) {
                const double v = ch->get(t.text);
                return {std::isnan(v) ? 0.0 : v, Unit::Number};
            }
            if (t.text == u"e") return {M_E, Unit::Number};
            if (t.text == u"pi") return {M_PI, Unit::Number};
            if (t.text == u"infinity") return {std::numeric_limits<double>::infinity(), Unit::Number};
            if (t.text == u"-infinity") return {-std::numeric_limits<double>::infinity(), Unit::Number};
            if (t.text == u"nan") return {std::numeric_limits<double>::quiet_NaN(), Unit::Number};
            fail(QStringLiteral("unknown keyword %1 in a math function").arg(t.text));
            return {0, Unit::Number};
        }
        case TokKind::LParen: {
            const Num v = mathSum(ch);
            expect(TokKind::RParen, "')'");
            return v;
        }
        case TokKind::Function:
            if (isMathFunction(t.text))
                return mathFunction(t.text, ch);
            break;
        default:
            break;
        }
        fail(QStringLiteral("not a math value"));
        return {0, Unit::Number};
    }

    Num angle(const Tok &t)
    {
        double f;
        if (t.text == u"deg")
            f = 1;
        else if (t.text == u"grad")
            f = 0.9;
        else if (t.text == u"rad")
            f = 180 / M_PI;
        else if (t.text == u"turn")
            f = 360;
        else {
            fail(QStringLiteral("unit %1 is not an angle").arg(t.text));
            return {0, Unit::Angle};
        }
        return {t.value * f, Unit::Angle};
    }

    Num mathProduct(const Channels *ch)
    {
        Num v = mathValue(ch);
        for (;;) {
            const qsizetype save = m_i;
            ws();
            if (peek().kind == TokKind::Delim && (peek().text == u"*" || peek().text == u"/")) {
                const bool times = next().text == u"*";
                const Num w = mathValue(ch);
                if (times) {
                    if (v.t != Unit::Number && w.t != Unit::Number)
                        fail(QStringLiteral("a product of two dimensions"));
                    v = {v.v * w.v, v.t == Unit::Number ? w.t : v.t};
                } else {
                    if (w.t != Unit::Number && w.t != v.t)
                        fail(QStringLiteral("a division by a dimension"));
                    v = {v.v / w.v, w.t == v.t ? Unit::Number : v.t};
                }
            } else {
                m_i = save;
                return v;
            }
        }
    }

    Num mathSum(const Channels *ch)
    {
        Num v = mathProduct(ch);
        for (;;) {
            const qsizetype save = m_i;
            const bool hadWs = peek().kind == TokKind::Ws;
            ws();
            const Tok &t = peek();
            if (t.kind == TokKind::Delim && (t.text == u"+" || t.text == u"-") && hadWs
                && m_i + 1 < m_t.size() && m_t.at(m_i + 1).kind == TokKind::Ws) {
                const bool plus = next().text == u"+";
                const Num w = mathProduct(ch);
                if (w.t != v.t)
                    fail(QStringLiteral("a sum of different types"));
                v = {plus ? v.v + w.v : v.v - w.v, v.t};
            } else if ((t.kind == TokKind::Number || t.kind == TokKind::Percentage || t.kind == TokKind::Dimension)
                       && hadWs && !t.sign.isNull()) {
                fail(QStringLiteral("+ and - need whitespace on both sides"));
            } else {
                m_i = save;
                return v;
            }
        }
    }

    QVarLengthArray<Num, 4> mathArgs(const Channels *ch)
    {
        QVarLengthArray<Num, 4> args;
        args.append(mathSum(ch));
        while (at(TokKind::Comma)) {
            next();
            args.append(mathSum(ch));
        }
        expect(TokKind::RParen, "')'");
        m_argCount = args.size();
        while (args.size() < 3)   // so that an incomplete call reads defined values while the parse unwinds
            args.append({0, args[0].t});
        return args;
    }

    Num mathFunction(const QString &name, const Channels *ch)
    {
        auto same = [&](const QVarLengthArray<Num, 4> &list) {
            for (const Num &x : list) {
                if (x.t != list[0].t)
                    fail(QStringLiteral("%1() of different types").arg(name));
            }
            return list[0].t;
        };
        auto radians = [&](const Num &x) {
            if (x.t == Unit::Percentage)
                fail(QStringLiteral("trigonometry of a percentage"));
            return x.t == Unit::Angle ? x.v * M_PI / 180 : x.v;
        };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        if (name == u"round") {
            int strategy = 0;   // nearest, up, down, to-zero
            ws();
            if (peek().kind == TokKind::Ident) {
                const QString s = peek().text;
                if (s == u"nearest" || s == u"up" || s == u"down" || s == u"to-zero") {
                    next();
                    strategy = s == u"nearest" ? 0 : s == u"up" ? 1 : s == u"down" ? 2 : 3;
                    expect(TokKind::Comma, "','");
                }
            }
            const auto a = mathArgs(ch);
            const double b = m_argCount > 1 ? a[1].v : 1.0;
            if (b == 0)
                return {nan, a[0].t};
            const double q = a[0].v / b;
            const double r = strategy == 0 ? std::floor(q + 0.5) : strategy == 1 ? std::ceil(q)
                    : strategy == 2 ? std::floor(q) : std::trunc(q);
            return {r * b, a[0].t};
        }
        const auto a = mathArgs(ch);
        if (name == u"calc") {
            if (m_argCount != 1)
                fail(QStringLiteral("calc() takes one argument"));
            return a[0];
        }
        const qsizetype argc = m_argCount;
        auto args = [&] { return QVarLengthArray<Num, 4>(a.begin(), a.begin() + argc); };
        if (name == u"min" || name == u"max") {
            const Unit t = same(args());
            double v = a[0].v;
            for (qsizetype k = 0; k < argc; ++k)
                v = name == u"min" ? std::min(v, a[k].v) : std::max(v, a[k].v);
            return {v, t};
        }
        if (name == u"clamp") {
            if (argc != 3)
                fail(QStringLiteral("clamp() takes three arguments"));
            const Unit t = same(args());
            return {std::max(a[0].v, std::min(a[1].v, a[2].v)), t};
        }
        if (name == u"mod" || name == u"rem") {
            const Unit t = same(args());
            if (argc != 2)
                fail(QStringLiteral("%1() takes two arguments").arg(name));
            if (a[1].v == 0)
                return {nan, t};
            const double q = a[0].v / a[1].v;
            return {a[0].v - a[1].v * (name == u"mod" ? std::floor(q) : std::trunc(q)), t};
        }
        if (name == u"sin")
            return {std::sin(radians(a[0])), Unit::Number};
        if (name == u"cos")
            return {std::cos(radians(a[0])), Unit::Number};
        if (name == u"tan")
            return {std::tan(radians(a[0])), Unit::Number};
        if (name == u"asin" || name == u"acos" || name == u"atan") {
            if (a[0].t != Unit::Number)
                fail(QStringLiteral("%1() of a dimension").arg(name));
            const double r = name == u"asin" ? std::asin(a[0].v) : name == u"acos" ? std::acos(a[0].v) : std::atan(a[0].v);
            return {r * 180 / M_PI, Unit::Angle};
        }
        if (name == u"atan2") {
            same(args());
            if (argc != 2)
                fail(QStringLiteral("atan2() takes two arguments"));
            return {std::atan2(a[0].v, a[1].v) * 180 / M_PI, Unit::Angle};
        }
        if (name == u"pow") {
            if (argc != 2 || a[0].t != Unit::Number || a[1].t != Unit::Number)
                fail(QStringLiteral("pow() takes two numbers"));
            return {std::pow(a[0].v, a[1].v), Unit::Number};
        }
        if (name == u"sqrt")
            return {a[0].v >= 0 ? std::sqrt(a[0].v) : nan, Unit::Number};
        if (name == u"hypot") {
            const Unit t = same(args());
            double s = 0;
            for (qsizetype k = 0; k < argc; ++k)
                s += a[k].v * a[k].v;
            return {std::sqrt(s), t};
        }
        if (name == u"log") {
            const double v = a[0].v;
            const double r = v > 0 ? std::log(v) : v == 0 ? -std::numeric_limits<double>::infinity() : nan;
            return {argc > 1 ? r / std::log(a[1].v) : r, Unit::Number};
        }
        if (name == u"exp")
            return {std::exp(a[0].v), Unit::Number};
        if (name == u"abs")
            return {std::abs(a[0].v), a[0].t};
        if (name == u"sign")
            return {a[0].v == 0 ? 0.0 : std::copysign(1.0, a[0].v), Unit::Number};
        fail(name);
        return {0, Unit::Number};
    }

    Comp component(const Channels *ch, bool allowPercent = true, bool hue = false, bool allowNone = true)
    {
        ws();
        const Tok &t = peek();
        if (t.kind == TokKind::Ident && t.text == u"none") {
            if (!allowNone)
                fail(QStringLiteral("none is not allowed here"));
            next();
            return {0, Unit::Number, true};
        }
        if (t.kind == TokKind::Ident && ch && ch->has(t.text)) {
            const double v = ch->get(next().text);
            return {std::isnan(v) ? 0.0 : v, Unit::Number, std::isnan(v)};
        }
        Num v{0, Unit::Number};
        if (t.kind == TokKind::Function && isMathFunction(t.text)) {
            const QString name = next().text;
            v = mathFunction(name, ch);
        } else if (t.kind == TokKind::Number || t.kind == TokKind::Percentage) {
            const Tok &x = next();
            v = {x.value, x.kind == TokKind::Number ? Unit::Number : Unit::Percentage};
        } else if (t.kind == TokKind::Dimension) {
            const Tok x = next();
            if (!hue)
                fail(QStringLiteral("unit %1 is not allowed here").arg(x.text));
            v = angle(x);
        } else {
            fail(QStringLiteral("expected a number"));
            return {0, Unit::Number, false};
        }
        if (v.t == Unit::Percentage && !allowPercent)
            fail(QStringLiteral("a percentage is not allowed here"));
        if (v.t == Unit::Angle && !hue)
            fail(QStringLiteral("an angle is not allowed here"));
        if (std::isnan(v.v))
            v.v = 0;
        return {v.v, v.t, false};
    }

    // [ / [<alpha-value> | none] ]?
    void alphaPart(S &c, const Channels *ch, const S *origin)
    {
        if (atDelim(u'/')) {
            next();
            const Comp a = component(ch);
            if (a.none) {
                c.setAlphaMissing();
                return;
            }
            if (a.t == Unit::Angle)
                fail(QStringLiteral("an angle is not an alpha value"));
            double v = a.t == Unit::Percentage ? a.v / 100 : a.v;
            if (std::isinf(v))
                v = v > 0 ? 1 : 0;
            c.setAlpha(std::clamp(v, 0.0, 1.0));
        } else if (origin) {
            if (origin->isAlphaMissing())
                c.setAlphaMissing();
            else
                c.setAlpha(origin->alpha());
        } else {
            c.setAlpha(1);
        }
    }

    S color()
    {
        ws();
        const Tok t = next();
        if (t.kind == TokKind::Hash)
            return hex(t.text);
        if (t.kind == TokKind::Ident) {
            for (const NamedColor &n : namedColors) {
                if (t.text == QLatin1String(n.name))
                    return rgb8(n.rgb);
            }
            if (t.text == u"transparent")
                return rgb8(0, 0);
            if (t.text == u"currentcolor") {
                usesCurrentColor = true;
                return m_ctx.currentColor;
            }
            S sys;
            if (m_ctx.usePalette && systemColor(t.text, &sys))
                return sys;
            fail(QStringLiteral("unknown colour %1").arg(t.text));
            return S();
        }
        if (t.kind == TokKind::Function) {
            const QString &f = t.text;
            if (f == u"rgb" || f == u"rgba")
                return rgbFunction();
            if (f == u"hsl" || f == u"hsla")
                return hslFunction();
            if (f == u"hwb")
                return modern(S::Hwb, nullptr, false);
            if (f == u"lab")
                return modern(S::Lab, nullptr, false);
            if (f == u"lch")
                return modern(S::Lch, nullptr, false);
            if (f == u"oklab")
                return modern(S::Oklab, nullptr, false);
            if (f == u"oklch")
                return modern(S::Oklch, nullptr, false);
            if (f == u"ictcp")
                return modern(S::Ictcp, nullptr, false);
            if (f == u"jzazbz")
                return modern(S::Jzazbz, nullptr, false);
            if (f == u"jzczhz")
                return modern(S::Jzczhz, nullptr, false);
            if (f == u"color")
                return colorFunction();
            if (f == u"color-mix")
                return colorMix();
            if (f == u"alpha")
                return alphaFunction();
            if (f == u"light-dark") {
                const S a = color();
                expect(TokKind::Comma, "','");
                const S b = color();
                expect(TokKind::RParen, "')'");
                return m_ctx.darkScheme ? b : a;
            }
            if (f == u"contrast-color") {
                const S c = color();
                expect(TokKind::RParen, "')'");
                return contrastColor(c);
            }
            if (f == u"device-cmyk")
                return deviceCmyk();
            if (f == u"hdr-color" || f == u"color-hdr")
                return hdrColor();
            fail(QStringLiteral("%1() is not a colour function").arg(f));
            return S();
        }
        fail(QStringLiteral("not a colour"));
        return S();
    }

    S hex(const QString &h)
    {
        if (h.size() != 3 && h.size() != 4 && h.size() != 6 && h.size() != 8) {
            fail(QStringLiteral("#%1 is not a hex colour").arg(h));
            return S();
        }
        QString x = h;
        if (h.size() <= 4) {
            x.clear();
            for (QChar c : h)
                x += QString(2, c);
        }
        bool ok = true;
        const uint v = x.toUInt(&ok, 16);
        if (!ok) {
            fail(QStringLiteral("#%1 is not a hex colour").arg(h));
            return S();
        }
        if (x.size() == 8)
            return rgb8(v >> 8, (v & 255) / 255.0);
        return rgb8(v);
    }

    const S *origin(S *storage)
    {
        if (atIdent("from")) {
            next();
            *storage = color();
            return storage;
        }
        return nullptr;
    }

    S rgbFunction()
    {
        S storage;
        const S *o = origin(&storage);
        if (!o) {
            S legacy;
            if (attempt([&] { legacy = rgbLegacy(); }))
                return legacy;
        }
        Channels ch;
        if (o) {
            const S s = o->convertedTo(S::Srgb);
            const char *names[] = {"r", "g", "b"};
            for (int i = 0; i < 3; ++i)
                ch.values.append({QLatin1String(names[i]), s.isMissing(i) ? qQNaN() : s.component(i) * 255});
            ch.values.append({QStringLiteral("alpha"), o->isAlphaMissing() ? qQNaN() : o->alpha()});
        }
        S c(S::Srgb, 0, 0, 0);
        bool anyNone = false;
        for (int i = 0; i < 3; ++i) {
            const Comp v = component(o ? &ch : nullptr);
            if (v.none) {
                c.setMissing(i);
                anyNone = true;
                continue;
            }
            double x = (v.t == Unit::Percentage ? v.v * 255 / 100 : v.v) / 255;
            if (!o)
                x = std::clamp(x, 0.0, 1.0);
            c.setComponent(i, x);
        }
        alphaPart(c, o ? &ch : nullptr, o);
        expect(TokKind::RParen, "')'");
        c.setLegacy(!o && !anyNone && !c.isAlphaMissing());
        return c;
    }

    S rgbLegacy()
    {
        double v[3];
        Unit kind = Unit::Number;
        for (int i = 0; i < 3; ++i) {
            if (i)
                expect(TokKind::Comma, "','");
            ws();
            if (peek().kind == TokKind::Ident) {
                fail(QStringLiteral("keyword in the legacy syntax"));
                return S();
            }
            const Comp c = component(nullptr, true, false, false);
            if (i && c.t != kind)
                fail(QStringLiteral("the legacy syntax does not mix numbers and percentages"));
            kind = c.t;
            v[i] = c.v;
        }
        double alpha = 1;
        if (at(TokKind::Comma)) {
            next();
            const Comp a = component(nullptr, true, false, false);
            alpha = std::clamp(a.t == Unit::Percentage ? a.v / 100 : a.v, 0.0, 1.0);
        }
        expect(TokKind::RParen, "')'");
        const double k = kind == Unit::Percentage ? 255.0 / 100 : 1.0;
        S c(S::Srgb, std::clamp(v[0] * k / 255, 0.0, 1.0), std::clamp(v[1] * k / 255, 0.0, 1.0),
            std::clamp(v[2] * k / 255, 0.0, 1.0), alpha);
        c.setLegacy(true);
        return c;
    }

    S hslFunction()
    {
        S storage;
        const S *o = origin(&storage);
        if (!o) {
            S legacy;
            const bool ok = attempt([&] {
                const Comp h = component(nullptr, false, true, false);
                expect(TokKind::Comma, "','");
                const Comp s = component(nullptr, true, false, false);
                expect(TokKind::Comma, "','");
                const Comp l = component(nullptr, true, false, false);
                if (s.t != Unit::Percentage || l.t != Unit::Percentage)
                    fail(QStringLiteral("legacy hsl() takes percentages"));
                double alpha = 1;
                if (at(TokKind::Comma)) {
                    next();
                    const Comp a = component(nullptr, true, false, false);
                    alpha = std::clamp(a.t == Unit::Percentage ? a.v / 100 : a.v, 0.0, 1.0);
                }
                expect(TokKind::RParen, "')'");
                legacy = S(S::Hsl, positiveMod(h.v, 360), std::max(0.0, s.v), l.v, alpha);
                legacy.setLegacy(true);
            });
            if (ok)
                return legacy;
        }
        return modern(S::Hsl, o, true);
    }

    // CSS Color 4 sections 7 to 9 and CSS Color HDR section 6: percentage reference ranges and parse-time clamps
    S modern(S::Space space, const S *o, bool originParsed)
    {
        S storage;
        if (!originParsed)
            o = origin(&storage);
        struct Rule { double ref; bool hue; bool clampLow; double low; bool clampHigh; double high; };
        Rule rules[3];
        const char *names[3];
        auto set = [&](int i, const char *n, double ref, bool hue = false, bool cl = false, double lo = 0,
                       bool ch = false, double hi = 0) {
            names[i] = n;
            rules[i] = {ref, hue, cl, lo, ch, hi};
        };
        switch (space) {
        case S::Hsl: set(0, "h", 0, true); set(1, "s", 100); set(2, "l", 100); break;
        case S::Hwb: set(0, "h", 0, true); set(1, "w", 100); set(2, "b", 100); break;
        case S::Lab: set(0, "l", 100, false, true, 0, true, 100); set(1, "a", 125); set(2, "b", 125); break;
        case S::Lch: set(0, "l", 100, false, true, 0, true, 100); set(1, "c", 150, false, true, 0); set(2, "h", 0, true); break;
        case S::Oklab: set(0, "l", 1, false, true, 0, true, 1); set(1, "a", 0.4); set(2, "b", 0.4); break;
        case S::Oklch: set(0, "l", 1, false, true, 0, true, 1); set(1, "c", 0.4, false, true, 0); set(2, "h", 0, true); break;
        case S::Ictcp: set(0, "i", 1); set(1, "ct", 0.5); set(2, "cp", 0.5); break;
        case S::Jzazbz: set(0, "j", 1); set(1, "a", 0.21); set(2, "b", 0.21); break;
        case S::Jzczhz: set(0, "j", 1); set(1, "c", 0.26); set(2, "h", 0, true); break;
        default: fail(QStringLiteral("internal: not a colour function space")); return S();
        }
        Channels ch;
        if (o) {
            const S s = o->convertedTo(space);
            for (int i = 0; i < 3; ++i)
                ch.values.append({QLatin1String(names[i]), s.isMissing(i) ? qQNaN() : s.component(i)});
            ch.values.append({QStringLiteral("alpha"), o->isAlphaMissing() ? qQNaN() : o->alpha()});
        }
        // relative colours keep their values, except that Lab, LCH, Oklab and OkLCh lightness and chroma keep
        // their parse-time clamps (as the web-platform-tests relative colour tests expect)
        const bool keepClamps = !o || space == S::Lab || space == S::Lch || space == S::Oklab || space == S::Oklch;
        S c(space, 0, 0, 0);
        for (int i = 0; i < 3; ++i) {
            const Rule &r = rules[i];
            const Comp v = component(o ? &ch : nullptr, !r.hue, r.hue);
            if (v.none) {
                c.setMissing(i);
                continue;
            }
            double x = v.t == Unit::Percentage ? v.v * r.ref / 100 : v.v;
            if (r.hue)
                x = std::isinf(x) ? 0.0 : positiveMod(x, 360);
            if (keepClamps && r.clampLow)
                x = std::max(r.low, x);
            if (keepClamps && r.clampHigh)
                x = std::min(r.high, x);
            if (!o && space == S::Hsl && i == 1)
                x = std::max(0.0, x);
            c.setComponent(i, x);
        }
        alphaPart(c, o ? &ch : nullptr, o);
        expect(TokKind::RParen, "')'");
        return c;
    }

    S colorFunction()
    {
        S storage;
        const S *o = origin(&storage);
        ws();
        const Tok t = next();
        if (t.kind != TokKind::Ident) {
            fail(QStringLiteral("color() needs a colour space"));
            return S();
        }
        static const struct { const char *name; S::Space space; } spaces[] = {
            {"srgb", S::Srgb}, {"srgb-linear", S::SrgbLinear}, {"display-p3", S::DisplayP3},
            {"display-p3-linear", S::DisplayP3Linear}, {"a98-rgb", S::A98Rgb}, {"prophoto-rgb", S::ProphotoRgb},
            {"rec2020", S::Rec2020}, {"rec2100-pq", S::Rec2100Pq}, {"rec2100-hlg", S::Rec2100Hlg},
            {"rec2100-linear", S::Rec2100Linear}, {"xyz", S::XyzD65}, {"xyz-d50", S::XyzD50}, {"xyz-d65", S::XyzD65}};
        S::Space space = S::Invalid;
        for (const auto &s : spaces) {
            if (t.text == QLatin1String(s.name))
                space = s.space;
        }
        if (space == S::Invalid) {
            if (t.text.startsWith(u"--"))
                fail(QStringLiteral("the custom colour space %1 needs an @color-profile, which Qt SVG does not load").arg(t.text));
            fail(QStringLiteral("%1 is not a predefined colour space").arg(t.text));
            return S();
        }
        const bool xyz = space == S::XyzD50 || space == S::XyzD65;
        Channels ch;
        if (o) {
            const S s = o->convertedTo(space);
            const char *names[] = {xyz ? "x" : "r", xyz ? "y" : "g", xyz ? "z" : "b"};
            for (int i = 0; i < 3; ++i)
                ch.values.append({QLatin1String(names[i]), s.isMissing(i) ? qQNaN() : s.component(i)});
            ch.values.append({QStringLiteral("alpha"), o->isAlphaMissing() ? qQNaN() : o->alpha()});
        }
        S c(space, 0, 0, 0);
        for (int i = 0; i < 3; ++i) {
            const Comp v = component(o ? &ch : nullptr);
            if (v.none)
                c.setMissing(i);
            else
                c.setComponent(i, v.t == Unit::Percentage ? v.v / 100 : v.v);
        }
        alphaPart(c, o ? &ch : nullptr, o);
        expect(TokKind::RParen, "')'");
        return c;
    }

    S alphaFunction()
    {
        S storage;
        const S *o = origin(&storage);
        if (!o) {
            fail(QStringLiteral("alpha() needs from <color>"));
            return S();
        }
        if (!atDelim(u'/')) {
            fail(QStringLiteral("alpha() needs / <alpha-value>"));
            return S();
        }
        Channels ch;
        ch.values.append({QStringLiteral("alpha"), o->isAlphaMissing() ? qQNaN() : o->alpha()});
        S c = *o;
        alphaPart(c, &ch, o);
        expect(TokKind::RParen, "')'");
        return c;
    }

    S deviceCmyk()
    {
        double v[4] = {0, 0, 0, 0};
        S c(S::Srgb, 0, 0, 0);
        const bool legacy = attempt([&] {
            for (int i = 0; i < 4; ++i) {
                if (i)
                    expect(TokKind::Comma, "','");
                v[i] = component(nullptr, false, false, false).v;
            }
            expect(TokKind::RParen, "')'");
        });
        if (!legacy) {
            for (int i = 0; i < 4; ++i) {
                const Comp x = component(nullptr);
                v[i] = x.none ? 0 : x.t == Unit::Percentage ? x.v / 100 : x.v;
            }
            alphaPart(c, nullptr, nullptr);
            expect(TokKind::RParen, "')'");
        }
        for (double &x : v)
            x = std::clamp(x, 0.0, 1.0);
        // CSS Color 5 section 6.1, the naive conversion (there is no @color-profile for device-cmyk)
        c.setComponent(0, 1 - std::min(1.0, v[0] * (1 - v[3]) + v[3]));
        c.setComponent(1, 1 - std::min(1.0, v[1] * (1 - v[3]) + v[3]));
        c.setComponent(2, 1 - std::min(1.0, v[2] * (1 - v[3]) + v[3]));
        return c;
    }

    // CSS Color 5 section 8: white or black, whichever contrasts more; white on a tie.  The algorithm is
    // UA-defined; this one uses the WCAG 2.1 relative luminance of the gamut-mapped sRGB colour.
    S contrastColor(const S &in)
    {
        S opaque = in;
        opaque.setAlpha(1);
        const QSvgCssTargetColor t = qsvg_resolveForTarget(opaque, QSvgCssColorTarget());
        const Vec3 lin = linSrgb({t.rgb[0], t.rgb[1], t.rgb[2]});
        const double lum = mul(srgbToXyz, lin)[1];
        return (1.05 / (lum + 0.05)) >= ((lum + 0.05) / 0.05) ? rgb8(0xffffff) : rgb8(0x000000);
    }

    // CSS Color 5 section 3.3, with CSS Values 5 section 6.1's percentage normalization
    S colorMix()
    {
        S::Space space = S::Oklab;
        int hueMethod = 0;   // shorter, longer, increasing, decreasing
        if (atIdent("in")) {
            next();
            ws();
            const Tok t = next();
            static const struct { const char *name; S::Space space; } spaces[] = {
                {"srgb", S::Srgb}, {"srgb-linear", S::SrgbLinear}, {"display-p3", S::DisplayP3},
                {"display-p3-linear", S::DisplayP3Linear}, {"a98-rgb", S::A98Rgb}, {"prophoto-rgb", S::ProphotoRgb},
                {"rec2020", S::Rec2020}, {"lab", S::Lab}, {"oklab", S::Oklab}, {"xyz", S::XyzD65},
                {"xyz-d50", S::XyzD50}, {"xyz-d65", S::XyzD65}, {"hsl", S::Hsl}, {"hwb", S::Hwb}, {"lch", S::Lch},
                {"oklch", S::Oklch}};
            space = S::Invalid;
            if (t.kind == TokKind::Ident) {
                for (const auto &s : spaces) {
                    if (t.text == QLatin1String(s.name))
                        space = s.space;
                }
            }
            if (space == S::Invalid) {
                fail(QStringLiteral("not an interpolation colour space"));
                return S();
            }
            if (S::isPolar(space) && at(TokKind::Ident)) {
                const QString m = peek().text;
                if (m == u"shorter" || m == u"longer" || m == u"increasing" || m == u"decreasing") {
                    next();
                    hueMethod = m == u"shorter" ? 0 : m == u"longer" ? 1 : m == u"increasing" ? 2 : 3;
                    if (!atIdent("hue"))
                        fail(QStringLiteral("expected hue"));
                    next();
                }
            }
            expect(TokKind::Comma, "','");
        }
        struct Item { S color; double pct; bool given; };
        QVarLengthArray<Item, 4> items;
        auto atPercent = [&] {
            ws();
            return peek().kind == TokKind::Percentage || (peek().kind == TokKind::Function && isMathFunction(peek().text));
        };
        for (;;) {
            Item it{S(), 0, false};
            if (atPercent()) {
                it.pct = mixPercent();
                it.given = true;
                it.color = color();
            } else {
                it.color = color();
                if (atPercent()) {
                    it.pct = mixPercent();
                    it.given = true;
                }
            }
            items.append(it);
            if (m_failed)
                return S();
            if (at(TokKind::Comma)) {
                next();
                continue;
            }
            expect(TokKind::RParen, "')'");
            break;
        }
        double specified = 0;
        int omitted = 0;
        for (const Item &it : items) {
            if (it.given)
                specified += it.pct;
            else
                ++omitted;
        }
        const double specifiedClamped = std::min(specified, 100.0);
        double total = 0;
        for (Item &it : items) {
            if (!it.given)
                it.pct = (100 - specifiedClamped) / omitted;
            total += it.pct;
        }
        if (total > 0) {
            for (Item &it : items)
                it.pct = it.pct * 100 / total;
        }
        const double leftover = total < 100 ? 100 - total : 0;
        S result;
        if (items.size() == 1) {
            result = items[0].color.convertedTo(space);
        } else {
            S acc = items[0].color;
            double accPct = items[0].pct;
            for (qsizetype k = 1; k < items.size(); ++k) {
                const double combined = accPct + items[k].pct;
                const double p = combined > 0 ? items[k].pct / combined : 0.5;
                acc = interpolate(acc, items[k].color, p, space, hueMethod);
                accPct = combined;
            }
            result = acc;
        }
        if (!result.isAlphaMissing())
            result.setAlpha(result.alpha() * (1 - leftover / 100));
        return result;
    }

    double mixPercent()
    {
        ws();
        const bool literal = peek().kind == TokKind::Percentage;
        const Comp v = component(nullptr, true, false, false);
        if (v.t != Unit::Percentage)
            fail(QStringLiteral("a color-mix() amount is a percentage"));
        if (literal && (v.v < 0 || v.v > 100))
            fail(QStringLiteral("a color-mix() amount is between 0% and 100%"));
        return std::clamp(v.v, 0.0, 100.0);
    }

    // CSS Color HDR sections 5.1 and 11.6
    S hdrColor()
    {
        S c[2];
        double h[2] = {0, 0};
        for (int i = 0; i < 2; ++i) {
            if (i)
                expect(TokKind::Comma, "','");
            ws();
            if (peek().kind == TokKind::Number) {
                h[i] = next().value;
                c[i] = color();
            } else {
                c[i] = color();
                ws();
                if (peek().kind == TokKind::Number)
                    h[i] = next().value;
            }
            if (h[i] < 0)
                fail(QStringLiteral("a negative headroom"));
        }
        expect(TokKind::RParen, "')'");
        if (!m_failed && h[0] == h[1])
            fail(QStringLiteral("hdr-color() needs two different headrooms"));
        // the colour is resolved when it is painted, against the target's headroom: it keeps both colours
        S out = hdrMix(c[0], h[0], c[1], h[1], 0.0);
        out.setHdrPair(c[0], h[0], c[1], h[1]);
        return out;
    }

public:
    static S hdrMix(const S &c1, double h1, const S &c2, double h2, double H)
    {
        const Vec3 x1 = c1.toXyzD65(), x2 = c2.toXyzD65();
        const double w1 = std::clamp((H - h2) / (h1 - h2), 0.0, 1.0);
        const double w2 = std::clamp((H - h1) / (h2 - h1), 0.0, 1.0);
        const double eps = 0.001;
        Vec3 out;
        for (int i = 0; i < 3; ++i)
            out[i] = (std::pow(x1[i] * Yw + eps, w1) * std::pow(x2[i] * Yw + eps, w2) - eps) / Yw;
        const double a1 = c1.isAlphaMissing() ? 1 : c1.alpha(), a2 = c2.isAlphaMissing() ? 1 : c2.alpha();
        return S(S::XyzD65, out[0], out[1], out[2], a1 * w1 + a2 * w2);
    }
};

// CSS Color 4 section 13
S interpolate(const S &a, const S &b, double p, S::Space space, int hueMethod)
{
    const S ca = a.convertedTo(space), cb = b.convertedTo(space);
    double va[3], vb[3];
    bool ma[3], mb[3];
    for (int i = 0; i < 3; ++i) {
        va[i] = ca.component(i);
        vb[i] = cb.component(i);
        ma[i] = ca.isMissing(i);
        mb[i] = cb.isMissing(i);
        if (ma[i] && !mb[i]) {
            va[i] = vb[i];
            ma[i] = false;
        } else if (mb[i] && !ma[i]) {
            vb[i] = va[i];
            mb[i] = false;
        }
    }
    bool aMissA = a.isAlphaMissing(), aMissB = b.isAlphaMissing();
    double aa = a.alpha(), ab = b.alpha();
    if (aMissA && !aMissB) {
        aa = ab;
        aMissA = false;
    } else if (aMissB && !aMissA) {
        ab = aa;
        aMissB = false;
    }
    const int hi = hueIndex(space);
    if (hi >= 0 && !ma[hi] && !mb[hi]) {
        double t1 = positiveMod(va[hi], 360), t2 = positiveMod(vb[hi], 360);
        const double d = t2 - t1;
        switch (hueMethod) {
        case 0:
            if (d > 180)
                t1 += 360;
            else if (d < -180)
                t2 += 360;
            break;
        case 1:
            if (0 < d && d < 180)
                t1 += 360;
            else if (-180 < d && d <= 0)
                t2 += 360;
            break;
        case 2:
            if (t2 < t1)
                t2 += 360;
            break;
        case 3:
            if (t1 < t2)
                t1 += 360;
            break;
        }
        va[hi] = t1;
        vb[hi] = t2;
    }
    const bool alphaNone = aMissA;
    S out(space, 0, 0, 0);
    const double alpha = alphaNone ? 0 : aa * (1 - p) + ab * p;
    for (int i = 0; i < 3; ++i) {
        if (ma[i] || mb[i]) {
            out.setMissing(i);
            continue;
        }
        double x;
        if (alphaNone || i == hi) {
            x = va[i] * (1 - p) + vb[i] * p;
        } else {
            x = va[i] * aa * (1 - p) + vb[i] * ab * p;
            if (alpha != 0)
                x /= alpha;
        }
        if (i == hi)
            x = positiveMod(x, 360);
        out.setComponent(i, x);
    }
    if (alphaNone)
        out.setAlphaMissing();
    else
        out.setAlpha(alpha);
    return out;
}

// ---------------------------------------------------------------- gamut mapping, CSS Color 4 section 14.2.2

// eps absorbs the rounding of the matrix round trip, so that a colour on the gamut boundary (white) is in gamut
bool inUnit(const Vec3 &v)
{
    const double eps = 1e-9;
    return v[0] >= -eps && v[0] <= 1 + eps && v[1] >= -eps && v[1] <= 1 + eps && v[2] >= -eps && v[2] <= 1 + eps;
}

// the colour in dest for display: missing components are zero (CSS Color 4 section 4.4), nothing carried forward
Vec3 displayValues(const S &color, S::Space dest)
{
    if (color.space() == dest) {
        Vec3 v;
        for (int i = 0; i < 3; ++i)
            v[i] = color.isMissing(i) || std::isnan(color.component(i)) ? 0.0 : color.component(i);
        return v;
    }
    return fromXyz(dest, color.toXyzD65());
}

Vec3 clip(const Vec3 &v)
{
    return {std::clamp(v[0], 0.0, 1.0), std::clamp(v[1], 0.0, 1.0), std::clamp(v[2], 0.0, 1.0)};
}

double deltaEOK(const Vec3 &a, const Vec3 &b)
{
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

Vec3 gamutMap(const S &color, S::Space dest, bool *mapped)
{
    *mapped = false;
    const Vec3 target = displayValues(color, dest);
    if (inUnit(target))
        return clip(target);
    *mapped = true;
    const S o = color.convertedTo(S::Oklch);
    const double L = o.isMissing(0) ? 0 : o.component(0);
    const double C = o.isMissing(1) ? 0 : o.component(1);
    const double H = o.isMissing(2) ? 0 : o.component(2);
    if (L >= 1)
        return clip(fromXyz(dest, oklabToXyz({1, 0, 0})));
    if (L <= 0)
        return clip(fromXyz(dest, oklabToXyz({0, 0, 0})));
    const double JND = 0.02, epsilon = 0.0001;
    auto oklabOfDest = [&](const Vec3 &v) { return xyzToOklab(toXyz(dest, v)); };
    Vec3 current = {L, C, H};
    Vec3 curDest = fromXyz(dest, oklabToXyz(fromPolar(current)));
    Vec3 clipped = clip(curDest);
    double E = deltaEOK(oklabOfDest(clipped), fromPolar(current));
    if (E < JND)
        return clipped;
    double lo = 0, hi = C;
    bool minInGamut = true;
    while (hi - lo > epsilon) {
        const double chroma = (lo + hi) / 2;
        current = {L, chroma, H};
        curDest = fromXyz(dest, oklabToXyz(fromPolar(current)));
        if (minInGamut && inUnit(curDest)) {
            lo = chroma;
            continue;
        }
        clipped = clip(curDest);
        E = deltaEOK(oklabOfDest(clipped), fromPolar(current));
        if (E < JND) {
            if (JND - E < epsilon)
                return clipped;
            minInGamut = false;
            lo = chroma;
        } else {
            hi = chroma;
        }
    }
    return clipped;
}

} // namespace

QSvgCssColor QSvgCssColor::atHeadroom(double headroom) const
{
    if (!m_hdr)
        return *this;
    return Parser::hdrMix(m_hdr->c1, m_hdr->h1, m_hdr->c2, m_hdr->h2, headroom);
}

double QSvgCssColor::hdrMaxHeadroom() const
{
    return m_hdr ? std::max(m_hdr->h1, m_hdr->h2) : 0.0;
}

void QSvgCssColor::setHdrPair(const QSvgCssColor &c1, double h1, const QSvgCssColor &c2, double h2)
{
    m_hdr = std::make_shared<const HdrPair>(HdrPair{c1, c2, h1, h2});
}

QSvgCssColorParseResult qsvg_parseCssColor(QStringView text, const QSvgCssColorContext &context)
{
    QSvgCssColorParseResult result;
    Parser p(tokenize(text), context);
    result.color = p.parseWhole();
    result.usesCurrentColor = p.usesCurrentColor;
    if (p.m_failed) {
        result.color = QSvgCssColor();
        result.error = p.m_error;
    }
    return result;
}

QSvgCssColorTarget QSvgCssColorTarget::forDevice(const QPaintDevice *device)
{
    QSvgCssColorTarget t;
    if (!device || device->devType() != QInternal::Image)
        return t;
    const QImage *image = static_cast<const QImage *>(device);
    switch (image->format()) {
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
        t.extendedRange = true;
        break;
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        t.extendedRange = true;
        t.floatFormat = true;
        break;
    default:
        break;
    }
    const QColorSpace cs = image->colorSpace();
    if (!cs.isValid())
        return t;
    if (cs == QColorSpace(QColorSpace::SRgb))
        t.space = QSvgCssColor::Srgb;
    else if (cs == QColorSpace(QColorSpace::SRgbLinear))
        t.space = QSvgCssColor::SrgbLinear;
    else if (cs == QColorSpace(QColorSpace::DisplayP3))
        t.space = QSvgCssColor::DisplayP3;
    else if (cs == QColorSpace(QColorSpace::AdobeRgb))
        t.space = QSvgCssColor::A98Rgb;
    else if (cs == QColorSpace(QColorSpace::ProPhotoRgb))
        t.space = QSvgCssColor::ProphotoRgb;
    else if (cs == QColorSpace(QColorSpace::Bt2020))
        t.space = QSvgCssColor::Rec2020;
    else if (cs == QColorSpace(QColorSpace::Bt2100Pq))
        t.space = QSvgCssColor::Rec2100Pq;
    else if (cs == QColorSpace(QColorSpace::Bt2100Hlg))
        t.space = QSvgCssColor::Rec2100Hlg;
    else if (cs.primaries() == QColorSpace::Primaries::Bt2020 && cs.transferFunction() == QColorSpace::TransferFunction::Linear)
        t.space = QSvgCssColor::Rec2100Linear;
    else if (cs.primaries() == QColorSpace::Primaries::DciP3D65 && cs.transferFunction() == QColorSpace::TransferFunction::Linear)
        t.space = QSvgCssColor::DisplayP3Linear;
    else
        t.unknownSpace = true;
    // PQ and HLG code values are bounded, whatever the storage
    if (t.space == QSvgCssColor::Rec2100Pq || t.space == QSvgCssColor::Rec2100Hlg)
        t.extendedRange = false;
    return t;
}

QSvgCssColorTarget QSvgCssColorTarget::forPainter(const QPainter *painter)
{
    return forDevice(painter ? painter->device() : nullptr);
}

QSvgCssTargetColor qsvg_resolveForTarget(const QSvgCssColor &color, const QSvgCssColorTarget &target)
{
    QSvgCssTargetColor out;
    if (!color.isValid())
        return out;
    // CSS Color HDR section 5: an hdr-color() takes the target's headroom
    if (color.hasHdrPair())
        return qsvg_resolveForTarget(color.atHeadroom(target.extendedRange ? color.hdrMaxHeadroom() : 0.0), target);
    out.alpha = color.isAlphaMissing() ? 0 : color.alpha();
    if (target.extendedRange) {
        out.rgb = displayValues(color, target.space);
        for (double v : out.rgb) {
            if (v < 0 || v > 1)
                out.outOfRange = true;
        }
        return out;
    }
    bool mapped = false;
    const Vec3 v = gamutMap(color, target.space, &mapped);
    out.rgb = clip(v);
    out.gamutMapped = mapped;
    return out;
}

QColor qsvg_toQColor(const QSvgCssTargetColor &value)
{
    if (!value.outOfRange) {
        auto u16 = [](double v) { return quint16(std::lround(std::clamp(v, 0.0, 1.0) * 65535)); };
        return QColor::fromRgba64(u16(value.rgb[0]), u16(value.rgb[1]), u16(value.rgb[2]), u16(value.alpha));
    }
    return QColor::fromRgbF(float(value.rgb[0]), float(value.rgb[1]), float(value.rgb[2]), float(value.alpha));
}

QBrush qsvg_solidBrush(const QSvgCssTargetColor &value, const QSvgCssColorTarget &target)
{
    if (value.outOfRange && target.floatFormat) {
        QImage texel(1, 1, QImage::Format_RGBA32FPx4_Premultiplied);
        float *p = reinterpret_cast<float *>(texel.bits());
        const double a = value.alpha;
        p[0] = float(value.rgb[0] * a);
        p[1] = float(value.rgb[1] * a);
        p[2] = float(value.rgb[2] * a);
        p[3] = float(a);
        return QBrush(texel);
    }
    return QBrush(qsvg_toQColor(value));
}

static QString describeTarget(const QSvgCssColorTarget &t)
{
    return QString::fromLatin1("%1%2").arg(QLatin1String(QSvgCssColor::name(t.space)),
                                         t.extendedRange ? QLatin1String(", extended range") : QLatin1String(""));
}

QBrush QSvgCssPaint::brush(const QPainter *painter)
{
    const QSvgCssColorTarget target = QSvgCssColorTarget::forPainter(painter);
    if (m_cached && target == m_target)
        return m_brush;
    const QSvgCssTargetColor v = qsvg_resolveForTarget(m_color, target);
    if (target.unknownSpace) {
        qCWarning(lcSvgHandler, "The target image's colour space is none that CSS Color 4 or CSS Color HDR predefines; "
                  "%s is drawn for sRGB", qPrintable(m_text));
    }
    if (v.gamutMapped) {
        qCWarning(lcSvgHandler, "%s is outside the gamut of the target (%s); it is drawn gamut mapped, as CSS Color 4 "
                  "section 14.2 says", qPrintable(m_text), qPrintable(describeTarget(target)));
    }
    m_brush = qsvg_solidBrush(v, target);
    m_target = target;
    m_cached = true;
    return m_brush;
}

QGradientStops qsvg_gradientStops(const QGradientStops &positions, const QList<QSvgCssColor> &colors,
                                  const QSvgCssColorTarget &target, bool *mapped)
{
    *mapped = false;
    QGradientStops out;
    if (positions.size() != colors.size())
        return positions;
    QSvgCssColorTarget bounded = target;
    bounded.extendedRange = false;
    bounded.floatFormat = false;
    auto resolve = [&](const QSvgCssColor &c) {
        const QSvgCssTargetColor v = qsvg_resolveForTarget(c, bounded);
        *mapped = *mapped || v.gamutMapped;
        return qsvg_toQColor(v);
    };
    // SVG gradients interpolate in sRGB (color-interpolation: sRGB, its initial value); in another target space
    // the interpolation is kept by 16 segments per stop interval, each interpolated in sRGB, premultiplied
    const bool subdivide = target.space != QSvgCssColor::Srgb;
    const int segments = 16;
    for (qsizetype i = 0; i < positions.size(); ++i) {
        const QSvgCssColor &c = colors.at(i);
        const QSvgCssColor c0 = c.hasHdrPair() ? c.atHeadroom(0) : c;
        out.append({positions.at(i).first, resolve(c0)});
        if (!subdivide || i + 1 >= positions.size())
            continue;
        const QSvgCssColor &d = colors.at(i + 1);
        const QSvgCssColor d0 = d.hasHdrPair() ? d.atHeadroom(0) : d;
        const auto a = c0.toXyzD65(), b = d0.toXyzD65();
        const QSvgCssColor sa = QSvgCssColor::fromXyzD65(QSvgCssColor::Srgb, a, c0.isAlphaMissing() ? 0 : c0.alpha());
        const QSvgCssColor sb = QSvgCssColor::fromXyzD65(QSvgCssColor::Srgb, b, d0.isAlphaMissing() ? 0 : d0.alpha());
        const double o0 = positions.at(i).first, o1 = positions.at(i + 1).first;
        for (int k = 1; k < segments; ++k) {
            const double p = double(k) / segments;
            const double alpha = sa.alpha() * (1 - p) + sb.alpha() * p;
            double rgb[3];
            for (int j = 0; j < 3; ++j) {
                const double x = sa.component(j) * sa.alpha() * (1 - p) + sb.component(j) * sb.alpha() * p;
                rgb[j] = alpha > 0 ? x / alpha : 0;
            }
            out.append({o0 + (o1 - o0) * p, resolve(QSvgCssColor(QSvgCssColor::Srgb, rgb[0], rgb[1], rgb[2], alpha))});
        }
    }
    return out;
}

QT_END_NAMESPACE
