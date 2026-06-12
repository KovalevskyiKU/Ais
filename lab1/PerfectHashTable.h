#pragma once
#include <vector>
#include <random>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include "Complex.h"

// ============================================================================
// Ідеальне хешування — схема Фредмана–Комлоша–Семереді (FKS),
// CLRS, розділ 11.5 "Ідеальне хешування".
//
// Ідея: для СТАТИЧНОЇ множини з n ключів будується дворівнева структура:
//   1-й рівень: універсальна хеш-функція h розкидає ключі по n кошиках;
//   2-й рівень: кошик j з m_j ключами отримує власну таблицю розміру m_j^2
//               і власну хеш-функцію, яку перебираємо випадково, доки вона
//               не стане ін'єктивною на ключах кошика (без жодної колізії).
//
// Гарантії:
//   * пошук — O(1) у НАЙГІРШОМУ випадку (рівно 2 обчислення хеш-функції
//     та 1 порівняння ключів);
//   * очікувана сумарна пам'ять — O(n): функцію 1-го рівня перебираємо,
//     доки sum(m_j^2) <= 4n (за теоремою це виконується з імовірністю >= 1/2,
//     тож очікувана кількість спроб <= 2).
//
// Універсальна сім'я хеш-функцій для пари цілих компонент (x, y):
//     h(x, y) = ((a1*x + a2*y + b) mod p) mod m,
// де p = 2^31 - 1 (просте число Мерсенна), a1, a2 — випадкові з [1, p-1],
// b — випадкове з [0, p-1]. Для різних пар (x, y) імовірність колізії <= 1/m.
//
// Компоненти ключів обмежені діапазоном |re|, |im| <= 10^9 < p, тому різні
// комплексні числа дають різні пари (x, y) за модулем p, і всі добутки
// вкладаються у 64-бітну арифметику без переповнення.
// ============================================================================
class PerfectHashTable {
    static constexpr uint64_t P     = 2147483647ULL;  // 2^31 - 1, просте Мерсенна
    static constexpr long long LIM  = 1000000000LL;   // обмеження компонент
    static constexpr long long SHIFT = LIM;           // зсув у невід'ємний діапазон

    // Хеш-функція з універсальної сім'ї
    struct HashFunc {
        uint64_t a1 = 1, a2 = 1, b = 0;
        size_t m = 1; // розмір таблиці

        size_t operator()(const Complex& c) const {
            uint64_t x = (uint64_t)(c.re() + SHIFT); // 0 <= x <= 2*10^9 < p
            uint64_t y = (uint64_t)(c.im() + SHIFT);
            uint64_t v = (a1 * x % P + a2 * y % P + b) % P;
            return (size_t)(v % (uint64_t)m);
        }
    };

    // Кошик другого рівня: таблиця розміру m_j^2 і власна хеш-функція
    struct Bucket {
        HashFunc h;
        std::vector<Complex> slots;  // комірки таблиці
        std::vector<char>    used;   // чи зайнята комірка
    };

    size_t n_ = 0;                 // кількість ключів
    HashFunc h1_;                  // функція першого рівня
    std::vector<Bucket> buckets_;  // n кошиків
    size_t rebuildAttempts_ = 0;   // скільки разів перебирали функцію 1-го рівня
    std::mt19937_64 rng_;

    HashFunc randomFunc(size_t m) {
        std::uniform_int_distribution<uint64_t> da(1, P - 1);
        std::uniform_int_distribution<uint64_t> db(0, P - 1);
        return HashFunc{ da(rng_), da(rng_), db(rng_), m };
    }

public:
    // Будує ідеальну хеш-таблицю для статичної множини ключів
    explicit PerfectHashTable(const std::vector<Complex>& inputKeys, uint64_t seed = 20260610)
        : rng_(seed)
    {
        // Перевірка обмежень типу і видалення дублікатів
        std::vector<Complex> keys = inputKeys;
        for (const Complex& c : keys)
            if (std::abs(c.re()) > LIM || std::abs(c.im()) > LIM)
                throw std::invalid_argument("PerfectHashTable: компоненти мають бути в межах [-10^9, 10^9]");
        std::sort(keys.begin(), keys.end(), [](const Complex& a, const Complex& b) {
            return a.re() != b.re() ? a.re() < b.re() : a.im() < b.im();
        });
        keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

        n_ = keys.size();
        if (n_ == 0) return;

        // --- Перший рівень: добираємо h1, доки sum(m_j^2) <= 4n -------------
        std::vector<std::vector<Complex>> groups;
        for (;;) {
            ++rebuildAttempts_;
            h1_ = randomFunc(n_);
            groups.assign(n_, {});
            for (const Complex& k : keys)
                groups[h1_(k)].push_back(k);

            unsigned long long sumSq = 0;
            for (const auto& g : groups)
                sumSq += (unsigned long long)g.size() * g.size();
            if (sumSq <= 4ULL * n_) break; // очікувано <= 2 спроб

            if (rebuildAttempts_ > 200)
                throw std::runtime_error("PerfectHashTable: не вдалося підібрати функцію 1-го рівня");
        }

        // --- Другий рівень: для кожного кошика — ін'єктивна функція ---------
        buckets_.assign(n_, {});
        for (size_t j = 0; j < n_; ++j) {
            const auto& g = groups[j];
            if (g.empty()) continue;

            size_t m2 = g.size() * g.size(); // розмір таблиці = m_j^2
            Bucket& bkt = buckets_[j];
            for (;;) { // очікувано < 2 спроб: імовірність колізії < 1/2
                bkt.h = randomFunc(m2);
                bkt.slots.assign(m2, Complex{});
                bkt.used.assign(m2, 0);
                bool ok = true;
                for (const Complex& k : g) {
                    size_t idx = bkt.h(k);
                    if (bkt.used[idx]) { ok = false; break; } // колізія — нова функція
                    bkt.used[idx] = 1;
                    bkt.slots[idx] = k;
                }
                if (ok) break;
            }
        }
    }

    // Пошук за O(1) у найгіршому випадку:
    // 2 обчислення хеш-функції + 1 порівняння ключів
    bool contains(const Complex& c) const {
        if (n_ == 0) return false;
        if (std::abs(c.re()) > LIM || std::abs(c.im()) > LIM) return false;
        const Bucket& bkt = buckets_[h1_(c)];
        if (bkt.slots.empty()) return false;
        size_t idx = bkt.h(c);
        return bkt.used[idx] && bkt.slots[idx] == c;
    }

    size_t size() const { return n_; }

    // Сумарна кількість комірок другого рівня (показник пам'яті, <= 4n)
    size_t totalSlots() const {
        size_t s = 0;
        for (const auto& b : buckets_) s += b.slots.size();
        return s;
    }

    // Статистика структури — для демонстрації властивостей схеми
    void printStats(std::ostream& os) const {
        os << "Ключів (n):                     " << n_ << '\n';
        os << "Кошиків першого рівня:          " << buckets_.size() << '\n';

        size_t nonEmpty = 0, maxBucket = 0;
        for (const auto& b : buckets_) {
            size_t cnt = 0;
            for (char u : b.used) cnt += (size_t)u;
            if (cnt > 0) ++nonEmpty;
            maxBucket = std::max(maxBucket, cnt);
        }
        os << "Непорожніх кошиків:             " << nonEmpty << '\n';
        os << "Найбільший кошик (m_j):         " << maxBucket << '\n';
        os << "Комірок 2-го рівня (sum m_j^2): " << totalSlots()
           << "  (межа 4n = " << 4 * n_ << ")\n";
        os << "Спроб підбору функції 1-го рівня: " << rebuildAttempts_ << '\n';
    }
};
