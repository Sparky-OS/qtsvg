// Copyright (C) 2026 Daniel Campos Ramos <Capitain_Jack@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Expected values computed with the specifications' own sample code (CSS Color 4
// section 19, CSS Color HDR section 11); regenerate them rather than edit by hand.
static const struct { const char *name; const char *css; const char *space; double c[3]; double alpha; } parseRows[] = {
    {"hex6 (C4 5.2)", "#336699", "srgb", {0.2, 0.4, 0.6}, 1.0},
    {"hex8 (C4 5.2)", "#33669980", "srgb", {0.2, 0.4, 0.6}, 0.5019607843137255},
    {"named (C4 6.1)", "rebeccapurple", "srgb", {0.4, 0.2, 0.6}, 1.0},
    {"transparent (C4 6.3)", "transparent", "srgb", {0.0, 0.0, 0.0}, 0.0},
    {"rgb-legacy (C4 5.1)", "rgb(51, 102, 153)", "srgb", {0.2, 0.4, 0.6}, 1.0},
    {"rgb-modern (C4 5.1)", "rgb(31.4159 159.2653 89.7932 / 80%)", "srgb", {0.12319960784313726, 0.6245698039215686, 0.3521301960784314}, 0.8},
    {"hsl (C4 7)", "hsl(210.123 50.5% 40.25%)", "hsl", {210.123, 50.5, 40.25}, 1.0},
    {"hwb (C4 8)", "hwb(150.5 20.25% 10.125%)", "hwb", {150.5, 20.25, 10.125}, 1.0},
    {"lab (C4 9.3)", "lab(52.2345% 40.1645 59.9971)", "lab", {52.2345, 40.1645, 59.9971}, 1.0},
    {"lch (C4 9.3)", "lch(52.2345% 72.2 56.2)", "lch", {52.2345, 72.2, 56.2}, 1.0},
    {"oklab (C4 9.4)", "oklab(59.686% 0.1009 0.1192)", "oklab", {0.59686, 0.1009, 0.1192}, 1.0},
    {"oklch (C4 9.4)", "oklch(0.6 0.2 30)", "oklch", {0.6, 0.2, 30.0}, 1.0},
    {"srgb (C4 10.2)", "color(srgb 0.123457 0.654321 0.876543)", "srgb", {0.123457, 0.654321, 0.876543}, 1.0},
    {"srgb-linear (C4 10.3)", "color(srgb-linear 0.0123457 0.3654321 0.6876543)", "srgb-linear", {0.0123457, 0.3654321, 0.6876543}, 1.0},
    {"display-p3 (C4 10.4)", "color(display-p3 0.423457 0.554321 0.676543)", "display-p3", {0.423457, 0.554321, 0.676543}, 1.0},
    {"display-p3-linear (C4 10.5)", "color(display-p3-linear 0.123457 0.354321 0.476543)", "display-p3-linear", {0.123457, 0.354321, 0.476543}, 1.0},
    {"a98-rgb (C4 10.6)", "color(a98-rgb 0.423457 0.554321 0.676543)", "a98-rgb", {0.423457, 0.554321, 0.676543}, 1.0},
    {"prophoto-rgb (C4 10.7)", "color(prophoto-rgb 0.423457 0.454321 0.576543)", "prophoto-rgb", {0.423457, 0.454321, 0.576543}, 1.0},
    {"rec2020 (C4 10.8)", "color(rec2020 0.423457 0.554321 0.676543)", "rec2020", {0.423457, 0.554321, 0.676543}, 1.0},
    {"xyz (C4 10.9)", "color(xyz 0.2123457 0.2654321 0.3876543)", "xyz-d65", {0.2123457, 0.2654321, 0.3876543}, 1.0},
    {"xyz-d50 (C4 10.9)", "color(xyz-d50 0.2123457 0.2654321 0.2876543)", "xyz-d50", {0.2123457, 0.2654321, 0.2876543}, 1.0},
    {"rec2100-pq (HDR 8.1)", "color(rec2100-pq 0.5 0.45 0.4)", "rec2100-pq", {0.5, 0.45, 0.4}, 1.0},
    {"rec2100-hlg (HDR 8.2)", "color(rec2100-hlg 0.6 0.55 0.5)", "rec2100-hlg", {0.6, 0.55, 0.5}, 1.0},
    {"rec2100-linear (HDR 8.3)", "color(rec2100-linear 2.5 2.25 2)", "rec2100-linear", {2.5, 2.25, 2.0}, 1.0},
    {"ictcp (HDR 6.3)", "ictcp(0.45 -0.05 0.03)", "ictcp", {0.45, -0.05, 0.03}, 1.0},
    {"jzazbz (HDR 6.4)", "jzazbz(0.12 0.02 -0.03)", "jzazbz", {0.12, 0.02, -0.03}, 1.0},
    {"jzczhz (HDR 6.5)", "jzczhz(0.12 0.04 300)", "jzczhz", {0.12, 0.04, 300.0}, 1.0},
    {"color-mix (C5 3)", "color-mix(in oklab, #336699 30%, color(display-p3 0.9 0.6 0.1))", "oklab", {0.6729674301837101, 0.032375191156416135, 0.08988942024095753}, 1.0},
    {"color-mix-hue (C5 3, C4 13.5)", "color-mix(in oklch longer hue, oklch(0.6 0.15 20), oklch(0.7 0.12 200))", "oklch", {0.6499999999999999, 0.135, 110.0}, 1.0},
    {"color-mix-three (C5 3.3)", "color-mix(in srgb-linear, red, lime 25%, blue 25%)", "srgb-linear", {0.5, 0.24999999999999994, 0.25}, 1.0},
    {"relative-oklch (C5 4.9)", "oklch(from #336699 calc(l + 0.1) calc(c * 1.5) calc(h + 30))", "oklch", {0.5993144558452084, 0.14799656568627492, 280.4330574201754}, 1.0},
    {"relative-color (C5 5.1)", "color(from color(display-p3 0.4 0.5 0.6) display-p3 calc(r * 0.9) g calc(b + 0.05))", "display-p3", {0.36000000000000004, 0.5, 0.65}, 1.0},
    {"alpha (C5 4.10)", "alpha(from color(display-p3 0.4 0.5 0.6) / 0.5)", "display-p3", {0.4, 0.5, 0.6}, 0.5},
    {"device-cmyk (C5 6)", "device-cmyk(10% 20% 30% 5% / 80%)", "srgb", {0.855, 0.76, 0.665}, 0.8},
    {"contrast-color (C5 8)", "contrast-color(color(display-p3 0.2 0.3 0.4))", "srgb", {1.0, 1.0, 1.0}, 1.0},
};
static const struct { const char *name; const char *css; } invalidRows[] = {
    {"unknown-space", "color(profoto-rgb 0.4835 0.9167 0.2188)"},
    {"bad-component", "lab(50% 40 abc)"},
    {"legacy-none", "rgb(none, 20, 30)"},
    {"commas-in-color", "color(srgb, 1, 0, 0)"},
    {"mix-negative", "color-mix(in srgb, red -10%, blue)"},
    {"hdr-same-headroom", "color-hdr(red 1, blue 1)"},
};
static const struct { const char *name; const char *css; QImage::Format format; int space; double rgb[3]; bool mapped; } renderRows[] = {
    {"color-srgb rgba64", "color(srgb 0.123457 0.654321 0.876543)", QImage::Format_RGBA64, -1, {0.123457, 0.654321, 0.876543}, false},
    {"color-srgb rgba64-p3", "color(srgb 0.123457 0.654321 0.876543)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {0.3133657657134425, 0.6447782979117164, 0.8562387137206271}, false},
    {"color-srgb rgba32f", "color(srgb 0.123457 0.654321 0.876543)", QImage::Format_RGBA32FPx4, -1, {0.123457, 0.654321, 0.876543}, false},
    {"color-srgb rgba32f-2020", "color(srgb 0.123457 0.654321 0.876543)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {0.4755004653272651, 0.6563648264397353, 0.8611406413029781}, false},
    {"lab rgba64", "lab(52.2345% 40.1645 59.9971)", QImage::Format_RGBA64, -1, {0.7761739623385975, 0.363389841266188, 0.02454739525191026}, false},
    {"lab rgba64-p3", "lab(52.2345% 40.1645 59.9971)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {0.7242782342605922, 0.386738038983091, 0.14836607991601777}, false},
    {"lab rgba32f", "lab(52.2345% 40.1645 59.9971)", QImage::Format_RGBA32FPx4, -1, {0.7761739623385975, 0.363389841266188, 0.02454739525191026}, false},
    {"lab rgba32f-2020", "lab(52.2345% 40.1645 59.9971)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {0.6753714612079438, 0.4393408195647032, 0.1980015700642023}, false},
    {"oklch rgba64", "oklch(0.7 0.3 145)", QImage::Format_RGBA64, -1, {0.0, 0.7660618230285321, 0.0}, true},
    {"oklch rgba64-p3", "oklch(0.7 0.3 145)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {0.10098928247604463, 0.7711538322814111, 0.0}, true},
    {"oklch rgba32f", "oklch(0.7 0.3 145)", QImage::Format_RGBA32FPx4, -1, {-0.3697087430112182, 0.7851986630475667, -0.23485115449652055}, false},
    {"oklch rgba32f-2020", "oklch(0.7 0.3 145)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {0.4105613648920222, 0.7640293393562103, 0.13914663018918239}, false},
    {"color-display-p3 rgba64", "color(display-p3 1 0.25 0)", QImage::Format_RGBA64, -1, {1.0, 0.25053909579955563, 0.014515301080269305}, true},
    {"color-display-p3 rgba64-p3", "color(display-p3 1 0.25 0)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {1.0, 0.25, 0.0}, false},
    {"color-display-p3 rgba32f", "color(display-p3 1 0.25 0)", QImage::Format_RGBA32FPx4, -1, {1.0885850082055275, 0.10587460215706865, -0.16661143660613567}, false},
    {"color-display-p3 rgba32f-2020", "color(display-p3 1 0.25 0)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {0.8938691552885285, 0.37280062096016514, -0.03474299459305302}, false},
    {"color-rec2100-linear rgba64", "color(rec2100-linear 2.5 2.25 2)", QImage::Format_RGBA64, -1, {1.0, 0.9999999999999997, 0.9999999999999997}, true},
    {"color-rec2100-linear rgba64-p3", "color(rec2100-linear 2.5 2.25 2)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {1.0, 0.9999999999999997, 0.9999999999999997}, true},
    {"color-rec2100-linear rgba32f", "color(rec2100-linear 2.5 2.25 2)", QImage::Format_RGBA32FPx4, -1, {1.5367212488384683, 1.4161047478837296, 1.3431657628747717}, false},
    {"color-rec2100-linear rgba32f-2020", "color(rec2100-linear 2.5 2.25 2)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {1.4649012073622552, 1.4019828977761006, 1.3348398541700344}, false},
    {"color-rec2100-pq rgba64", "color(rec2100-pq 0.5 0.45 0.4)", QImage::Format_RGBA64, -1, {0.7873799550261054, 0.5380864404713642, 0.41434515519587045}, false},
    {"color-rec2100-pq rgba64-p3", "color(rec2100-pq 0.5 0.45 0.4)", QImage::Format_RGBA64, QColorSpace::DisplayP3, {0.750804728917465, 0.5487896465737321, 0.4346517590831188}, false},
    {"color-rec2100-pq rgba32f", "color(rec2100-pq 0.5 0.45 0.4)", QImage::Format_RGBA32FPx4, -1, {0.7873799550261054, 0.5380864404713642, 0.41434515519587045}, false},
    {"color-rec2100-pq rgba32f-2020", "color(rec2100-pq 0.5 0.45 0.4)", QImage::Format_RGBA32FPx4, QColorSpace::Bt2020, {0.719897871811976, 0.5819212173750409, 0.46580537788114085}, false},
};
static const struct { const char *name; const char *svg; QSvgCssColorSchemePreference preference; double rgb[3]; } schemeRows[] = {
    {"no color-scheme, no preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::None, {0.2, 0.4, 0.6}},
    {"no color-scheme, dark preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Dark, {0.2, 0.4, 0.6}},
    {"light, dark preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: light\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Dark, {0.2, 0.4, 0.6}},
    {"dark, no preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::None, {0.8, 0.6, 0.4}},
    {"dark, light preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Light, {0.8, 0.6, 0.4}},
    {"light dark, no preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: light dark\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::None, {0.2, 0.4, 0.6}},
    {"light dark, light preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: light dark\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Light, {0.2, 0.4, 0.6}},
    {"light dark, dark preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: light dark\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Dark, {0.8, 0.6, 0.4}},
    {"dark light, no preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark light\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::None, {0.8, 0.6, 0.4}},
    {"only light, dark preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: only light\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Dark, {0.2, 0.4, 0.6}},
    {"only dark, light preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark only\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Light, {0.8, 0.6, 0.4}},
    {"custom ident only, dark preference", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: foo\"><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::Dark, {0.2, 0.4, 0.6}},
    {"inherited from a group", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark\"><g><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></g></svg>", QSvgCssColorSchemePreference::None, {0.8, 0.6, 0.4}},
    {"normal under a dark group", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\"><g style=\"color-scheme: dark\"><rect width=\"4\" height=\"4\" style=\"color-scheme: normal; fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></g></svg>", QSvgCssColorSchemePreference::None, {0.2, 0.4, 0.6}},
    {"nested in color-mix()", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\" style=\"color-scheme: dark\"><rect width=\"4\" height=\"4\" style=\"fill: color-mix(in srgb, light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4)), white 25%)\"/></svg>", QSvgCssColorSchemePreference::None, {0.8500000000000001, 0.7, 0.55}},
    {"from a style sheet", "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\"><style>rect { color-scheme: dark }</style><rect width=\"4\" height=\"4\" style=\"fill: light-dark(color(srgb 0.2 0.4 0.6), color(srgb 0.8 0.6 0.4))\"/></svg>", QSvgCssColorSchemePreference::None, {0.8, 0.6, 0.4}},
};
static const struct { const char *background; bool black; bool wcagBlack; double ratio; } contrastRows[] = {
    {"#e74c3c", false, true, 3.820000520678017},  // Oklab L 0.6307
    {"#3498db", false, true, 3.153189482342993},  // Oklab L 0.6531
    {"#808080", false, true, 3.9494396480491147},  // Oklab L 0.5999
    {"#ff0000", false, true, 3.997882936374221},  // Oklab L 0.6280
    {"oklch(0.65 0.15 250)", false, true, 3.2256623841268866},  // Oklab L 0.6500
    {"#ff6600", true, true, 7.153245356944874},  // Oklab L 0.6958
    {"oklch(0.75 0.1 100)", true, true, 9.507153137627734},  // Oklab L 0.7500
    {"color(display-p3 0.2 0.3 0.4)", false, false, 8.84008966530431},  // Oklab L 0.4079
    {"oklch(0.6 0.25 145)", false, true, 3.5224941455712933},  // Oklab L 0.6088
};
