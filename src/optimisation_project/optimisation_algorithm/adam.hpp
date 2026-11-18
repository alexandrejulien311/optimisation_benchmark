#pragma once

#include "optimiser.hpp"

// Adam (Kingma & Ba, 2015):
//   m <- b1 m + (1 - b1) g            first moment (running mean of g)
//   v <- b2 v + (1 - b2) g^2          second moment (running mean of g^2)
//   m_hat = m / (1 - b1^t),  v_hat = v / (1 - b2^t)    bias correction
//   x <- x - lr * m_hat / (sqrt(v_hat) + eps)
//
// The moment buffers m and v are owned through raw pointers allocated with
// new[] and released with delete[], to show manual memory management. In
// production code a std::vector<double> would do this for you.
//
// Owning raw pointers force the "rule of five": the compiler-generated copy
// would copy the *pointers* (shallow copy), so two Adam objects would delete[]
// the same buffer -> double free. Copy is therefore disabled, and move is
// written by hand to transfer ownership and null out the source.
class Adam : public Optimiser {
public:
    explicit Adam(double learning_rate = 1e-3, double beta1 = 0.9, double beta2 = 0.999,
                  double epsilon = 1e-8);
    ~Adam() override;

    Adam(const Adam&) = delete;
    Adam& operator=(const Adam&) = delete;
    Adam(Adam&& other) noexcept;
    Adam& operator=(Adam&& other) noexcept;

    std::string name() const override { return "Adam"; }
    double learning_rate() const override { return lr_; }

    void reset(std::size_t n) override;
    void step(double* x, const double* grad) override;

private:
    void release();  // delete[] the buffers and null the pointers

    double lr_;
    double beta1_;
    double beta2_;
    double epsilon_;

    std::size_t n_ = 0;
    long long t_ = 0;      // number of steps taken since reset()
    double* m_ = nullptr;  // owned, n_ doubles
    double* v_ = nullptr;  // owned, n_ doubles
};
