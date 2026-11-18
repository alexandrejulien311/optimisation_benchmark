#include "html_report.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <ostream>

namespace {

constexpr std::size_t kMaxSeries = 3;
constexpr double kGapFloor = 1e-16;  // f - f* is clamped here before log10

int g_clip_counter = 0;  // unique ids for SVG clip paths

const char* series_var(std::size_t i) {
    static const char* vars[kMaxSeries] = {"var(--series-1)", "var(--series-2)", "var(--series-3)"};
    return vars[i % kMaxSeries];
}

const char* series_class(std::size_t i) {
    static const char* cls[kMaxSeries] = {"s1", "s2", "s3"};
    return cls[i % kMaxSeries];
}

// log10 of the optimality gap; NaN for non-finite values.
double log_gap(double f, double f_star) {
    if (!std::isfinite(f)) return std::numeric_limits<double>::quiet_NaN();
    return std::log10(std::max(f - f_star, kGapFloor));
}

// "Nice" tick step (1, 2 or 5 times a power of ten) for roughly `count` ticks.
double nice_step(double span, int count) {
    const double raw = span / count;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double r = raw / mag;
    const double nice = r < 1.5 ? 1.0 : r < 3.5 ? 2.0 : r < 7.5 ? 5.0 : 10.0;
    return nice * mag;
}

std::string fmt_sci(double v) {
    if (!std::isfinite(v)) return "n/a";
    return std::format("{:.3e}", v);
}

std::string fmt_short(double v) {
    if (std::fabs(v) < 1e-12) return "0";
    return std::format("{:.3g}", v);
}

std::string fmt_point(const double* p, std::size_t n) {
    std::string s = "(";
    for (std::size_t i = 0; i < n; ++i) {
        if (i) s += ", ";
        s += fmt_short(p[i]);
    }
    return s + ")";
}

std::string status_of(const RunResult& r) {
    if (r.diverged) return "diverged";
    if (r.converged) return "converged";
    return "max iterations";
}

void write_legend(std::ostream& out, const std::vector<const RunResult*>& runs) {
    out << "<div class=\"legend\">";
    for (std::size_t i = 0; i < runs.size(); ++i) {
        out << std::format(
            "<span class=\"key\"><span class=\"swatch\" style=\"background:{}\"></span>{} "
            "<span class=\"muted\">(lr = {})</span></span>",
            series_var(i), runs[i]->optimiser_name, fmt_short(runs[i]->learning_rate));
    }
    out << "</div>\n";
}

// ---------------------------------------------------------------------------
// Convergence chart: log10(f - f*) vs iteration
// ---------------------------------------------------------------------------
void write_convergence_svg(std::ostream& out, const BenchmarkFunction* f,
                           const std::vector<const RunResult*>& runs) {
    constexpr double W = 560, H = 320, ml = 56, mr = 64, mt = 14, mb = 42;
    const double pw = W - ml - mr, ph = H - mt - mb;
    const double f_star = f->global_minimum_value();

    std::size_t x_max = 1;
    double y_lo = std::numeric_limits<double>::infinity();
    double y_hi = -y_lo;
    for (const RunResult* r : runs) {
        x_max = std::max(x_max, r->f_history.size() - 1);
        for (double v : r->f_history) {
            const double y = log_gap(v, f_star);
            if (std::isnan(y)) continue;
            y_lo = std::min(y_lo, y);
            y_hi = std::max(y_hi, y);
        }
    }
    y_lo = std::floor(y_lo);
    y_hi = std::min(std::ceil(y_hi), 14.0);
    if (y_hi <= y_lo) y_hi = y_lo + 1;
    const int y_step = std::max(1, static_cast<int>(std::ceil((y_hi - y_lo) / 6.0)));

    auto sx = [&](double k) { return ml + pw * k / static_cast<double>(x_max); };
    auto sy = [&](double y) { return mt + ph * (1.0 - (y - y_lo) / (y_hi - y_lo)); };

    const int clip = ++g_clip_counter;
    out << std::format(
        "<svg class=\"chart\" viewBox=\"0 0 {} {}\" role=\"img\" aria-label=\"Convergence of "
        "{}\">\n<defs><clipPath id=\"c{}\"><rect x=\"{}\" y=\"{}\" width=\"{}\" "
        "height=\"{}\"/></clipPath></defs>\n",
        W, H, f->name(), clip, ml, mt - 2, pw + 2, ph + 4);

    // Horizontal grid + y labels.
    for (int y = static_cast<int>(y_lo); y <= static_cast<int>(y_hi); y += y_step) {
        out << std::format(
            "<line class=\"grid\" x1=\"{0}\" x2=\"{1}\" y1=\"{2:.1f}\" y2=\"{2:.1f}\"/>"
            "<text class=\"tick\" x=\"{3}\" y=\"{2:.1f}\" dy=\"0.32em\" "
            "text-anchor=\"end\">1e{4}</text>\n",
            ml, ml + pw, sy(y), ml - 8, y);
    }
    // x ticks.
    const double xs = nice_step(static_cast<double>(x_max), 5);
    for (double k = 0; k <= static_cast<double>(x_max) + 1e-9; k += xs) {
        out << std::format(
            "<text class=\"tick\" x=\"{:.1f}\" y=\"{}\" text-anchor=\"middle\">{}</text>\n", sx(k),
            mt + ph + 18, static_cast<long long>(k));
    }
    out << std::format("<line class=\"axis\" x1=\"{0}\" x2=\"{1}\" y1=\"{2}\" y2=\"{2}\"/>\n", ml,
                       ml + pw, mt + ph);
    out << std::format(
        "<text class=\"axis-title\" x=\"{}\" y=\"{}\" text-anchor=\"middle\">iteration</text>\n",
        ml + pw / 2, H - 4);
    out << std::format(
        "<text class=\"axis-title\" transform=\"translate(14 {}) rotate(-90)\" "
        "text-anchor=\"middle\">f(x) − f*</text>\n",
        mt + ph / 2);

    // Series lines + end labels next to the end dot (nudged apart if they collide).
    std::vector<double> label_x, label_y;
    out << std::format("<g clip-path=\"url(#c{})\">\n", clip);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const RunResult* r = runs[i];
        out << std::format("<polyline class=\"line\" stroke=\"{}\" points=\"", series_var(i));
        double last_x = ml, last_y = mt + ph;
        for (std::size_t k = 0; k < r->f_history.size(); ++k) {
            const double y = log_gap(r->f_history[k], f_star);
            if (std::isnan(y)) break;
            last_x = sx(static_cast<double>(k));
            last_y = sy(std::min(y, y_hi));
            out << std::format("{:.1f},{:.1f} ", last_x, last_y);
        }
        out << "\"/>\n";
        out << std::format(
            "<circle class=\"dot\" cx=\"{:.1f}\" cy=\"{:.1f}\" r=\"4\" fill=\"{}\"/>\n", last_x,
            last_y, series_var(i));
        label_x.push_back(last_x + 8);
        label_y.push_back(std::clamp(last_y, mt + 6, mt + ph - 6));
    }
    out << "</g>\n";
    // Two labels closer than one text line and overlapping horizontally:
    // stack them, keeping the pair inside the plot area.
    if (label_y.size() == 2 && std::fabs(label_y[0] - label_y[1]) < 14 &&
        std::fabs(label_x[0] - label_x[1]) < 40) {
        const bool first_above = label_y[0] <= label_y[1];
        double mid = std::clamp((label_y[0] + label_y[1]) / 2, mt + 7, mt + ph - 7);
        label_y[0] = mid + (first_above ? -7 : 7);
        label_y[1] = mid + (first_above ? 7 : -7);
        const double x = std::max(label_x[0], label_x[1]);
        label_x[0] = label_x[1] = x;
    }
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const std::string short_name = runs[i]->optimiser_name == "Gradient descent"
                                           ? std::string("GD")
                                           : runs[i]->optimiser_name;
        out << std::format(
            "<text class=\"direct\" x=\"{:.1f}\" y=\"{:.1f}\" dy=\"0.32em\">{}</text>\n",
            label_x[i], label_y[i], short_name);
    }

    // Hover bands: transparent columns with a native tooltip.
    constexpr int bands = 60;
    const double bw = pw / bands;
    for (int b = 0; b < bands; ++b) {
        const std::size_t k = static_cast<std::size_t>(
            std::round((b + 0.5) / bands * static_cast<double>(x_max)));
        std::string tip = std::format("iteration {}", k);
        for (const RunResult* r : runs) {
            const std::string v =
                k < r->f_history.size() ? fmt_sci(r->f_history[k] - f_star) : std::string("stopped");
            tip += std::format("\n{}: f − f* = {}", r->optimiser_name, v);
        }
        out << std::format(
            "<rect class=\"band\" x=\"{:.1f}\" y=\"{}\" width=\"{:.1f}\" height=\"{}\">"
            "<title>{}</title></rect>\n",
            ml + b * bw, mt, bw, ph, tip);
    }
    out << "</svg>\n";
}

// ---------------------------------------------------------------------------
// Landscape (2-D only): shaded log(f) + optimisation paths
// ---------------------------------------------------------------------------
void write_landscape_svg(std::ostream& out, const BenchmarkFunction* f,
                         const std::vector<const RunResult*>& runs) {
    constexpr double S = 340, m = 36, P = S - 2 * m;
    constexpr int G = 56;  // grid resolution
    const double lb = f->lower_bound(), ub = f->upper_bound();

    // View box: bounding box of the paths and of x*, padded, made square.
    double x_star[2];
    f->global_minimiser(x_star);
    double x0 = x_star[0], x1 = x_star[0], y0 = x_star[1], y1 = x_star[1];
    for (const RunResult* r : runs) {
        for (std::size_t k = 0; k < r->f_history.size(); ++k) {
            const double* p = r->point(k);
            if (!std::isfinite(p[0]) || !std::isfinite(p[1])) continue;
            x0 = std::min(x0, std::clamp(p[0], lb, ub));
            x1 = std::max(x1, std::clamp(p[0], lb, ub));
            y0 = std::min(y0, std::clamp(p[1], lb, ub));
            y1 = std::max(y1, std::clamp(p[1], lb, ub));
        }
    }
    double span = std::max({x1 - x0, y1 - y0, 0.1 * (ub - lb)}) * 1.3;
    const double cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
    const double vx0 = cx - span / 2, vy0 = cy - span / 2;

    auto sx = [&](double x) { return m + P * (x - vx0) / span; };
    auto sy = [&](double y) { return m + P * (1.0 - (y - vy0) / span); };

    // Evaluate f on the grid.
    std::vector<double> values(G * G);
    double vmin = std::numeric_limits<double>::infinity(), vmax = -vmin;
    double pt[2];
    for (int j = 0; j < G; ++j) {
        for (int i = 0; i < G; ++i) {
            pt[0] = vx0 + (i + 0.5) * span / G;
            pt[1] = vy0 + (j + 0.5) * span / G;
            const double v = f->value(pt);
            values[j * G + i] = v;
            vmin = std::min(vmin, v);
            vmax = std::max(vmax, v);
        }
    }
    const double lmax = std::log1p(vmax - vmin);

    const int clip = ++g_clip_counter;
    out << std::format(
        "<svg class=\"chart landscape\" viewBox=\"0 0 {0} {0}\" role=\"img\" "
        "aria-label=\"Landscape of {1} with optimisation paths\">\n"
        "<defs><clipPath id=\"c{2}\"><rect x=\"{3}\" y=\"{3}\" width=\"{4}\" "
        "height=\"{4}\"/></clipPath></defs>\n<g class=\"heat\">\n",
        S, f->name(), clip, m, P);
    const double cell = P / G;
    for (int j = 0; j < G; ++j) {
        for (int i = 0; i < G; ++i) {
            const double t = lmax > 0 ? std::log1p(values[j * G + i] - vmin) / lmax : 0.0;
            out << std::format(
                "<rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.2f}\" height=\"{:.2f}\" "
                "style=\"--t:{:.0f}%\"/>",
                m + i * cell, m + (G - 1 - j) * cell, cell + 0.5, cell + 0.5, 2 + 48 * t);
        }
        out << '\n';
    }
    out << "</g>\n";

    // Axis tick labels (corners + middle).
    for (int t = 0; t <= 2; ++t) {
        const double vx = vx0 + span * t / 2.0, vy = vy0 + span * t / 2.0;
        out << std::format(
            "<text class=\"tick\" x=\"{:.1f}\" y=\"{}\" text-anchor=\"middle\">{}</text>"
            "<text class=\"tick\" x=\"{}\" y=\"{:.1f}\" dy=\"0.32em\" "
            "text-anchor=\"end\">{}</text>\n",
            sx(vx), m + P + 16, fmt_short(vx), m - 6, sy(vy), fmt_short(vy));
    }

    // Paths: a surface-coloured halo under each coloured line.
    out << std::format("<g clip-path=\"url(#c{})\">\n", clip);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const RunResult* r = runs[i];
        std::string pts;
        for (std::size_t k = 0; k < r->f_history.size(); ++k) {
            const double* p = r->point(k);
            pts += std::format("{:.1f},{:.1f} ", sx(p[0]), sy(p[1]));
        }
        out << std::format("<polyline class=\"halo\" points=\"{}\"/>\n", pts);
        out << std::format("<polyline class=\"line\" stroke=\"{}\" points=\"{}\"/>\n",
                           series_var(i), pts);
    }
    // Markers: hollow = start, filled = end.
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const RunResult* r = runs[i];
        const double* start = r->point(0);
        const double* end = r->point(r->f_history.size() - 1);
        out << std::format(
            "<circle class=\"start {}\" cx=\"{:.1f}\" cy=\"{:.1f}\" r=\"5\"><title>Start "
            "{}</title></circle>\n",
            series_class(i), sx(start[0]), sy(start[1]), fmt_point(start, 2));
        out << std::format(
            "<circle class=\"dot\" cx=\"{:.1f}\" cy=\"{:.1f}\" r=\"5\" fill=\"{}\"><title>{} end "
            "{}  f = {}</title></circle>\n",
            sx(end[0]), sy(end[1]), series_var(i), r->optimiser_name, fmt_point(end, 2),
            fmt_sci(r->f_history.back()));
    }
    const double xs = sx(x_star[0]), ys = sy(x_star[1]);
    out << std::format(
        "<g class=\"xstar\"><path d=\"M{0:.1f} {1:.1f}l8 8M{2:.1f} {1:.1f}l-8 8\"/>"
        "<text x=\"{3:.1f}\" y=\"{4:.1f}\">x*</text><title>Global minimum x* = {5}</title></g>\n",
        xs - 4, ys - 4, xs + 4, xs + 8, ys - 6, fmt_point(x_star, 2));
    out << "</g>\n</svg>\n";
}

constexpr const char* kCss = R"CSS(
:root {
  color-scheme: light;
  --page: #f9f9f7; --surface: #fcfcfb;
  --ink: #0b0b0b; --ink-2: #52514e; --muted: #898781;
  --grid: #e1e0d9; --axis: #c3c2b7; --border: rgba(11,11,11,0.10);
  --series-1: #2a78d6; --series-2: #eb6834; --series-3: #1baf7a;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    color-scheme: dark;
    --page: #0d0d0d; --surface: #1a1a19;
    --ink: #f0efec; --ink-2: #c3c2b7; --muted: #898781;
    --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
    --series-1: #3987e5; --series-2: #d95926; --series-3: #199e70;
  }
}
:root[data-theme="dark"] {
  color-scheme: dark;
  --page: #0d0d0d; --surface: #1a1a19;
  --ink: #f0efec; --ink-2: #c3c2b7; --muted: #898781;
  --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
  --series-1: #3987e5; --series-2: #d95926; --series-3: #199e70;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--page); color: var(--ink);
  font: 15px/1.5 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 1180px; margin: 0 auto; padding: 32px 16px 64px; }
h1 { font-size: 26px; margin: 0 0 4px; }
h2 { font-size: 20px; margin: 40px 0 4px; }
h3 { font-size: 16px; margin: 0; }
p { margin: 0 0 12px; color: var(--ink-2); }
.muted { color: var(--muted); }
.card { background: var(--surface); border: 1px solid var(--border); border-radius: 12px;
  padding: 16px; margin-top: 16px; }
.card-head { display: flex; flex-wrap: wrap; gap: 4px 16px; align-items: baseline;
  justify-content: space-between; }
.sub { font-size: 13px; color: var(--muted); }
.legend { display: flex; flex-wrap: wrap; gap: 4px 16px; font-size: 13px; color: var(--ink-2); }
.key { display: inline-flex; align-items: center; gap: 6px; }
.swatch { width: 12px; height: 12px; border-radius: 3px; display: inline-block; }
.plots { display: flex; flex-wrap: wrap; gap: 16px; margin-top: 8px; }
.chart { display: block; width: 100%; height: auto; }
.plots > .chart { flex: 1 1 420px; min-width: 0; max-width: 620px; }
.plots > .landscape { flex: 0 1 340px; }
.chart text { font-size: 11px; fill: var(--muted); font-variant-numeric: tabular-nums; }
.chart .axis-title { fill: var(--ink-2); }
.chart .direct { fill: var(--ink-2); font-size: 12px; }
.grid { stroke: var(--grid); stroke-width: 1; }
.axis { stroke: var(--axis); stroke-width: 1; }
.line { fill: none; stroke-width: 2; stroke-linejoin: round; stroke-linecap: round; }
.halo { fill: none; stroke: var(--surface); stroke-width: 5; stroke-linejoin: round; opacity: 0.85; }
.dot { stroke: var(--surface); stroke-width: 2; }
.start { fill: var(--surface); stroke-width: 2; }
.start.s1 { stroke: var(--series-1); } .start.s2 { stroke: var(--series-2); }
.start.s3 { stroke: var(--series-3); }
/* Opaque cells (ink mixed into the surface), so overlapping edges leave no seams. */
.heat rect { fill: color-mix(in srgb, var(--ink) var(--t), var(--surface)); }
.xstar path { stroke: var(--ink); stroke-width: 2; stroke-linecap: round; }
.xstar text { fill: var(--ink); font-size: 12px; font-weight: 600; }
.band { fill: var(--ink); fill-opacity: 0; }
.band:hover { fill-opacity: 0.06; }
.table-wrap { overflow-x: auto; margin-top: 16px; }
table { border-collapse: collapse; width: 100%; font-size: 13px; background: var(--surface); }
th, td { padding: 6px 10px; border-bottom: 1px solid var(--grid); text-align: left; white-space: nowrap; }
th { color: var(--ink-2); font-weight: 600; }
td.num { text-align: right; font-variant-numeric: tabular-nums; }
)CSS";

}  // namespace

bool write_html_report(const std::string& path, const std::string& title,
                       const std::vector<ReportSection>& sections) {
    std::ofstream out(path);
    if (!out) return false;

    out << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << std::format("<title>{}</title>\n<style>{}</style>\n</head>\n<body>\n<main>\n", title,
                       kCss)
        << std::format("<h1>{}</h1>\n", title)
        << "<p>Convergence plots show the optimality gap f(x<sub>k</sub>) − f* on a log scale "
           "(hover for values). Landscapes shade log f (darker = higher); hollow circles mark "
           "the start, filled circles the end, × the global minimum.</p>\n";

    for (const ReportSection& section : sections) {
        out << std::format("<h2>{}</h2>\n<p>{}</p>\n", section.title, section.description);
        for (const ReportEntry& e : section.entries) {
            const BenchmarkFunction* f = e.function;
            std::vector<const RunResult*> runs(
                e.runs.begin(), e.runs.begin() + std::min(e.runs.size(), kMaxSeries));
            out << "<section class=\"card\">\n<div class=\"card-head\">"
                << std::format("<h3>{} <span class=\"sub\">n = {}</span></h3>", f->name(),
                               f->dimension());
            write_legend(out, runs);
            out << "</div>\n<div class=\"plots\">\n";
            if (f->dimension() == 2) write_landscape_svg(out, f, runs);
            write_convergence_svg(out, f, runs);
            out << "</div>\n</section>\n";
        }
    }

    // Summary table (also the accessible, non-graphical view of the results).
    out << "<h2>Summary</h2>\n<div class=\"table-wrap\"><table>\n<thead><tr><th>Function</th>"
           "<th>n</th><th>Optimiser</th><th>lr</th><th>Iterations</th><th>Status</th>"
           "<th>Final f − f*</th><th>‖x − x*‖</th></tr></thead>\n<tbody>\n";
    for (const ReportSection& section : sections) {
        for (const ReportEntry& e : section.entries) {
            const BenchmarkFunction* f = e.function;
            std::vector<double> x_star(f->dimension());
            f->global_minimiser(x_star.data());
            for (const RunResult* r : e.runs) {
                double dist = 0.0;
                for (std::size_t i = 0; i < f->dimension(); ++i) {
                    const double d = r->x_final[i] - x_star[i];
                    dist += d * d;
                }
                out << std::format(
                    "<tr><td>{}</td><td class=\"num\">{}</td><td>{}</td><td "
                    "class=\"num\">{}</td><td class=\"num\">{}</td><td>{}</td><td "
                    "class=\"num\">{}</td><td class=\"num\">{}</td></tr>\n",
                    f->name(), f->dimension(), r->optimiser_name, fmt_short(r->learning_rate),
                    r->iterations, status_of(*r),
                    fmt_sci(r->f_history.back() - f->global_minimum_value()),
                    fmt_sci(std::sqrt(dist)));
            }
        }
    }
    out << "</tbody></table></div>\n</main>\n</body>\n</html>\n";
    return static_cast<bool>(out);
}
