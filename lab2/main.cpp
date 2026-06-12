// ============================================================================
// Лабораторна робота 2 (завдання 2, варіант 8, тип даних Т8 — раціональні числа):
//   Частина 1. В-дерево (CLRS, розділ 18).
//   Частина 2. Дерево відрізків на основі червоно-чорного дерева
//              (interval tree, CLRS, розділ 14.3).
// ============================================================================
#include <iostream>
#include <vector>
#include <random>
#include <set>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

#include "Rational.h"
#include "BTree.h"
#include "IntervalTree.h"

// ---------------------------------------------------------------------------
// Демонстрація типу даних Rational
// ---------------------------------------------------------------------------
static void demoRational() {
    std::cout << "=== Тип даних: раціональні числа ===\n";
    Rational a(3, 6), b(-10, 4), c(7), d(2, -8);
    std::cout << "3/6 нормалізується до " << a
              << ", -10/4 до " << b
              << ", 7 — це " << c
              << ", 2/-8 до " << d << '\n';
    std::cout << "Порівняння: " << a << " > " << b << " : " << (a > b ? "так" : "ні")
              << ";  " << Rational(1, 3) << " < " << Rational(2, 5) << " : "
              << (Rational(1, 3) < Rational(2, 5) ? "так" : "ні") << "\n\n";
}

// ---------------------------------------------------------------------------
// Частина 1: В-дерево
// ---------------------------------------------------------------------------
static void demoBTree() {
    std::cout << "=== Частина 1. В-дерево (t = 2) ===\n";
    BTree tree(2);

    std::vector<Rational> keys = {
        {1, 2}, {3, 4}, {-5, 2}, {7, 3}, {2, 1}, {0, 1}, {-1, 3},
        {5, 6}, {11, 4}, {-7, 8}, {9, 2}, {13, 5}, {-3, 1}, {8, 7}, {4, 9}
    };
    std::cout << "Вставляємо " << keys.size() << " ключів: ";
    for (const Rational& k : keys) std::cout << k << ' ';
    std::cout << "\n\nДерево за рівнями:\n";
    for (const Rational& k : keys) tree.insert(k);
    tree.printByLevels(std::cout);

    std::cout << "\nСиметричний обхід (ключі за зростанням): ";
    for (const Rational& k : tree.inorder()) std::cout << k << ' ';
    std::cout << "\nВисота: " << tree.height()
              << ", інваріанти В-дерева: " << (tree.validate() ? "OK" : "ПОРУШЕНО") << '\n';

    std::cout << "\nПошук: contains(3/4) = " << (tree.contains({3, 4}) ? "так" : "ні")
              << ", contains(1/7) = " << (tree.contains({1, 7}) ? "так" : "ні") << '\n';

    std::vector<Rational> toDelete = { {3, 4}, {-3, 1}, {2, 1}, {1, 2}, {9, 2} };
    std::cout << "\nВидаляємо: ";
    for (const Rational& k : toDelete) std::cout << k << ' ';
    for (const Rational& k : toDelete) tree.remove(k);
    std::cout << "\nДерево після видалення:\n";
    tree.printByLevels(std::cout);
    std::cout << "Інваріанти: " << (tree.validate() ? "OK" : "ПОРУШЕНО") << "\n\n";
}

static bool stressBTree() {
    std::cout << "=== Самотестування В-дерева (випадкові дані) ===\n";
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<long long> numD(-1000, 1000), denD(1, 60);

    for (int t = 2; t <= 4; ++t) {
        BTree tree(t);
        std::set<Rational> reference; // еталонна впорядкована множина

        // вставки
        for (int i = 0; i < 800; ++i) {
            Rational r(numD(rng), denD(rng));
            bool inserted = tree.insert(r);
            bool refInserted = reference.insert(r).second;
            if (inserted != refInserted) {
                std::cout << "ПОМИЛКА вставки " << r << '\n';
                return false;
            }
        }
        // обхід має збігатися з еталоном
        std::vector<Rational> got = tree.inorder();
        if (!std::equal(got.begin(), got.end(), reference.begin(), reference.end())) {
            std::cout << "ПОМИЛКА: обхід не збігається з еталоном\n";
            return false;
        }
        if (!tree.validate()) {
            std::cout << "ПОМИЛКА: порушено інваріанти після вставок\n";
            return false;
        }

        // видалення половини елементів (і спроби видалити відсутні)
        std::vector<Rational> all(reference.begin(), reference.end());
        std::shuffle(all.begin(), all.end(), rng);
        for (size_t i = 0; i < all.size() / 2; ++i) {
            if (!tree.remove(all[i])) { std::cout << "ПОМИЛКА видалення\n"; return false; }
            reference.erase(all[i]);
            if (tree.remove(all[i])) { std::cout << "ПОМИЛКА: повторне видалення\n"; return false; }
        }
        got = tree.inorder();
        if (!std::equal(got.begin(), got.end(), reference.begin(), reference.end()) ||
            !tree.validate()) {
            std::cout << "ПОМИЛКА після видалень\n";
            return false;
        }
        std::cout << "  t = " << t << ": " << all.size() << " вставок, "
                  << all.size() / 2 << " видалень — обхід та інваріанти OK (висота "
                  << tree.height() << ")\n";
    }
    std::cout << "Самотестування В-дерева пройдено.\n\n";
    return true;
}

// ---------------------------------------------------------------------------
// Частина 2: дерево відрізків на основі червоно-чорного дерева
// ---------------------------------------------------------------------------
static void demoIntervalTree() {
    std::cout << "=== Частина 2. Дерево відрізків на основі ЧЧ-дерева ===\n";
    IntervalTree tree;

    std::vector<Interval> ivs = {
        { {0, 1},  {3, 1} },   // [0; 3]
        { {1, 2},  {5, 2} },   // [1/2; 5/2]
        { {4, 1},  {6, 1} },   // [4; 6]
        { {7, 2},  {9, 2} },   // [7/2; 9/2]
        { {5, 1},  {8, 1} },   // [5; 8]
        { {-3, 2}, {1, 4} },   // [-3/2; 1/4]
        { {15, 2}, {10, 1} },  // [15/2; 10]
        { {6, 1},  {7, 1} },   // [6; 7]
        { {19, 4}, {23, 4} },  // [19/4; 23/4]
    };
    std::cout << "Вставляємо відрізки: ";
    for (const Interval& iv : ivs) std::cout << iv << ' ';
    std::cout << "\n\nСтруктура дерева (R/B — колір, max — найбільший правий кінець у піддереві):\n";
    for (const Interval& iv : ivs) tree.insert(iv);
    tree.print(std::cout);
    std::cout << "Властивості ЧЧ-дерева та поля max: "
              << (tree.validate() ? "OK" : "ПОРУШЕНО") << '\n';

    Interval q1({2, 1}, {4, 1});   // [2; 4]
    Interval q2({11, 1}, {12, 1}); // [11; 12]
    Interval found;
    std::cout << "\nПошук перетину з " << q1 << ": ";
    if (tree.findOverlap(q1, found)) std::cout << "знайдено " << found << '\n';
    else                             std::cout << "немає\n";
    std::cout << "Пошук перетину з " << q2 << ": ";
    if (tree.findOverlap(q2, found)) std::cout << "знайдено " << found << '\n';
    else                             std::cout << "немає\n";

    std::cout << "Усі відрізки, що перетинаються з " << q1 << ": ";
    for (const Interval& iv : tree.findAllOverlaps(q1)) std::cout << iv << ' ';
    std::cout << '\n';

    Interval del({4, 1}, {6, 1});
    std::cout << "\nВидаляємо " << del << " і повторюємо пошук перетину з " << q1 << ": ";
    tree.remove(del);
    if (tree.findOverlap(q1, found)) std::cout << "знайдено " << found << '\n';
    else                             std::cout << "немає\n";
    std::cout << "Дерево після видалення:\n";
    tree.print(std::cout);
    std::cout << "Властивості: " << (tree.validate() ? "OK" : "ПОРУШЕНО") << "\n\n";
}

static bool stressIntervalTree() {
    std::cout << "=== Самотестування дерева відрізків (випадкові дані) ===\n";
    std::mt19937_64 rng(2024);
    std::uniform_int_distribution<long long> numD(-500, 500), denD(1, 20), lenD(0, 300);

    IntervalTree tree;
    std::vector<Interval> reference; // еталонний список відрізків

    auto randomInterval = [&]() {
        Rational lo(numD(rng), denD(rng));
        Rational len(lenD(rng), denD(rng));
        Rational hi(lo.num() * len.den() + len.num() * lo.den(),
                    lo.den() * len.den()); // hi = lo + len >= lo
        return Interval(lo, hi);
    };

    // вставки впереміш із видаленнями
    for (int i = 0; i < 1500; ++i) {
        if (!reference.empty() && i % 4 == 3) { // кожна четверта операція — видалення
            size_t idx = (size_t)(rng() % reference.size());
            Interval iv = reference[idx];
            if (!tree.remove(iv)) { std::cout << "ПОМИЛКА видалення " << iv << '\n'; return false; }
            reference.erase(reference.begin() + idx);
        } else {
            Interval iv = randomInterval();
            tree.insert(iv);
            reference.push_back(iv);
        }
        if (i % 250 == 0 && !tree.validate()) {
            std::cout << "ПОМИЛКА: порушено інваріанти на кроці " << i << '\n';
            return false;
        }
    }
    if (!tree.validate() || tree.size() != reference.size()) {
        std::cout << "ПОМИЛКА: інваріанти або розмір після операцій\n";
        return false;
    }

    // запити перетину звіряємо з повним перебором
    for (int q = 0; q < 2000; ++q) {
        Interval query = randomInterval();

        std::vector<Interval> brute;
        for (const Interval& iv : reference)
            if (iv.overlaps(query)) brute.push_back(iv);

        Interval found;
        bool any = tree.findOverlap(query, found);
        if (any != !brute.empty()) {
            std::cout << "ПОМИЛКА: findOverlap не збігається з перебором\n";
            return false;
        }
        if (any && !found.overlaps(query)) {
            std::cout << "ПОМИЛКА: повернений відрізок не перетинається\n";
            return false;
        }

        std::vector<Interval> all = tree.findAllOverlaps(query);
        if (all.size() != brute.size()) {
            std::cout << "ПОМИЛКА: findAllOverlaps повернув " << all.size()
                      << " замість " << brute.size() << '\n';
            return false;
        }
    }
    std::cout << "  " << tree.size() << " відрізків у дереві, 2000 запитів перетину "
              << "збігаються з повним перебором, інваріанти OK\n";
    std::cout << "Самотестування дерева відрізків пройдено.\n\n";
    return true;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // коректний вивід UTF-8 у консолі Windows
#endif
    std::cout << "Лабораторна робота 2. Варіант 8, тип даних Т8 (раціональні числа).\n";
    std::cout << "В-дерево + дерево відрізків на основі червоно-чорного дерева.\n\n";

    demoRational();
    demoBTree();
    bool ok1 = stressBTree();
    demoIntervalTree();
    bool ok2 = stressIntervalTree();

    std::cout << ((ok1 && ok2) ? "Усі перевірки пройдено успішно." : "Є помилки!") << '\n';
    return (ok1 && ok2) ? 0 : 1;
}
