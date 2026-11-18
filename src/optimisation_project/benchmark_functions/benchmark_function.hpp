#pragma once

#include <cstddef>
#include <string>
#include <utility>

// Abstract base class for every benchmark (test) function f : R^n -> R.
//
// Points are passed as raw pointers to contiguous arrays of doubles:
//   - `const double* x`  : read-only input, x[0] .. x[n-1]
//   - `double* grad`     : output buffer, written by the function
// The caller owns the memory (e.g. a std::vector<double> and its .data()),
// the function only borrows it for the duration of the call.
class BenchmarkFunction {
public:
    BenchmarkFunction(std::string name, std::size_t dimension,
                      double lower_bound, double upper_bound)
        : name_(std::move(name)),
          dimension_(dimension),
          lower_bound_(lower_bound),
          upper_bound_(upper_bound) {}

    // Virtual destructor: required because objects are deleted through a
    // base-class pointer (std::unique_ptr<BenchmarkFunction>).
    virtual ~BenchmarkFunction() = default;

    // f(x). `x` must point to `dimension()` doubles.
    virtual double value(const double* x) const = 0;

    // Writes grad f(x) into `grad` (must point to `dimension()` doubles).
    virtual void gradient(const double* x, double* grad) const = 0;

    // Writes a known global minimiser into `x_star` (`dimension()` doubles).
    virtual void global_minimiser(double* x_star) const = 0;

    // f at the global minimum (0 for all functions provided here).
    virtual double global_minimum_value() const { return 0.0; }

    const std::string& name() const { return name_; }
    std::size_t dimension() const { return dimension_; }

    // Usual search domain [lower, upper]^n, handy for random initialisation.
    double lower_bound() const { return lower_bound_; }
    double upper_bound() const { return upper_bound_; }

private:
    std::string name_;
    std::size_t dimension_;
    double lower_bound_;
    double upper_bound_;
};
