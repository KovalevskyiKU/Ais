// ============================================================================
// Лабораторна робота 1. Ідеальне хешування (схема FKS).
// Варіант 8, тип даних Т8 (завдання 1) — комплексні числа з цілочисельними
// компонентами. Порядок: за модулем, при рівних модулях — за дійсною частиною.
// ============================================================================
#include <iostream>
#include <vector>
#include <random>
#include <set>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

#include "Complex.h"
#include "PerfectHashTable.h"

// Демонстрація порядку, визначеного для типу даних
static void demoOrdering() {
    std::cout << "=== Порядок на комплексних числах (за модулем, потім за Re) ===\n";
    std::vector<Complex> v = {
        {3, 4}, {0, 5}, {-5, 0}, {1, 1}, {0, 0}, {2, -2}, {-3, -4}, {5, 0}, {1, 0}
    };
    std::sort(v.begin(), v.end()); // використовує operator< типу Complex
    std::cout << "Відсортовано: ";
    for (const Complex& c : v) std::cout << c << "  ";
    std::cout << "\n(числа 3+4i, -3-4i, 0+5i, -5+0i, 5+0i мають модуль 5 і впорядковані за Re)\n\n";
}

// Основна демонстрація ідеального хешування
static void demoPerfectHashing() {
    std::cout << "=== Ідеальне хешування (FKS) ===\n";
    std::vector<Complex> keys = {
        {3, 4}, {0, 5}, {-3, -4}, {1, 1}, {2, 2}, {5, 0},
        {-5, 0}, {1, 0}, {0, 1}, {10, -7}, {-12, 35}, {100, 200},
        {7, 24}, {-8, 15}, {20, 21}, {0, -1}, {999, -999}, {123456789, -987654321}
    };

    PerfectHashTable table(keys);
    table.printStats(std::cout);

    std::cout << "\nПошук усіх доданих ключів:\n";
    bool allFound = true;
    for (const Complex& k : keys) {
        bool f = table.contains(k);
        allFound = allFound && f;
        std::cout << "  contains(" << k << ") = " << (f ? "так" : "НІ!") << '\n';
    }

    std::vector<Complex> absent = { {4, 3}, {0, 2}, {-1, -1}, {1000000, 1000000} };
    std::cout << "\nПошук відсутніх ключів:\n";
    bool noneFound = true;
    for (const Complex& k : absent) {
        bool f = table.contains(k);
        noneFound = noneFound && !f;
        std::cout << "  contains(" << k << ") = " << (f ? "так?!" : "ні") << '\n';
    }

    std::cout << "\nРезультат демонстрації: "
              << ((allFound && noneFound) ? "OK" : "ПОМИЛКА") << "\n\n";
}

// Стрес-тест: випадкові ключі, перевірка проти еталонної множини
static bool stressTest() {
    std::cout << "=== Стрес-тест (випадкові дані) ===\n";
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<long long> comp(-1000000000LL, 1000000000LL);

    auto cmpRiM = [](const Complex& a, const Complex& b) {
        return a.re() != b.re() ? a.re() < b.re() : a.im() < b.im();
    };

    for (int iter = 0; iter < 5; ++iter) {
        size_t n = 200 + (size_t)iter * 400;
        std::set<Complex, decltype(cmpRiM)> reference(cmpRiM);
        std::vector<Complex> keys;
        while (reference.size() < n) {
            Complex c(comp(rng), comp(rng));
            if (reference.insert(c).second) keys.push_back(c);
        }

        PerfectHashTable table(keys, /*seed=*/777 + (uint64_t)iter);

        // 1) усі ключі знаходяться
        for (const Complex& k : keys)
            if (!table.contains(k)) {
                std::cout << "ПОМИЛКА: не знайдено доданий ключ " << k << '\n';
                return false;
            }
        // 2) випадкові не-ключі не знаходяться
        for (int q = 0; q < 2000; ++q) {
            Complex c(comp(rng), comp(rng));
            bool expected = reference.count(c) > 0;
            if (table.contains(c) != expected) {
                std::cout << "ПОМИЛКА: розбіжність для " << c << '\n';
                return false;
            }
        }
        // 3) пам'ять у межах теоретичної оцінки
        if (table.totalSlots() > 4 * n) {
            std::cout << "ПОМИЛКА: перевищено межу пам'яті 4n\n";
            return false;
        }
        std::cout << "  n = " << n << ": усі " << n << " ключів знайдено, "
                  << "2000 запитів збігаються з еталоном, пам'ять "
                  << table.totalSlots() << " <= " << 4 * n << '\n';
    }
    std::cout << "Стрес-тест пройдено.\n\n";
    return true;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // коректний вивід UTF-8 у консолі Windows
#endif
    std::cout << "Лабораторна робота 1. Ідеальне хешування.\n";
    std::cout << "Варіант 8, тип даних Т8: комплексні числа з цілими компонентами.\n\n";

    demoOrdering();
    demoPerfectHashing();
    bool ok = stressTest();

    std::cout << (ok ? "Усі перевірки пройдено успішно." : "Є помилки!") << '\n';
    return ok ? 0 : 1;
}
