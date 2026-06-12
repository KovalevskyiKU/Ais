#pragma once
#include <vector>
#include <queue>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include "Rational.h"

// ============================================================================
// В-дерево (CLRS, розділ 18) для зберігання раціональних чисел.
//
// t — мінімальний степінь дерева (t >= 2):
//   * кожен вузол, крім кореня, містить від t-1 до 2t-1 ключів;
//   * внутрішній вузол з n ключами має рівно n+1 дітей;
//   * усі листки знаходяться на одній глибині;
//   * ключі у вузлі впорядковані за зростанням.
//
// Складність операцій пошуку, вставки та видалення — O(t * log_t n).
// Вставка реалізована за один прохід вниз (з випереджувальним розщепленням
// повних вузлів), видалення — за CLRS (з випереджувальним поповненням
// вузлів, у які спускаємось, до >= t ключів).
// ============================================================================
class BTree {
    using Key = Rational;

    struct Node {
        std::vector<Key>   keys;
        std::vector<Node*> child; // порожній для листка
        bool leaf = true;
        int n() const { return (int)keys.size(); }
    };

    Node* root_;
    int t_; // мінімальний степінь

    // ---------------- службові методи -----------------------------------

    static void freeNode(Node* x) {
        if (!x) return;
        for (Node* c : x->child) freeNode(c);
        delete x;
    }

    // Індекс першого ключа у вузлі, що не менший за k
    static int findIndex(const Node* x, const Key& k) {
        int i = 0;
        while (i < x->n() && x->keys[i] < k) ++i;
        return i;
    }

    bool searchRec(const Node* x, const Key& k) const {
        int i = findIndex(x, k);
        if (i < x->n() && x->keys[i] == k) return true;
        if (x->leaf) return false;
        return searchRec(x->child[i], k);
    }

    // ---------------- вставка -------------------------------------------

    // Розщеплення повної дитини child[i] вузла x (x не повний)
    void splitChild(Node* x, int i) {
        Node* y = x->child[i];          // повний вузол: 2t-1 ключів
        Node* z = new Node;             // отримає t-1 старших ключів y
        z->leaf = y->leaf;

        z->keys.assign(y->keys.begin() + t_, y->keys.end());
        if (!y->leaf)
            z->child.assign(y->child.begin() + t_, y->child.end());

        Key median = y->keys[t_ - 1];   // середній ключ піднімається в x
        y->keys.resize(t_ - 1);
        if (!y->leaf) y->child.resize(t_);

        x->child.insert(x->child.begin() + i + 1, z);
        x->keys.insert(x->keys.begin() + i, median);
    }

    // Вставка у свідомо неповний вузол
    void insertNonFull(Node* x, const Key& k) {
        if (x->leaf) {
            x->keys.insert(std::upper_bound(x->keys.begin(), x->keys.end(), k), k);
            return;
        }
        int i = findIndex(x, k);
        if (x->child[i]->n() == 2 * t_ - 1) { // випереджувальне розщеплення
            splitChild(x, i);
            if (x->keys[i] < k) ++i;
        }
        insertNonFull(x->child[i], k);
    }

    // ---------------- видалення (CLRS) ----------------------------------

    Key getMax(Node* x) const {
        while (!x->leaf) x = x->child.back();
        return x->keys.back();
    }

    Key getMin(Node* x) const {
        while (!x->leaf) x = x->child.front();
        return x->keys.front();
    }

    // Злиття child[i], keys[i] та child[i+1] в один вузол (обидва мають t-1 ключів)
    void mergeChildren(Node* x, int i) {
        Node* y = x->child[i];
        Node* z = x->child[i + 1];
        y->keys.push_back(x->keys[i]);
        y->keys.insert(y->keys.end(), z->keys.begin(), z->keys.end());
        if (!y->leaf)
            y->child.insert(y->child.end(), z->child.begin(), z->child.end());
        x->keys.erase(x->keys.begin() + i);
        x->child.erase(x->child.begin() + i + 1);
        delete z;
    }

    // Позичання ключа у лівого сусіда child[i-1]
    void borrowFromLeft(Node* x, int i) {
        Node* c = x->child[i];
        Node* left = x->child[i - 1];
        c->keys.insert(c->keys.begin(), x->keys[i - 1]);
        x->keys[i - 1] = left->keys.back();
        left->keys.pop_back();
        if (!c->leaf) {
            c->child.insert(c->child.begin(), left->child.back());
            left->child.pop_back();
        }
    }

    // Позичання ключа у правого сусіда child[i+1]
    void borrowFromRight(Node* x, int i) {
        Node* c = x->child[i];
        Node* right = x->child[i + 1];
        c->keys.push_back(x->keys[i]);
        x->keys[i] = right->keys.front();
        right->keys.erase(right->keys.begin());
        if (!c->leaf) {
            c->child.push_back(right->child.front());
            right->child.erase(right->child.begin());
        }
    }

    // Гарантує, що child[i] матиме >= t ключів перед спуском у нього
    void fillChild(Node* x, int i) {
        if (i > 0 && x->child[i - 1]->n() >= t_)
            borrowFromLeft(x, i);
        else if (i < x->n() && x->child[i + 1]->n() >= t_)
            borrowFromRight(x, i);
        else if (i < x->n())
            mergeChildren(x, i);
        else
            mergeChildren(x, i - 1);
    }

    // Видалення ключа keys[i] із внутрішнього вузла x
    void removeFromInternal(Node* x, int i) {
        Key k = x->keys[i];
        Node* y = x->child[i];     // ліве піддерево ключа
        Node* z = x->child[i + 1]; // праве піддерево ключа

        if (y->n() >= t_) {                  // випадок 2а: замінюємо попередником
            Key pred = getMax(y);
            x->keys[i] = pred;
            removeRec(y, pred);
        } else if (z->n() >= t_) {           // випадок 2б: замінюємо наступником
            Key succ = getMin(z);
            x->keys[i] = succ;
            removeRec(z, succ);
        } else {                             // випадок 2в: злиття y + k + z
            mergeChildren(x, i);
            removeRec(y, k);
        }
    }

    // Інваріант: x — корінь або має >= t ключів
    void removeRec(Node* x, const Key& k) {
        int i = findIndex(x, k);
        if (i < x->n() && x->keys[i] == k) {
            if (x->leaf)
                x->keys.erase(x->keys.begin() + i); // випадок 1: ключ у листку
            else
                removeFromInternal(x, i);           // випадок 2: ключ у внутрішньому
        } else {
            if (x->leaf) return;                    // ключа немає в дереві
            bool lastChild = (i == x->n());
            if (x->child[i]->n() < t_)              // випадок 3: поповнюємо дитину
                fillChild(x, i);
            // після злиття кількість дітей могла зменшитись
            if (lastChild && i > x->n())
                removeRec(x->child[i - 1], k);
            else
                removeRec(x->child[i], k);
        }
    }

    // ---------------- перевірка інваріантів (для самотестування) ---------

    bool validateRec(const Node* x, bool isRoot, const Key* lo, const Key* hi,
                     int depth, int& leafDepth) const {
        int n = x->n();
        if (n > 2 * t_ - 1) return false;
        if (!isRoot && n < t_ - 1) return false;

        for (int i = 0; i + 1 < n; ++i)
            if (!(x->keys[i] < x->keys[i + 1])) return false; // строго зростають
        if (n > 0) {
            if (lo && !(*lo < x->keys.front())) return false;
            if (hi && !(x->keys.back() < *hi)) return false;
        }

        if (x->leaf) {
            if (!x->child.empty()) return false;
            if (leafDepth == -1) leafDepth = depth;
            return leafDepth == depth; // усі листки на одній глибині
        }
        if ((int)x->child.size() != n + 1) return false;
        for (int i = 0; i <= n; ++i) {
            const Key* l = (i == 0) ? lo : &x->keys[i - 1];
            const Key* h = (i == n) ? hi : &x->keys[i];
            if (!validateRec(x->child[i], false, l, h, depth + 1, leafDepth))
                return false;
        }
        return true;
    }

    void inorderRec(const Node* x, std::vector<Key>& out) const {
        if (x->leaf) {
            out.insert(out.end(), x->keys.begin(), x->keys.end());
            return;
        }
        for (int i = 0; i < x->n(); ++i) {
            inorderRec(x->child[i], out);
            out.push_back(x->keys[i]);
        }
        inorderRec(x->child.back(), out);
    }

public:
    explicit BTree(int t = 3) : t_(t) {
        if (t < 2) throw std::invalid_argument("BTree: мінімальний степінь t має бути >= 2");
        root_ = new Node;
    }
    ~BTree() { freeNode(root_); }

    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    int minDegree() const { return t_; }

    bool contains(const Key& k) const { return searchRec(root_, k); }

    // Вставка; дублікати не зберігаються (повертає false, якщо ключ уже є)
    bool insert(const Key& k) {
        if (contains(k)) return false;
        if (root_->n() == 2 * t_ - 1) { // корінь повний — росте висота
            Node* newRoot = new Node;
            newRoot->leaf = false;
            newRoot->child.push_back(root_);
            root_ = newRoot;
            splitChild(root_, 0);
        }
        insertNonFull(root_, k);
        return true;
    }

    // Видалення; повертає false, якщо ключа немає
    bool remove(const Key& k) {
        if (!contains(k)) return false;
        removeRec(root_, k);
        if (root_->n() == 0 && !root_->leaf) { // висота зменшилась
            Node* old = root_;
            root_ = root_->child[0];
            old->child.clear();
            delete old;
        }
        return true;
    }

    // Усі ключі за зростанням (симетричний обхід)
    std::vector<Key> inorder() const {
        std::vector<Key> out;
        if (root_->n() > 0 || !root_->leaf) inorderRec(root_, out);
        return out;
    }

    size_t size() const { return inorder().size(); }

    int height() const {
        int h = 0;
        const Node* x = root_;
        while (!x->leaf) { ++h; x = x->child[0]; }
        return h;
    }

    // Перевірка всіх інваріантів В-дерева
    bool validate() const {
        int leafDepth = -1;
        return validateRec(root_, true, nullptr, nullptr, 0, leafDepth);
    }

    // Друк дерева за рівнями: кожен вузол у вигляді [k1 k2 ...]
    void printByLevels(std::ostream& os) const {
        std::queue<const Node*> q;
        q.push(root_);
        int level = 0;
        while (!q.empty()) {
            size_t cnt = q.size();
            os << "Рівень " << level++ << ": ";
            while (cnt--) {
                const Node* x = q.front(); q.pop();
                os << '[';
                for (int i = 0; i < x->n(); ++i)
                    os << (i ? " " : "") << x->keys[i];
                os << "] ";
                for (const Node* c : x->child) q.push(c);
            }
            os << '\n';
        }
    }
};
