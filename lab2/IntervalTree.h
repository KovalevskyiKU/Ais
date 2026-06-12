#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "Rational.h"

// ============================================================================
// Дерево відрізків на основі червоно-чорного дерева
// (interval tree, CLRS, розділ 14.3 "Дерева відрізків").
//
// Зберігає відрізки [low, high] з раціональними кінцями.
//   * Ключ вузла — лівий кінець відрізка low;
//   * додаткове поле max — найбільший правий кінець відрізка у піддереві;
//   * структура — класичне червоно-чорне дерево, тому висота O(log n),
//     і всі операції (вставка, видалення, пошук перетину) — O(log n).
//
// Відрізки [a,b] і [c,d] перетинаються <=> a <= d і c <= b.
//
// Поле max підтримується:
//   * при вставці — оновленням на шляху спуску;
//   * при поворотах — перерахунком для двох вузлів, що повернулись;
//   * при видаленні — перерахунком уздовж шляху від місця зміни до кореня.
// ============================================================================

struct Interval {
    Rational low, high;

    Interval(const Rational& l = Rational(0), const Rational& h = Rational(0))
        : low(l), high(h) {}

    // Чи перетинаються відрізки (включно з дотиком у точці)
    bool overlaps(const Interval& o) const {
        return low <= o.high && o.low <= high;
    }
    bool operator==(const Interval& o) const {
        return low == o.low && high == o.high;
    }
    friend std::ostream& operator<<(std::ostream& os, const Interval& iv) {
        return os << '[' << iv.low << "; " << iv.high << ']';
    }
};

class IntervalTree {
    enum Color { RED, BLACK };

    struct Node {
        Interval iv;
        Rational max;       // найбільший правий кінець у піддереві
        Color color = BLACK;
        Node* left = nullptr;
        Node* right = nullptr;
        Node* parent = nullptr;
    };

    Node* nil_;   // вартовий (чорний)
    Node* root_;
    size_t size_ = 0;

    // ---------------- підтримка поля max ---------------------------------

    void recalcMax(Node* x) {
        x->max = x->iv.high;
        if (x->left != nil_ && x->max < x->left->max)   x->max = x->left->max;
        if (x->right != nil_ && x->max < x->right->max) x->max = x->right->max;
    }

    // ---------------- повороти (з оновленням max) -------------------------

    void leftRotate(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left != nil_) y->left->parent = x;
        y->parent = x->parent;
        if (x->parent == nil_)            root_ = y;
        else if (x == x->parent->left)    x->parent->left = y;
        else                              x->parent->right = y;
        y->left = x;
        x->parent = y;
        recalcMax(x); // x тепер нижче — рахуємо першим
        recalcMax(y);
    }

    void rightRotate(Node* x) {
        Node* y = x->left;
        x->left = y->right;
        if (y->right != nil_) y->right->parent = x;
        y->parent = x->parent;
        if (x->parent == nil_)            root_ = y;
        else if (x == x->parent->right)   x->parent->right = y;
        else                              x->parent->left = y;
        y->right = x;
        x->parent = y;
        recalcMax(x);
        recalcMax(y);
    }

    // ---------------- вставка (RB-INSERT) ---------------------------------

    void insertFixup(Node* z) {
        while (z->parent->color == RED) {
            if (z->parent == z->parent->parent->left) {
                Node* uncle = z->parent->parent->right;
                if (uncle->color == RED) {                 // випадок 1: перефарбування
                    z->parent->color = BLACK;
                    uncle->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {           // випадок 2 -> 3
                        z = z->parent;
                        leftRotate(z);
                    }
                    z->parent->color = BLACK;              // випадок 3
                    z->parent->parent->color = RED;
                    rightRotate(z->parent->parent);
                }
            } else { // дзеркально
                Node* uncle = z->parent->parent->left;
                if (uncle->color == RED) {
                    z->parent->color = BLACK;
                    uncle->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rightRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    leftRotate(z->parent->parent);
                }
            }
        }
        root_->color = BLACK;
    }

    // ---------------- видалення (RB-DELETE) -------------------------------

    void transplant(Node* u, Node* v) {
        if (u->parent == nil_)            root_ = v;
        else if (u == u->parent->left)    u->parent->left = v;
        else                              u->parent->right = v;
        v->parent = u->parent; // для nil_ це теж коректно (CLRS)
    }

    Node* minimum(Node* x) const {
        while (x->left != nil_) x = x->left;
        return x;
    }

    void deleteFixup(Node* x) {
        while (x != root_ && x->color == BLACK) {
            if (x == x->parent->left) {
                Node* w = x->parent->right; // брат x
                if (w->color == RED) {                     // випадок 1
                    w->color = BLACK;
                    x->parent->color = RED;
                    leftRotate(x->parent);
                    w = x->parent->right;
                }
                if (w->left->color == BLACK && w->right->color == BLACK) {
                    w->color = RED;                        // випадок 2
                    x = x->parent;
                } else {
                    if (w->right->color == BLACK) {        // випадок 3
                        w->left->color = BLACK;
                        w->color = RED;
                        rightRotate(w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;           // випадок 4
                    x->parent->color = BLACK;
                    w->right->color = BLACK;
                    leftRotate(x->parent);
                    x = root_;
                }
            } else { // дзеркально
                Node* w = x->parent->left;
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    rightRotate(x->parent);
                    w = x->parent->left;
                }
                if (w->right->color == BLACK && w->left->color == BLACK) {
                    w->color = RED;
                    x = x->parent;
                } else {
                    if (w->left->color == BLACK) {
                        w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = x->parent->left;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    w->left->color = BLACK;
                    rightRotate(x->parent);
                    x = root_;
                }
            }
        }
        x->color = BLACK;
    }

    void rbDelete(Node* z) {
        Node* y = z;
        Node* x;
        Color yOriginalColor = y->color;

        if (z->left == nil_) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nil_) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = minimum(z->right); // наступник z
            yOriginalColor = y->color;
            x = y->right;
            if (y->parent == z) {
                x->parent = y;
            } else {
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }

        // Відновлюємо max уздовж шляху від місця зміни до кореня
        for (Node* p = x->parent; p != nil_; p = p->parent)
            recalcMax(p);

        if (yOriginalColor == BLACK)
            deleteFixup(x);

        delete z;
        --size_;
    }

    // ---------------- пошук вузла за відрізком ----------------------------

    // Точний пошук вузла з заданим відрізком (для видалення).
    // При однакових low відрізок може бути в обох піддеревах.
    Node* findNode(Node* x, const Interval& iv) const {
        if (x == nil_) return nil_;
        if (x->iv == iv) return x;
        if (iv.low < x->iv.low) return findNode(x->left, iv);
        if (x->iv.low < iv.low) return findNode(x->right, iv);
        Node* r = findNode(x->left, iv);
        return r != nil_ ? r : findNode(x->right, iv);
    }

    void collectOverlaps(Node* x, const Interval& q, std::vector<Interval>& out) const {
        if (x == nil_ || x->max < q.low) return; // у піддереві немає кандидатів
        collectOverlaps(x->left, q, out);
        if (x->iv.overlaps(q)) out.push_back(x->iv);
        if (x->iv.low <= q.high) collectOverlaps(x->right, q, out);
    }

    void freeRec(Node* x) {
        if (x == nil_) return;
        freeRec(x->left);
        freeRec(x->right);
        delete x;
    }

    void printRec(const Node* x, std::ostream& os,
                  const std::string& prefix, bool isLeft) const {
        if (x == nil_) return;
        printRec(x->right, os, prefix + (isLeft ? "|   " : "    "), false);
        os << prefix << (isLeft ? "\\-- " : "/-- ")
           << (x->color == RED ? "R " : "B ") << x->iv
           << " max=" << x->max << '\n';
        printRec(x->left, os, prefix + (isLeft ? "    " : "|   "), true);
    }

    // Перевірка властивостей ЧЧ-дерева і коректності max.
    // Повертає чорну висоту піддерева; ok скидається при порушеннях.
    int validateRec(const Node* x, bool& ok) const {
        if (x == nil_) return 1;
        if (x->color == RED &&
            (x->left->color == RED || x->right->color == RED))
            ok = false;                                  // червоний з червоною дитиною
        if (x->left != nil_ && x->iv.low < x->left->iv.low) ok = false;  // BST за low
        if (x->right != nil_ && x->right->iv.low < x->iv.low) ok = false;

        Rational m = x->iv.high;
        if (x->left != nil_ && m < x->left->max)  m = x->left->max;
        if (x->right != nil_ && m < x->right->max) m = x->right->max;
        if (!(x->max == m)) ok = false;                  // коректність max

        int bl = validateRec(x->left, ok);
        int br = validateRec(x->right, ok);
        if (bl != br) ok = false;                        // однакова чорна висота
        return bl + (x->color == BLACK ? 1 : 0);
    }

public:
    IntervalTree() {
        nil_ = new Node;
        nil_->color = BLACK;
        nil_->left = nil_->right = nil_->parent = nil_;
        root_ = nil_;
    }
    ~IntervalTree() {
        freeRec(root_);
        delete nil_;
    }

    IntervalTree(const IntervalTree&) = delete;
    IntervalTree& operator=(const IntervalTree&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return root_ == nil_; }

    // Вставка відрізка — O(log n)
    void insert(const Interval& iv) {
        if (iv.high < iv.low)
            throw std::invalid_argument("IntervalTree: лівий кінець більший за правий");

        Node* z = new Node;
        z->iv = iv;
        z->max = iv.high;
        z->color = RED;
        z->left = z->right = z->parent = nil_;

        Node* y = nil_;
        Node* x = root_;
        while (x != nil_) {
            y = x;
            if (x->max < iv.high) x->max = iv.high; // оновлюємо max на шляху вниз
            x = (iv.low < x->iv.low) ? x->left : x->right;
        }
        z->parent = y;
        if (y == nil_)                 root_ = z;
        else if (iv.low < y->iv.low)   y->left = z;
        else                           y->right = z;

        insertFixup(z);
        ++size_;
    }

    // Видалення відрізка (точний збіг кінців) — O(log n); false, якщо немає
    bool remove(const Interval& iv) {
        Node* z = findNode(root_, iv);
        if (z == nil_) return false;
        rbDelete(z);
        return true;
    }

    bool contains(const Interval& iv) const {
        return findNode(root_, iv) != nil_;
    }

    // Пошук БУДЬ-ЯКОГО відрізка, що перетинається із query — O(log n)
    // (INTERVAL-SEARCH з CLRS). Повертає true і записує результат у out.
    bool findOverlap(const Interval& query, Interval& out) const {
        Node* x = root_;
        while (x != nil_ && !x->iv.overlaps(query)) {
            // Якщо max лівого піддерева >= query.low — перетин (якщо існує)
            // гарантовано знайдеться у лівому піддереві
            if (x->left != nil_ && query.low <= x->left->max)
                x = x->left;
            else
                x = x->right;
        }
        if (x == nil_) return false;
        out = x->iv;
        return true;
    }

    // Усі відрізки, що перетинаються із query — O(k + log n)
    std::vector<Interval> findAllOverlaps(const Interval& query) const {
        std::vector<Interval> out;
        collectOverlaps(root_, query, out);
        return out;
    }

    // Усі відрізки за зростанням low (симетричний обхід)
    std::vector<Interval> toVector() const {
        std::vector<Interval> out;
        collectAll(root_, out);
        return out;
    }

    // Перевірка інваріантів ЧЧ-дерева та поля max
    bool validate() const {
        if (root_ == nil_) return true;
        if (root_->color != BLACK) return false;
        bool ok = true;
        validateRec(root_, ok);
        return ok;
    }

    void print(std::ostream& os) const {
        if (root_ == nil_) { os << "(порожнє дерево)\n"; return; }
        printRec(root_, os, "", false);
    }

private:
    void collectAll(const Node* x, std::vector<Interval>& out) const {
        if (x == nil_) return;
        collectAll(x->left, out);
        out.push_back(x->iv);
        collectAll(x->right, out);
    }
};
