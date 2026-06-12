#pragma once
#include <iostream>
#include <stdexcept>

// ============================================================================
// Власна реалізація раціонального числа (тип даних Т8, завдання 2).
//
// Число зберігається у нормалізованому (нескоротному) вигляді:
//   * знаменник завжди > 0;
//   * НСД(|чисельник|, знаменник) = 1.
//
// Лінійний порядок очевидний: a/b < c/d  <=>  a*d < c*b (бо b, d > 0).
// Порівняння виконується перехресним множенням — без дійсної арифметики,
// тому без втрати точності.
// ============================================================================
class Rational {
    long long num_; // чисельник (несе знак)
    long long den_; // знаменник, завжди > 0

    static long long gcdll(long long a, long long b) {
        if (a < 0) a = -a;
        if (b < 0) b = -b;
        while (b != 0) { long long r = a % b; a = b; b = r; }
        return a;
    }

    void normalize() {
        if (den_ == 0)
            throw std::invalid_argument("Rational: знаменник не може дорівнювати нулю");
        if (den_ < 0) { num_ = -num_; den_ = -den_; }
        long long g = gcdll(num_, den_);
        if (g > 1) { num_ /= g; den_ /= g; }
    }

public:
    Rational(long long num = 0, long long den = 1) : num_(num), den_(den) {
        normalize();
    }

    long long num() const { return num_; }
    long long den() const { return den_; }

    // Порівняння через перехресне множення (знаменники додатні)
    friend bool operator<(const Rational& a, const Rational& b) {
        return a.num_ * b.den_ < b.num_ * a.den_;
    }
    friend bool operator>(const Rational& a, const Rational& b) { return b < a; }
    friend bool operator<=(const Rational& a, const Rational& b) { return !(b < a); }
    friend bool operator>=(const Rational& a, const Rational& b) { return !(a < b); }

    // Завдяки нормалізації рівність — це збіг чисельника і знаменника
    friend bool operator==(const Rational& a, const Rational& b) {
        return a.num_ == b.num_ && a.den_ == b.den_;
    }
    friend bool operator!=(const Rational& a, const Rational& b) { return !(a == b); }

    friend std::ostream& operator<<(std::ostream& os, const Rational& r) {
        os << r.num_;
        if (r.den_ != 1) os << '/' << r.den_;
        return os;
    }
};
