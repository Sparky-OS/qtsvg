// Copyright (C) 2026 Daniel Campos Ramos <Capitain_Jack@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QtSvg/private/qsvgcsscolor_p.h>
#include <QtSvg/QSvgRenderer>
#include <QtGui/QColorSpace>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtCore/QSet>

#include <cmath>

using namespace Qt::Literals::StringLiterals;

// Expected values come from a separate reference implementation of the specifications' sample code
// (CSS Color 4 section 19, CSS Color HDR section 11), not from the code under test.
#include "tst_qsvgcsscolor_rows.inc"

class tst_QSvgCssColor : public QObject
{
    Q_OBJECT

private slots:
    void parse_data();
    void parse();
    void invalid_data();
    void invalid();
    void render_data();
    void render();
    void invalidIsSaid();
    void sixteenBits();
    void gradientStops();
    void colorSchemeValue_data();
    void colorSchemeValue();
    void colorScheme_data();
    void colorScheme();
    void colorSchemeIsNoPresentationAttribute();
    void invalidColorSchemeIsSaid();
    void systemColorsFollowColorScheme();
    void contrastColor_data();
    void contrastColor();
};

void tst_QSvgCssColor::parse_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<QString>("space");
    QTest::addColumn<double>("c0");
    QTest::addColumn<double>("c1");
    QTest::addColumn<double>("c2");
    QTest::addColumn<double>("alpha");
    for (const auto &r : parseRows) {
        QTest::newRow(r.name) << QString::fromUtf8(r.css) << QString::fromLatin1(r.space) << r.c[0] << r.c[1]
                              << r.c[2] << r.alpha;
    }
}

static bool close(double a, double b, double eps)
{
    if (std::isnan(b))
        return std::isnan(a);
    return std::abs(a - b) <= eps;
}

void tst_QSvgCssColor::parse()
{
    QFETCH(QString, css);
    QFETCH(QString, space);
    QFETCH(double, c0);
    QFETCH(double, c1);
    QFETCH(double, c2);
    QFETCH(double, alpha);
    const QSvgCssColorParseResult r = qsvg_parseCssColor(css, QSvgCssColorContext());
    QVERIFY2(r.color.isValid(), qPrintable(r.error));
    QCOMPARE(QString::fromLatin1(QSvgCssColor::name(r.color.space())), space);
    const double want[3] = {c0, c1, c2};
    for (int i = 0; i < 3; ++i) {
        const double got = r.color.isMissing(i) ? qQNaN() : r.color.component(i);
        QVERIFY2(close(got, want[i], 1e-9), qPrintable(u"component %1: %2, want %3"_s.arg(i).arg(got).arg(want[i])));
    }
    QVERIFY(close(r.color.alpha(), alpha, 1e-9));
}

void tst_QSvgCssColor::invalid_data()
{
    QTest::addColumn<QString>("css");
    for (const auto &r : invalidRows)
        QTest::newRow(r.name) << QString::fromUtf8(r.css);
}

void tst_QSvgCssColor::invalid()
{
    QFETCH(QString, css);
    const QSvgCssColorParseResult r = qsvg_parseCssColor(css, QSvgCssColorContext());
    QVERIFY(!r.color.isValid());
    QVERIFY(!r.error.isEmpty());
}

void tst_QSvgCssColor::render_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("space");
    QTest::addColumn<double>("r");
    QTest::addColumn<double>("g");
    QTest::addColumn<double>("b");
    QTest::addColumn<bool>("mapped");
    for (const auto &row : renderRows) {
        QTest::newRow(row.name) << QString::fromUtf8(row.css) << int(row.format) << row.space << row.rgb[0]
                                << row.rgb[1] << row.rgb[2] << row.mapped;
    }
}

static QImage renderSvg(const QByteArray &svg, QImage::Format format, int space, int w = 4, int h = 4)
{
    QImage image(w, h, format);
    if (space >= 0)
        image.setColorSpace(QColorSpace(QColorSpace::NamedColorSpace(space)));
    image.fill(Qt::transparent);
    QSvgRenderer renderer(svg);
    QPainter p(&image);
    renderer.render(&p);
    p.end();
    return image;
}

static void storedRgb(const QImage &image, int x, int y, double out[3])
{
    const uchar *line = image.constScanLine(y);
    if (image.format() == QImage::Format_RGBA64) {
        const quint16 *p = reinterpret_cast<const quint16 *>(line) + 4 * x;
        for (int i = 0; i < 3; ++i)
            out[i] = p[i] / 65535.0;
    } else {
        const float *p = reinterpret_cast<const float *>(line) + 4 * x;
        for (int i = 0; i < 3; ++i)
            out[i] = p[i];
    }
}

// a solid fill reaches the target at 16 bits per channel or better (CSS Color 4 section 16.5, CSS Color HDR
// section 10.1 ask for up to 16), and a color outside a bounded target is gamut mapped and said so
void tst_QSvgCssColor::render()
{
    QFETCH(QString, css);
    QFETCH(int, format);
    QFETCH(int, space);
    QFETCH(double, r);
    QFETCH(double, g);
    QFETCH(double, b);
    QFETCH(bool, mapped);
    if (mapped)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"outside the gamut of the target"_s));
    const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\">"
                           "<rect width=\"4\" height=\"4\" fill=\"" + css.toUtf8() + "\"/></svg>";
    const QImage image = renderSvg(svg, QImage::Format(format), space);
    double got[3];
    storedRgb(image, 1, 1, got);
    const double want[3] = {r, g, b};
    for (int i = 0; i < 3; ++i) {
        QVERIFY2(std::abs(got[i] - want[i]) <= 1.0 / 65536,
                 qPrintable(u"channel %1: %2, want %3"_s.arg(i).arg(got[i], 0, 'g', 9).arg(want[i], 0, 'g', 9)));
    }
}

// CSS Color 4 section 4.5: an invalid color drops its declaration; the renderer says so
void tst_QSvgCssColor::invalidIsSaid()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Invalid color \"lab\\(50% 40 abc\\)\""_s));
    const QImage image = renderSvg("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\">"
                                   "<rect width=\"4\" height=\"4\" fill=\"lab(50% 40 abc)\"/></svg>",
                                   QImage::Format_RGBA64, -1);
    // the fill keeps its initial value, black (SVG 2 section 13.2)
    QCOMPARE(image.pixelColor(1, 1), QColor(Qt::black));
}

// 256 solid fills one 16-bit step apart (1/65535) stay 256 distinct values in a 16-bit target
void tst_QSvgCssColor::sixteenBits()
{
    QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1024\" height=\"2\">";
    for (int k = 0; k < 256; ++k) {
        const double v = (32768 + k) / 65535.0;
        svg += "<rect x=\"" + QByteArray::number(4 * k) + "\" width=\"4\" height=\"2\" fill=\"color(srgb "
               + QByteArray::number(v, 'f', 10) + ' ' + QByteArray::number(v, 'f', 10) + ' '
               + QByteArray::number(v, 'f', 10) + ")\"/>";
    }
    svg += "</svg>";
    const QImage image = renderSvg(svg, QImage::Format_RGBA64, -1, 1024, 2);
    QSet<quint16> levels;
    for (int x = 0; x < 1024; ++x)
        levels.insert(reinterpret_cast<const quint16 *>(image.constScanLine(1))[4 * x + 1]);
    QCOMPARE(levels.size(), 256);
}

// gradient stops are resolved for the target: a display-p3 stop in an sRGB target is its sRGB value
void tst_QSvgCssColor::gradientStops()
{
    const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"2\"><defs>"
                           "<linearGradient id=\"g\"><stop offset=\"0\" stop-color=\"color(display-p3 0.423457 0.554321 0.676543)\"/>"
                           "<stop offset=\"1\" stop-color=\"color(display-p3 0.423457 0.554321 0.676543)\"/>"
                           "</linearGradient></defs><rect width=\"8\" height=\"2\" fill=\"url(#g)\"/></svg>";
    const QImage image = renderSvg(svg, QImage::Format_RGBA64, -1, 8, 2);
    double got[3];
    storedRgb(image, 4, 1, got);
    // color(display-p3 0.423457 0.554321 0.676543) in sRGB, from the reference implementation
    const double want[3] = {0.38617457, 0.55899669, 0.68873827};
    // QPainter's gradient tables hold 16 bits per channel
    for (int i = 0; i < 3; ++i)
        QVERIFY2(std::abs(got[i] - want[i]) <= 3.0 / 65535, qPrintable(u"%1 want %2"_s.arg(got[i]).arg(want[i])));
}

// CSS Color Adjustment 1 section 2.2: normal | [ light | dark | <custom-ident> ]+ && only?
void tst_QSvgCssColor::colorSchemeValue_data()
{
    QTest::addColumn<QString>("css");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("normal");
    QTest::addColumn<QString>("schemes");
    QTest::addColumn<bool>("only");
    QTest::newRow("normal") << u"normal"_s << true << true << QString() << false;
    QTest::newRow("light dark") << u"light dark"_s << true << false << u"light dark"_s << false;
    QTest::newRow("dark light") << u"Dark  LIGHT"_s << true << false << u"dark light"_s << false;
    QTest::newRow("only light") << u"only light"_s << true << false << u"light"_s << true;
    QTest::newRow("light only") << u"light only"_s << true << false << u"light"_s << true;
    QTest::newRow("custom ident") << u"foo dark"_s << true << false << u"dark"_s << false;
    QTest::newRow("initial") << u"initial"_s << true << true << QString() << false;
    QTest::newRow("only alone") << u"only"_s << false << false << QString() << false;
    QTest::newRow("only between") << u"light only dark"_s << false << false << QString() << false;
    QTest::newRow("normal and more") << u"normal light"_s << false << false << QString() << false;
    QTest::newRow("comma") << u"light, dark"_s << false << false << QString() << false;
    QTest::newRow("css-wide keyword in a list") << u"inherit dark"_s << false << false << QString() << false;
}

void tst_QSvgCssColor::colorSchemeValue()
{
    QFETCH(QString, css);
    QFETCH(bool, valid);
    QFETCH(bool, normal);
    QFETCH(QString, schemes);
    QFETCH(bool, only);
    QSvgCssColorSchemeValue value;
    bool keepsParent = true;
    QString error;
    QCOMPARE(qsvg_parseColorScheme(css, &value, &keepsParent, &error), valid);
    if (!valid) {
        QVERIFY(!error.isEmpty());
        return;
    }
    QVERIFY(!keepsParent);
    QCOMPARE(value.normal, normal);
    QStringList names;
    for (QSvgCssColorScheme s : value.schemes)
        names << (s == QSvgCssColorScheme::Dark ? u"dark"_s : u"light"_s);
    QCOMPARE(names.join(u' '), schemes);
    QCOMPARE(value.only, only);
}

void tst_QSvgCssColor::colorScheme_data()
{
    QTest::addColumn<QByteArray>("svg");
    QTest::addColumn<int>("preference");
    QTest::addColumn<double>("r");
    QTest::addColumn<double>("g");
    QTest::addColumn<double>("b");
    for (const auto &row : schemeRows) {
        QTest::newRow(row.name) << QByteArray(row.svg) << int(row.preference) << row.rgb[0] << row.rgb[1]
                                << row.rgb[2];
    }
}

// CSS Color 5 section 7: light-dark() takes its first color in a light element color scheme and its second in a
// dark one; the element color scheme comes from color-scheme and the preference (CSS Color Adjustment 1 sections
// 2.2 and 2.3), and without color-scheme it is light, whatever the preference
void tst_QSvgCssColor::colorScheme()
{
    QFETCH(QByteArray, svg);
    QFETCH(int, preference);
    QFETCH(double, r);
    QFETCH(double, g);
    QFETCH(double, b);
    qsvg_setPreferredColorSchemeOverride(QSvgCssColorSchemePreference(preference));
    const QImage image = renderSvg(svg, QImage::Format_RGBA64, -1);
    qsvg_setPreferredColorSchemeOverride(std::nullopt);
    double got[3];
    storedRgb(image, 1, 1, got);
    const double want[3] = {r, g, b};
    for (int i = 0; i < 3; ++i) {
        QVERIFY2(std::abs(got[i] - want[i]) <= 1.0 / 65536,
                 qPrintable(u"channel %1: %2, want %3"_s.arg(i).arg(got[i], 0, 'g', 9).arg(want[i], 0, 'g', 9)));
    }
}

// SVG 2 section 6.6: color-scheme has no presentation attribute, so the attribute is not read
void tst_QSvgCssColor::colorSchemeIsNoPresentationAttribute()
{
    const QImage image = renderSvg("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" "
                                   "color-scheme=\"dark\"><rect width=\"4\" height=\"4\" "
                                   "fill=\"light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>",
                                   QImage::Format_RGBA64, -1);
    double got[3];
    storedRgb(image, 1, 1, got);
    QVERIFY(std::abs(got[0] - 0.2) <= 1.0 / 65536);
}

void tst_QSvgCssColor::invalidColorSchemeIsSaid()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"Invalid color-scheme \"light only dark\""_s));
    renderSvg("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" "
              "style=\"color-scheme: light only dark\"><rect width=\"4\" height=\"4\"/></svg>",
              QImage::Format_RGBA64, -1);
}

// System colors follow the element color scheme (CSS Color Adjustment 1 section 2.4).  The application palette
// is a light one here; for the dark color scheme the colors come from Qt's default dark palette, which is said.
void tst_QSvgCssColor::systemColorsFollowColorScheme()
{
    const QPalette before = QGuiApplication::palette();
    QPalette light = before;
    light.setColor(QPalette::Base, QColor(0xf4, 0xf1, 0xe8));
    light.setColor(QPalette::Text, QColor(0x2b, 0x5a, 0x87));
    QGuiApplication::setPalette(light);
    const QByteArray rect = "<rect width=\"4\" height=\"4\" style=\"fill: CanvasText\"/></svg>";
    const QImage inLight = renderSvg("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" "
                                     "style=\"color-scheme: light\">" + rect, QImage::Format_RGBA64, -1);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"system colors of the dark color scheme come from "
                                                          "Qt's default palette"_s));
    const QImage inDark = renderSvg("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" "
                                    "style=\"color-scheme: dark\">" + rect, QImage::Format_RGBA64, -1);
    QGuiApplication::setPalette(before);
    QCOMPARE(inLight.pixelColor(1, 1).rgb(), QColor(0x2b, 0x5a, 0x87).rgb());
    // qtbase's qt_fusionPalette() for a dark platform: text (240, 240, 240)
    QCOMPARE(inDark.pixelColor(1, 1).rgb(), QColor(240, 240, 240).rgb());
}

void tst_QSvgCssColor::contrastColor_data()
{
    QTest::addColumn<QString>("background");
    QTest::addColumn<bool>("black");
    QTest::addColumn<bool>("wcagBlack");
    QTest::addColumn<double>("ratio");
    for (const auto &row : contrastRows)
        QTest::newRow(row.background) << QString::fromUtf8(row.background) << row.black << row.wcagBlack << row.ratio;
}

// CSS Color 5 section 8: white or black, by a UA-defined algorithm that is not simply WCAG 2.1's contrast ratio,
// with at least WCAG 2.1's 3:1 (AA for large text); the rows include backgrounds where the two choices differ
void tst_QSvgCssColor::contrastColor()
{
    QFETCH(QString, background);
    QFETCH(bool, black);
    QFETCH(bool, wcagBlack);
    QFETCH(double, ratio);
    const QSvgCssColorParseResult r = qsvg_parseCssColor(u"contrast-color(" + background + u')',
                                                         QSvgCssColorContext());
    QVERIFY2(r.color.isValid(), qPrintable(r.error));
    const double want = black ? 0.0 : 1.0;
    for (int i = 0; i < 3; ++i)
        QCOMPARE(r.color.component(i), want);

    // the WCAG 2.1 contrast ratio of the result with the background, as an sRGB color, is at least 3:1
    QSvgCssColor bg = qsvg_parseCssColor(background, QSvgCssColorContext()).color;
    bg.setAlpha(1);
    const QSvgCssTargetColor t = qsvg_resolveForTarget(bg, QSvgCssColorTarget());
    auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    const double lum = 0.2126 * linear(t.rgb[0]) + 0.7152 * linear(t.rgb[1]) + 0.0722 * linear(t.rgb[2]);
    const double got = black ? (lum + 0.05) / 0.05 : 1.05 / (lum + 0.05);
    QVERIFY2(got >= 3.0, qPrintable(u"contrast %1:1"_s.arg(got)));
    QVERIFY2(std::abs(got - ratio) < 1e-3, qPrintable(u"contrast %1:1, want %2:1"_s.arg(got).arg(ratio)));
    if (black != wcagBlack)
        qDebug("%s: %s, where WCAG 2.1's larger ratio would give %s", qPrintable(background),
               black ? "black" : "white", wcagBlack ? "black" : "white");
}

QTEST_MAIN(tst_QSvgCssColor)
#include "tst_qsvgcsscolor.moc"
