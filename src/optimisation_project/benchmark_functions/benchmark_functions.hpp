#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "benchmark_function.hpp"

// ---------------------------------------------------------------------------
// n-dimensional functions
// ---------------------------------------------------------------------------

// f(x) = sum x_i^2                         min 0 at (0,...,0)   convex
class Sphere : public BenchmarkFunction {
public:
    explicit Sphere(std::size_t n);
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// f(x) = sum_{i<n-1} 100 (x_{i+1} - x_i^2)^2 + (1 - x_i)^2
//                                          min 0 at (1,...,1)   narrow curved valley
class Rosenbrock : public BenchmarkFunction {
public:
    explicit Rosenbrock(std::size_t n);
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// f(x) = 10 n + sum (x_i^2 - 10 cos(2 pi x_i))
//                                          min 0 at (0,...,0)   many local minima
class Rastrigin : public BenchmarkFunction {
public:
    explicit Rastrigin(std::size_t n);
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// f(x) = -20 exp(-0.2 sqrt(sum x_i^2 / n)) - exp(sum cos(2 pi x_i) / n) + 20 + e
//                                          min 0 at (0,...,0)   flat outer region, many local minima
class Ackley : public BenchmarkFunction {
public:
    explicit Ackley(std::size_t n);
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// ---------------------------------------------------------------------------
// 2-dimensional functions
// ---------------------------------------------------------------------------

// f(x,y) = (x + 2y - 7)^2 + (2x + y - 5)^2         min 0 at (1, 3)   convex quadratic
class Booth : public BenchmarkFunction {
public:
    Booth();
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// f(x,y) = (1.5 - x + xy)^2 + (2.25 - x + xy^2)^2 + (2.625 - x + xy^3)^2
//                                                  min 0 at (3, 0.5)  sharp ridges
class Beale : public BenchmarkFunction {
public:
    Beale();
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;
};

// f(x,y) = (x^2 + y - 11)^2 + (x + y^2 - 7)^2
//   min 0 at (3, 2), (-2.805118, 3.131312), (-3.779310, -3.283186), (3.584428, -1.848126)
class Himmelblau : public BenchmarkFunction {
public:
    Himmelblau();
    double value(const double* x) const override;
    void gradient(const double* x, double* grad) const override;
    void global_minimiser(double* x_star) const override;  // writes (3, 2)
};

// ---------------------------------------------------------------------------
// List of benchmarks
// ---------------------------------------------------------------------------

// Returns one instance of every benchmark. The n-dimensional ones are built
// with dimension `n`; the 2-D ones are always 2-D.
//
// Ownership: each std::unique_ptr owns its object and deletes it automatically
// when the vector is destroyed. Use `ptr.get()` (or `*ptr`) to hand a
// non-owning BenchmarkFunction* / BenchmarkFunction& to an optimiser.
std::vector<std::unique_ptr<BenchmarkFunction>> make_benchmark_functions(std::size_t n);
