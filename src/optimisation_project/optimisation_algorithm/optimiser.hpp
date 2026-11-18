#pragma once

#include <cstddef>
#include <string>

// Abstract base class for first-order optimisers.
//
// An optimiser only knows how to turn a gradient into an update of x. It does
// not know which function is being minimised: the driver loop (see
// minimise.hpp) evaluates the function and passes the gradient in.
class Optimiser {
public:
    virtual ~Optimiser() = default;

    virtual std::string name() const = 0;
    virtual double learning_rate() const = 0;

    // Prepares internal state for a problem of dimension n. Must be called
    // before the first step() and again whenever a new run starts.
    virtual void reset(std::size_t n) = 0;

    // Updates x in place using grad (both point to n doubles, n = last reset).
    virtual void step(double* x, const double* grad) = 0;
};
