// Benchmark of gradient descent vs Adam on classic test functions.
//
// Build and run: see README.md (build.ps1 on Windows, or CMake).
//   ./build/benchmark.exe      -> prints a summary, writes benchmark_report.html
//
// Everything is deterministic (fixed start points, seed 42), so two runs give
// the same output.

#include <cmath>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "src/optimisation_project/benchmark_functions/benchmark_functions.hpp"
#include "src/optimisation_project/optimisation_algorithm/adam.hpp"
#include "src/optimisation_project/optimisation_algorithm/gradient_descent.hpp"
#include "src/optimisation_project/optimisation_algorithm/minimise.hpp"
#include "src/optimisation_project/report/html_report.hpp"

namespace {

// Hyper-parameters per benchmark. Gradient descent needs lr < 2 / (largest
// curvature), which varies a lot between functions; Adam's step size is
// roughly lr per coordinate regardless of the gradient's scale.
struct Setup {
    const char* name;
    double gd_lr;
    double adam_lr;
    double start_x, start_y;  // start point for the 2-D runs
};

const Setup kSetups[] = {
    {"Sphere", 0.1, 0.1, -4.0, 3.5},
    {"Rosenbrock", 1e-3, 0.02, -1.5, 2.0},
    {"Rastrigin", 2e-3, 0.05, 2.7, -1.9},
    {"Ackley", 0.05, 0.1, 3.4, -2.6},
    {"Booth", 0.05, 0.2, -8.0, 8.0},
    {"Beale", 1e-3, 0.05, -1.0, -1.5},
    {"Himmelblau", 0.01, 0.1, -1.0, -0.5},
};

// Returns a pointer into the static table, or nullptr if the name is unknown.
const Setup* find_setup(const std::string& name) {
    for (const Setup& s : kSetups) {
        if (name == s.name) return &s;  // &s: address of the table element
    }
    return nullptr;
}

// Uniform draw in [lo, hi), identical on every platform.
double uniform(std::mt19937& rng, double lo, double hi) {
    const double u = static_cast<double>(rng()) / 4294967296.0;  // rng() in [0, 2^32)
    return lo + (hi - lo) * u;
}

// Owns everything a report needs: the functions and the results. The report
// itself only holds non-owning pointers into this storage.
struct Experiment {
    std::vector<std::unique_ptr<BenchmarkFunction>> functions;
    // unique_ptr so that RunResult addresses stay stable when the vector grows
    // (a vector<RunResult> may reallocate and invalidate pointers to its items).
    std::vector<std::unique_ptr<RunResult>> results;
    ReportSection section;
};

void print_header() {
    std::printf("%-12s %3s  %-17s %8s %6s  %-15s %12s\n", "Function", "n", "Optimiser", "lr",
                "iters", "status", "f - f*");
    std::printf("%s\n", std::string(80, '-').c_str());
}

void print_row(const BenchmarkFunction* f, const RunResult* r) {
    const char* status = r->diverged ? "diverged" : r->converged ? "converged" : "max iterations";
    std::printf("%-12s %3zu  %-17s %8.3g %6zu  %-15s %12.3e\n", f->name().c_str(),
                f->dimension(), r->optimiser_name.c_str(), r->learning_rate, r->iterations, status,
                r->f_history.back() - f->global_minimum_value());
}

// Runs GD and Adam on every function of `exp`. If `fixed_start` is true the
// 2-D start points from the table are used, otherwise a random start drawn
// in the middle half of the search domain.
void run_experiment(Experiment& exp, const RunOptions& options, bool fixed_start,
                    std::mt19937& rng) {
    for (const auto& owned : exp.functions) {
        BenchmarkFunction* f = owned.get();  // borrow a raw pointer from the owner
        const Setup* setup = find_setup(f->name());
        if (setup == nullptr) continue;

        std::vector<double> x0(f->dimension());
        if (fixed_start) {
            x0[0] = setup->start_x;
            x0[1] = setup->start_y;
        } else {
            const double lo = 0.5 * f->lower_bound(), hi = 0.5 * f->upper_bound();
            for (double& xi : x0) xi = uniform(rng, lo, hi);
        }

        GradientDescent gd(setup->gd_lr);
        Adam adam(setup->adam_lr);

        // Both optimisers are used through a base-class pointer: minimise()
        // does not know (or care) which one it is driving.
        Optimiser* optimisers[] = {&gd, &adam};

        ReportEntry entry;
        entry.function = f;
        for (Optimiser* opt : optimisers) {
            exp.results.push_back(
                std::make_unique<RunResult>(minimise(f, opt, x0.data(), options)));
            const RunResult* r = exp.results.back().get();
            entry.runs.push_back(r);
            print_row(f, r);
        }
        exp.section.entries.push_back(entry);
    }
}

}  // namespace

int main() {
    RunOptions options;
    options.max_iterations = 3000;
    options.gradient_tolerance = 1e-8;

    std::mt19937 rng(42);  // fixed seed: reproducible results

    // --- 2-D: every function, with landscapes -------------------------------
    Experiment two_d;
    two_d.functions = make_benchmark_functions(2);
    two_d.section.title = "Two dimensions";
    two_d.section.description =
        "Every benchmark in 2-D from a fixed start point, so the paths can be drawn on the "
        "landscape.";

    // --- 10-D: only the n-dimensional functions -----------------------------
    Experiment ten_d;
    for (auto& f : make_benchmark_functions(10)) {
        if (f->dimension() == 10) ten_d.functions.push_back(std::move(f));  // transfer ownership
    }
    ten_d.section.title = "Ten dimensions";
    ten_d.section.description =
        "The n-dimensional benchmarks in 10-D, from a random start (seed 42) in the middle half "
        "of the search domain. Both optimisers start from the same point.";

    print_header();
    run_experiment(two_d, options, /*fixed_start=*/true, rng);
    run_experiment(ten_d, options, /*fixed_start=*/false, rng);

    const std::string path = "benchmark_report.html";
    if (!write_html_report(path, "Gradient descent vs Adam", {two_d.section, ten_d.section})) {
        std::fprintf(stderr, "Could not write %s\n", path.c_str());
        return 1;
    }
    std::printf("\nReport written to %s\n", path.c_str());
    return 0;
}
