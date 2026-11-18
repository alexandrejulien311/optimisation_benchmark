#include "benchmark_functions.hpp"

#include <cmath>
#include <numbers>

namespace {
constexpr double pi = std::numbers::pi;
constexpr double two_pi = 2.0 * std::numbers::pi;
}  // namespace

// ===========================================================================
// Sphere
// ===========================================================================

Sphere::Sphere(std::size_t n) : BenchmarkFunction("Sphere", n, -5.12, 5.12) {}

double Sphere::value(const double* x) const {
    // Pointer-walking loop: `p` starts at the first element and is advanced
    // with ++p until it reaches one-past-the-last element (`end`).
    const double* end = x + dimension();
    double sum = 0.0;
    for (const double* p = x; p != end; ++p) {
        sum += (*p) * (*p);
    }
    return sum;
}

void Sphere::gradient(const double* x, double* grad) const {
    const double* end = x + dimension();
    // Two pointers advanced in lock-step: one reads, one writes.
    for (const double* p = x; p != end; ++p, ++grad) {
        *grad = 2.0 * (*p);
    }
}

void Sphere::global_minimiser(double* x_star) const {
    for (std::size_t i = 0; i < dimension(); ++i) x_star[i] = 0.0;
}

// ===========================================================================
// Rosenbrock
// ===========================================================================

Rosenbrock::Rosenbrock(std::size_t n) : BenchmarkFunction("Rosenbrock", n, -2.048, 2.048) {}

double Rosenbrock::value(const double* x) const {
    double sum = 0.0;
    for (std::size_t i = 0; i + 1 < dimension(); ++i) {
        // x[i] is exactly *(x + i): indexing is pointer arithmetic.
        const double a = x[i + 1] - x[i] * x[i];
        const double b = 1.0 - x[i];
        sum += 100.0 * a * a + b * b;
    }
    return sum;
}

void Rosenbrock::gradient(const double* x, double* grad) const {
    const std::size_t n = dimension();
    for (std::size_t i = 0; i < n; ++i) grad[i] = 0.0;

    // Each term i touches coordinates i and i+1.
    for (std::size_t i = 0; i + 1 < n; ++i) {
        const double a = x[i + 1] - x[i] * x[i];
        grad[i] += -400.0 * x[i] * a - 2.0 * (1.0 - x[i]);
        grad[i + 1] += 200.0 * a;
    }
}

void Rosenbrock::global_minimiser(double* x_star) const {
    for (std::size_t i = 0; i < dimension(); ++i) x_star[i] = 1.0;
}

// ===========================================================================
// Rastrigin
// ===========================================================================

Rastrigin::Rastrigin(std::size_t n) : BenchmarkFunction("Rastrigin", n, -5.12, 5.12) {}

double Rastrigin::value(const double* x) const {
    double sum = 10.0 * static_cast<double>(dimension());
    for (std::size_t i = 0; i < dimension(); ++i) {
        sum += x[i] * x[i] - 10.0 * std::cos(two_pi * x[i]);
    }
    return sum;
}

void Rastrigin::gradient(const double* x, double* grad) const {
    for (std::size_t i = 0; i < dimension(); ++i) {
        grad[i] = 2.0 * x[i] + 20.0 * pi * std::sin(two_pi * x[i]);
    }
}

void Rastrigin::global_minimiser(double* x_star) const {
    for (std::size_t i = 0; i < dimension(); ++i) x_star[i] = 0.0;
}

// ===========================================================================
// Ackley
// ===========================================================================

Ackley::Ackley(std::size_t n) : BenchmarkFunction("Ackley", n, -32.768, 32.768) {}

double Ackley::value(const double* x) const {
    const double n = static_cast<double>(dimension());
    double sum_sq = 0.0;
    double sum_cos = 0.0;
    for (std::size_t i = 0; i < dimension(); ++i) {
        sum_sq += x[i] * x[i];
        sum_cos += std::cos(two_pi * x[i]);
    }
    return -20.0 * std::exp(-0.2 * std::sqrt(sum_sq / n)) - std::exp(sum_cos / n) + 20.0 +
           std::numbers::e;
}

void Ackley::gradient(const double* x, double* grad) const {
    const double n = static_cast<double>(dimension());
    double sum_sq = 0.0;
    double sum_cos = 0.0;
    for (std::size_t i = 0; i < dimension(); ++i) {
        sum_sq += x[i] * x[i];
        sum_cos += std::cos(two_pi * x[i]);
    }
    const double r = std::sqrt(sum_sq / n);
    const double exp_cos = std::exp(sum_cos / n);
    // The first term is not differentiable at r = 0 (cone tip); use 0 there.
    const double coef_sq = (r > 0.0) ? 4.0 * std::exp(-0.2 * r) / (n * r) : 0.0;
    for (std::size_t i = 0; i < dimension(); ++i) {
        grad[i] = coef_sq * x[i] + (two_pi / n) * std::sin(two_pi * x[i]) * exp_cos;
    }
}

void Ackley::global_minimiser(double* x_star) const {
    for (std::size_t i = 0; i < dimension(); ++i) x_star[i] = 0.0;
}

// ===========================================================================
// Booth
// ===========================================================================

Booth::Booth() : BenchmarkFunction("Booth", 2, -10.0, 10.0) {}

double Booth::value(const double* x) const {
    const double a = x[0] + 2.0 * x[1] - 7.0;
    const double b = 2.0 * x[0] + x[1] - 5.0;
    return a * a + b * b;
}

void Booth::gradient(const double* x, double* grad) const {
    const double a = x[0] + 2.0 * x[1] - 7.0;
    const double b = 2.0 * x[0] + x[1] - 5.0;
    grad[0] = 2.0 * a + 4.0 * b;
    grad[1] = 4.0 * a + 2.0 * b;
}

void Booth::global_minimiser(double* x_star) const {
    x_star[0] = 1.0;
    x_star[1] = 3.0;
}

// ===========================================================================
// Beale
// ===========================================================================

Beale::Beale() : BenchmarkFunction("Beale", 2, -4.5, 4.5) {}

double Beale::value(const double* x) const {
    const double u = x[0], v = x[1];
    const double t1 = 1.5 - u + u * v;
    const double t2 = 2.25 - u + u * v * v;
    const double t3 = 2.625 - u + u * v * v * v;
    return t1 * t1 + t2 * t2 + t3 * t3;
}

void Beale::gradient(const double* x, double* grad) const {
    const double u = x[0], v = x[1];
    const double t1 = 1.5 - u + u * v;
    const double t2 = 2.25 - u + u * v * v;
    const double t3 = 2.625 - u + u * v * v * v;
    grad[0] = 2.0 * t1 * (v - 1.0) + 2.0 * t2 * (v * v - 1.0) + 2.0 * t3 * (v * v * v - 1.0);
    grad[1] = 2.0 * t1 * u + 4.0 * t2 * u * v + 6.0 * t3 * u * v * v;
}

void Beale::global_minimiser(double* x_star) const {
    x_star[0] = 3.0;
    x_star[1] = 0.5;
}

// ===========================================================================
// Himmelblau
// ===========================================================================

Himmelblau::Himmelblau() : BenchmarkFunction("Himmelblau", 2, -5.0, 5.0) {}

double Himmelblau::value(const double* x) const {
    const double a = x[0] * x[0] + x[1] - 11.0;
    const double b = x[0] + x[1] * x[1] - 7.0;
    return a * a + b * b;
}

void Himmelblau::gradient(const double* x, double* grad) const {
    const double a = x[0] * x[0] + x[1] - 11.0;
    const double b = x[0] + x[1] * x[1] - 7.0;
    grad[0] = 4.0 * x[0] * a + 2.0 * b;
    grad[1] = 2.0 * a + 4.0 * x[1] * b;
}

void Himmelblau::global_minimiser(double* x_star) const {
    x_star[0] = 3.0;
    x_star[1] = 2.0;
}

// ===========================================================================
// List
// ===========================================================================

std::vector<std::unique_ptr<BenchmarkFunction>> make_benchmark_functions(std::size_t n) {
    std::vector<std::unique_ptr<BenchmarkFunction>> list;
    // std::make_unique<Derived> returns unique_ptr<Derived>, which converts
    // implicitly to unique_ptr<BenchmarkFunction>: this is polymorphism through
    // a base-class pointer. Calls like list[i]->value(x) are dispatched at run
    // time to the right derived class via the virtual table.
    list.push_back(std::make_unique<Sphere>(n));
    list.push_back(std::make_unique<Rosenbrock>(n));
    list.push_back(std::make_unique<Rastrigin>(n));
    list.push_back(std::make_unique<Ackley>(n));
    list.push_back(std::make_unique<Booth>());
    list.push_back(std::make_unique<Beale>());
    list.push_back(std::make_unique<Himmelblau>());
    return list;
}
