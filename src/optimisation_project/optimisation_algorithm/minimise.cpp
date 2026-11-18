#include "minimise.hpp"

#include <cmath>

namespace {
double norm(const double* v, std::size_t n) {
    double s = 0.0;
    for (const double* p = v; p != v + n; ++p) s += (*p) * (*p);
    return std::sqrt(s);
}
}  // namespace

RunResult minimise(const BenchmarkFunction* function, Optimiser* optimiser, const double* x0,
                   const RunOptions& options) {
    const std::size_t n = function->dimension();

    RunResult result;
    result.optimiser_name = optimiser->name();
    result.learning_rate = optimiser->learning_rate();
    result.dimension = n;

    // Copy the caller's start point: a (begin, end) pointer pair is a valid
    // iterator range for std::vector's constructor.
    std::vector<double> x(x0, x0 + n);
    std::vector<double> grad(n);

    optimiser->reset(n);

    auto record = [&](double f) {
        result.f_history.push_back(f);
        result.trajectory.insert(result.trajectory.end(), x.begin(), x.end());
    };

    double f = function->value(x.data());
    record(f);

    for (std::size_t k = 0; k < options.max_iterations; ++k) {
        function->gradient(x.data(), grad.data());
        if (norm(grad.data(), n) < options.gradient_tolerance) {
            result.converged = true;
            break;
        }

        optimiser->step(x.data(), grad.data());  // virtual call -> GD or Adam
        f = function->value(x.data());
        ++result.iterations;

        if (!std::isfinite(f) || std::fabs(f) > options.divergence_limit) {
            result.diverged = true;
            break;
        }
        record(f);
    }

    result.x_final = x;
    return result;
}
