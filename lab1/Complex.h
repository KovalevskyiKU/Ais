#pragma once
#include <iostream>

// ============================================================================
// Власна реалізація комплексного числа з цілочисельними компонентами
// (тип даних Т8, завдання 1).
//
// Порядок (за умовою): числа порівнюються за модулем; числа з однаковими
// модулями порівнюються за першою (дійсною) компонентою.
//
// Замість самого модуля |z| = sqrt(re^2 + im^2) порівнюємо квадрати модулів
// re^2 + im^2 — це цілі числа, порядок той самий, але без втрати точності
// через дійсну арифметику.
// ============================================================================
class Complex {
    long long re_; // дійсна компонента
    long long im_; // уявна компонента

public:
    Complex(long long re = 0, long long im = 0) : re_(re), im_(im) {}

    long long re() const { return re_; }
    long long im() const { return im_; }

    // Квадрат модуля: |z|^2 = re^2 + im^2
    long long normSq() const { return re_ * re_ + im_ * im_; }

    // Рівність — покомпонентна
    friend bool operator==(const Complex& a, const Complex& b) {
        return a.re_ == b.re_ && a.im_ == b.im_;
    }
    friend bool operator!=(const Complex& a, const Complex& b) {
        return !(a == b);
    }

    // Порядок за умовою: спочатку за модулем, потім за дійсною компонентою
    friend bool operator<(const Complex& a, const Complex& b) {
        if (a.normSq() != b.normSq()) return a.normSq() < b.normSq();
        return a.re_ < b.re_;
    }
    friend bool operator>(const Complex& a, const Complex& b) { return b < a; }
    friend bool operator<=(const Complex& a, const Complex& b) { return !(b < a); }
    friend bool operator>=(const Complex& a, const Complex& b) { return !(a < b); }

    // Вивід у вигляді a+bi / a-bi
    friend std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << c.re_;
        if (c.im_ >= 0) os << '+' << c.im_ << 'i';
        else            os << '-' << -c.im_ << 'i';
        return os;
    }
};
