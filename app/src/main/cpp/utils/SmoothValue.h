#ifndef KAOSSEFFECT_SMOOTHVALUE_H
#define KAOSSEFFECT_SMOOTHVALUE_H

#include <cmath>

template <typename T> class SmoothValue {
public:
  SmoothValue(T initial = 0, float smoothingCoeff = 0.995f)
      : target_(initial), current_(initial), coeff_(smoothingCoeff) {}

  void setTarget(T value) { target_ = value; }

  void setImmediate(T value) {
    target_ = value;
    current_ = value;
  }

  void setSmoothingCoeff(float coeff) { coeff_ = coeff; }

  T getNext() {
    if (isSmoothing()) {
      current_ = current_ * coeff_ + target_ * (1.0f - coeff_);
    } else {
      current_ = target_;
    }
    return current_;
  }

  T getCurrent() const { return current_; }

  bool isSmoothing() const { return std::abs(current_ - target_) > 0.0001f; }

private:
  T target_;
  T current_;
  float coeff_;
};

#endif // KAOSSEFFECT_SMOOTHVALUE_H
