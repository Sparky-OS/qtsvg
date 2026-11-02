// Copyright (C) 2026 Daniel Campos Ramos <Capitain_Jack@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGCSSCOLOR_P_H
#define QSVGCSSCOLOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtsvgglobal_p.h"

#include <QtGui/qbrush.h>
#include <QtGui/qcolor.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

#include <array>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QPainter;
class QPaintDevice;

// A <color> value of CSS Color 4, CSS Color 5 and CSS Color HDR 1, kept in its own colour space at full
// precision until it is painted, where the painter's target decides the space and the depth.
class Q_SVG_EXPORT QSvgCssColor
{
public:
    enum Space : quint8 {
        Invalid,
        // the predefined RGB spaces (CSS Color 4 section 10, CSS Color HDR section 8)
        Srgb, SrgbLinear, DisplayP3, DisplayP3Linear, A98Rgb, ProphotoRgb, Rec2020,
        Rec2100Pq, Rec2100Hlg, Rec2100Linear,
        XyzD50, XyzD65,
        // the other notations (CSS Color 4 sections 7 to 9, CSS Color HDR section 6)
        Hsl, Hwb, Lab, Lch, Oklab, Oklch, Ictcp, Jzazbz, Jzczhz
    };

    QSvgCssColor() = default;
    QSvgCssColor(Space space, double c0, double c1, double c2, double alpha = 1.0)
        : m_space(space), m_c{c0, c1, c2}, m_alpha(alpha) {}

    bool isValid() const { return m_space != Invalid; }
    Space space() const { return m_space; }
    double component(int i) const { return m_c[i]; }
    double alpha() const { return m_alpha; }
    bool isMissing(int i) const { return m_missing & (1u << i); }
    bool isAlphaMissing() const { return m_missing & 8u; }
    bool isLegacy() const { return m_legacy; }

    void setComponent(int i, double v) { m_c[i] = v; m_missing &= ~(1u << i); }
    void setMissing(int i) { m_c[i] = 0; m_missing |= (1u << i); }
    void setAlpha(double a) { m_alpha = a; m_missing &= ~8u; }
    void setAlphaMissing() { m_alpha = 0; m_missing |= 8u; }
    void setLegacy(bool legacy) { m_legacy = legacy; }

    // CSS Color 4 section 11.2, with the missing components of section 13.3 carried forward
    QSvgCssColor convertedTo(Space space) const;
    std::array<double, 3> toXyzD65() const;
    static QSvgCssColor fromXyzD65(Space space, const std::array<double, 3> &xyz, double alpha);

    static QSvgCssColor fromQColor(const QColor &color);
    // The colour for an sRGB target with no other information (16 bits per channel, gamut mapped):
    // what QSvgRenderer users that read a QColor back get.
    QColor toQColor() const;

    // CSS Color HDR section 5: an hdr-color() keeps its two colours, and the paint target's headroom picks between them
    bool hasHdrPair() const { return bool(m_hdr); }
    double hdrMaxHeadroom() const;
    QSvgCssColor atHeadroom(double headroom) const;
    void setHdrPair(const QSvgCssColor &c1, double h1, const QSvgCssColor &c2, double h2);

    static bool isPolar(Space space) { return space == Hsl || space == Hwb || space == Lch || space == Oklch || space == Jzczhz; }
    static bool isRgbSpace(Space space) { return space >= Srgb && space <= Rec2100Linear; }
    static const char *name(Space space);

private:
    Space m_space = Invalid;
    double m_c[3] = {0, 0, 0};
    double m_alpha = 1.0;
    quint8 m_missing = 0;
    bool m_legacy = false;
    struct HdrPair;
    std::shared_ptr<const HdrPair> m_hdr;
};

struct QSvgCssColor::HdrPair
{
    QSvgCssColor c1, c2;
    double h1, h2;
};

// The colour schemes Qt SVG supports (CSS Color Adjustment 1, CR Snapshot 30 October 2026, section 2).
enum class QSvgCssColorScheme : quint8 { Light, Dark };

// A colour scheme preference (CSS Color Adjustment 1 section 2.3): no preference, light or dark.
enum class QSvgCssColorSchemePreference : quint8 { None, Light, Dark };

// The computed value of the color-scheme property (CSS Color Adjustment 1 section 2.2): normal, or a colour scheme
// support, the light and dark keywords in their order (<custom-ident>s are meaningless and left out) and whether
// "only" was given.
struct Q_SVG_EXPORT QSvgCssColorSchemeValue
{
    bool normal = true;
    QList<QSvgCssColorScheme> schemes;
    bool only = false;
};

// Parses a color-scheme value: normal | [ light | dark | <custom-ident> ]+ && only?.  inherit and the other
// CSS-wide keywords that keep the parent's value set *keepsParent; initial gives normal.  On failure, returns false
// and *error says why.
Q_SVG_EXPORT bool qsvg_parseColorScheme(QStringView text, QSvgCssColorSchemeValue *value, bool *keepsParent,
                                        QString *error);

// CSS Color Adjustment 1 section 2.3, the used colour scheme of a support (normal is a null support) for a
// preference; Qt SVG supports light and dark, and its default is light.
Q_SVG_EXPORT QSvgCssColorScheme qsvg_usedColorScheme(const QSvgCssColorSchemeValue &support,
                                                     QSvgCssColorSchemePreference preference);

// The preferred colour scheme of the documents Qt SVG renders: an SVG document is embedded in the application, whose
// colour scheme is its preference (CSS Color Adjustment 1 section 2.1), so the application's colour scheme
// (QStyleHints::colorScheme()), and no preference without a QGuiApplication or when the platform does not say.
Q_SVG_EXPORT QSvgCssColorSchemePreference qsvg_preferredColorScheme();
// For tests: replaces the application's colour scheme as the preference; std::nullopt restores it.
Q_SVG_EXPORT void qsvg_setPreferredColorSchemeOverride(std::optional<QSvgCssColorSchemePreference> preference);

// What a colour is resolved against while it is parsed (CSS Color 4 sections 6.2 and 6.4, CSS Color 5 section 7):
// currentcolor, and the element colour scheme that system colours and light-dark() follow.
struct Q_SVG_EXPORT QSvgCssColorContext
{
    QSvgCssColor currentColor = QSvgCssColor(QSvgCssColor::Srgb, 0, 0, 0);
    QSvgCssColorScheme colorScheme = QSvgCssColorScheme::Light;
    bool usePalette = true;
};

struct Q_SVG_EXPORT QSvgCssColorParseResult
{
    QSvgCssColor color;
    bool usesCurrentColor = false;
    QString error;
    // set when a system colour comes from Qt's default palette for the element colour scheme, because the
    // application palette is one of the other colour scheme
    QString note;
};

// Parses a <color> (the whole string must be one colour).  On failure the result's colour is invalid and error says why.
Q_SVG_EXPORT QSvgCssColorParseResult qsvg_parseCssColor(QStringView text, const QSvgCssColorContext &context);

// The target a painter draws into: its colour space (an RGB space of QSvgCssColor) and whether it holds values
// outside [0, 1] (the floating point QImage formats).  A bounded target gets CSS Color 4 section 14.2 gamut mapping.
struct Q_SVG_EXPORT QSvgCssColorTarget
{
    QSvgCssColor::Space space = QSvgCssColor::Srgb;
    bool extendedRange = false;
    bool floatFormat = false;
    // HDR headroom in stops for hdr-color() (CSS Color HDR section 5): 0 for an SDR target
    double headroom = 0;
    // the image's colour space is none of the predefined ones: colours are drawn for sRGB, and said so
    bool unknownSpace = false;

    bool operator==(const QSvgCssColorTarget &o) const
    {
        return space == o.space && extendedRange == o.extendedRange && floatFormat == o.floatFormat
                && headroom == o.headroom && unknownSpace == o.unknownSpace;
    }
    bool operator!=(const QSvgCssColorTarget &o) const { return !(*this == o); }

    static QSvgCssColorTarget forDevice(const QPaintDevice *device);
    static QSvgCssColorTarget forPainter(const QPainter *painter);
};

struct Q_SVG_EXPORT QSvgCssTargetColor
{
    // the colour in the target's space, as the target stores it
    std::array<double, 3> rgb = {0, 0, 0};
    double alpha = 1;
    bool gamutMapped = false;
    bool outOfRange = false;    // components outside [0, 1] kept for an extended-range target
};

Q_SVG_EXPORT QSvgCssTargetColor qsvg_resolveForTarget(const QSvgCssColor &color, const QSvgCssColorTarget &target);
// A QColor that carries the target value at the precision QColor holds: 16 bits per channel in [0, 1],
// half float outside it.
Q_SVG_EXPORT QColor qsvg_toQColor(const QSvgCssTargetColor &value);
// A brush for a solid fill in the target; values outside [0, 1] on a 32-bit float target are carried by a
// 1x1 float texture, because QColor holds them as half floats only.
Q_SVG_EXPORT QBrush qsvg_solidBrush(const QSvgCssTargetColor &value, const QSvgCssColorTarget &target);

// A paint colour with its brush for the last target it was painted on; says once per target when the colour had to
// be gamut mapped (CSS Color 4 section 14.2) or the target's colour space is not a predefined one.
class Q_SVG_EXPORT QSvgCssPaint
{
public:
    void setColor(const QSvgCssColor &color, const QString &text)
    {
        m_color = color;
        m_text = text;
        m_cached = false;
    }
    bool isValid() const { return m_color.isValid(); }
    const QSvgCssColor &color() const { return m_color; }
    QBrush brush(const QPainter *painter);

private:
    QSvgCssColor m_color;
    QString m_text;
    QSvgCssColorTarget m_target;
    QBrush m_brush;
    bool m_cached = false;
};

// Gradient stops for a target: the stop colours resolved for its space, with SVG's sRGB interpolation
// (color-interpolation, SVG 2 section 13.12) kept by extra stops when the target's space is another one.
// QPainter's gradient tables hold [0, 1] at 16 bits, so stops are always gamut mapped; mapped says whether any was.
Q_SVG_EXPORT QGradientStops qsvg_gradientStops(const QGradientStops &positions, const QList<QSvgCssColor> &colors,
                                               const QSvgCssColorTarget &target, bool *mapped);

QT_END_NAMESPACE

#endif // QSVGCSSCOLOR_P_H
