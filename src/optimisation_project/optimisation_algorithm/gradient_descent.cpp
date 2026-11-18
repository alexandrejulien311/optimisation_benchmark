#include "gradient_descent.hpp"

void GradientDescent::step(double* x, const double* grad) {
    // `x` and `grad` are local copies of the caller's pointers, so moving them
    // here does not affect the caller.
    //
    // `*x++ -= ...` reads as: take the element x points to, update it, then
    // advance x to the next element (postfix ++ binds tighter than *).
    const double* const end = x + n_;
    while (x != end) {
        *x++ -= lr_ * *grad++;
    }
}
