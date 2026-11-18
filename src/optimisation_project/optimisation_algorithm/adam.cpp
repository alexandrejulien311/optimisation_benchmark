#include "adam.hpp"

#include <cmath>
#include <utility>

Adam::Adam(double learning_rate, double beta1, double beta2, double epsilon)
    : lr_(learning_rate), beta1_(beta1), beta2_(beta2), epsilon_(epsilon) {}

Adam::~Adam() { release(); }

// Move constructor: steal the buffers, leave `other` empty but valid.
// std::exchange(a, b) sets a = b and returns the old a.
Adam::Adam(Adam&& other) noexcept
    : lr_(other.lr_),
      beta1_(other.beta1_),
      beta2_(other.beta2_),
      epsilon_(other.epsilon_),
      n_(std::exchange(other.n_, 0)),
      t_(std::exchange(other.t_, 0)),
      m_(std::exchange(other.m_, nullptr)),
      v_(std::exchange(other.v_, nullptr)) {}

Adam& Adam::operator=(Adam&& other) noexcept {
    if (this != &other) {  // self-move would otherwise free our own buffers
        release();
        lr_ = other.lr_;
        beta1_ = other.beta1_;
        beta2_ = other.beta2_;
        epsilon_ = other.epsilon_;
        n_ = std::exchange(other.n_, 0);
        t_ = std::exchange(other.t_, 0);
        m_ = std::exchange(other.m_, nullptr);
        v_ = std::exchange(other.v_, nullptr);
    }
    return *this;
}

void Adam::release() {
    delete[] m_;  // delete[] on nullptr is a safe no-op
    delete[] v_;
    m_ = nullptr;
    v_ = nullptr;
    n_ = 0;
}

void Adam::reset(std::size_t n) {
    release();
    n_ = n;
    t_ = 0;
    // `new double[n]()` - the trailing () value-initialises, i.e. zero-fills.
    // Without it the contents would be indeterminate garbage.
    m_ = new double[n]();
    v_ = new double[n]();
}

void Adam::step(double* x, const double* grad) {
    ++t_;
    const double bias1 = 1.0 - std::pow(beta1_, static_cast<double>(t_));
    const double bias2 = 1.0 - std::pow(beta2_, static_cast<double>(t_));

    // Walk four arrays in parallel with four pointers.
    double* m = m_;
    double* v = v_;
    const double* const end = grad + n_;
    for (; grad != end; ++grad, ++m, ++v, ++x) {
        const double g = *grad;
        *m = beta1_ * (*m) + (1.0 - beta1_) * g;
        *v = beta2_ * (*v) + (1.0 - beta2_) * g * g;
        const double m_hat = *m / bias1;
        const double v_hat = *v / bias2;
        *x -= lr_ * m_hat / (std::sqrt(v_hat) + epsilon_);
    }
}
