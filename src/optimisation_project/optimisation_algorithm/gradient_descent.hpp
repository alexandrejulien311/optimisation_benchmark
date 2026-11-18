#pragma once

#include "optimiser.hpp"

// Plain gradient descent:  x <- x - lr * grad f(x)
class GradientDescent : public Optimiser {
public:
    explicit GradientDescent(double learning_rate) : lr_(learning_rate) {}

    std::string name() const override { return "Gradient descent"; }
    double learning_rate() const override { return lr_; }

    void reset(std::size_t n) override { n_ = n; }  // stateless apart from n
    void step(double* x, const double* grad) override;

private:
    double lr_;
    std::size_t n_ = 0;
};
