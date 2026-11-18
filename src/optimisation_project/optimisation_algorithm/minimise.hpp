#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../benchmark_functions/benchmark_function.hpp"
#include "optimiser.hpp"

struct RunOptions {
    std::size_t max_iterations = 2000;
    double gradient_tolerance = 1e-8;  // stop when ||grad|| < tolerance
    double divergence_limit = 1e12;    // stop when |f| exceeds this (or is NaN/inf)
};

struct RunResult {
    std::string optimiser_name;
    double learning_rate = 0.0;
    std::size_t dimension = 0;

    std::vector<double> f_history;   // f(x_k), k = 0 .. iterations
    std::vector<double> trajectory;  // x_0, x_1, ... flattened: point k is at [k*dimension]
    std::vector<double> x_final;

    std::size_t iterations = 0;
    bool converged = false;  // gradient tolerance reached
    bool diverged = false;   // f blew up

    // Non-owning pointer to point k inside `trajectory` (valid while the
    // RunResult is alive and the vector is not resized).
    const double* point(std::size_t k) const { return trajectory.data() + k * dimension; }
};

// Runs `optimiser` on `function` starting from `x0` (dimension() doubles).
// Both objects are borrowed (non-owning pointers); the optimiser is reset first.
RunResult minimise(const BenchmarkFunction* function, Optimiser* optimiser, const double* x0,
                   const RunOptions& options = {});
