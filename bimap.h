#pragma once

#include <algorithm>  // std::swap
#include <cstddef>    // size_t
#include <functional> // std::function
#include <utility>    // std::forward, std::make_pair, std::move, std::pair

/*
 * Has splay tree based structure.
 * Requires O(log(size)) time on average for inserting, erasing or finding one element.
 * Requires ((6 * sizeof(pointer) + sizeof(Left) + sizeof(Right)) * size
 *          + 2 * sizeof(pointer) + sizeof(size_t)
 *          + sizeof(LeftComparator) + sizeof(RightComparator)) bytes memory.
 * Doesn't allocate any dynamic memory for any operation (except for exactly one allocation to inserting a new pair).
 */

template <typename Left, typename Right, typename LeftComparator = std::less<>, typename RightComparator = std::less<>>
class bimap
{
    /* Stores data of left and right trees in the same node */
    struct node_t
    {
        struct tree_node_t
        {
            node_t * left;
            node_t * right;
            node_t * parent;

            tree_node_t() noexcept
                : left(nullptr)
                , right(nullptr)
                , parent(nullptr)
            {
            }
        };

        template <typename L, typename R>
        explicit node_t(L left_value, R right_value)
            : left_value(std::forward<L>(left_value))
            , right_value(std::forward<R>(right_value))
        {
        }

        Left left_value;
        Right right_value;
        tree_node_t left_tree_data;
        tree_node_t right_tree_data;
    };

    struct left_descriptor_t
    {
        static node_t *& left(node_t * node) noexcept
        {
            return node->left_tree_data.left;
        }

        static node_t * const & left(node_t const * node) noexcept
        {
            return node->left_tree_data.left;
        }

        static node_t *& right(node_t * node) noexcept
        {
            return node->left_tree_data.right;
        }

        static node_t * const & right(node_t const * node) noexcept
        {
            return node->left_tree_data.right;
        }

        static node_t *& parent(node_t * node) noexcept
        {
            return node->left_tree_data.parent;
        }

        static node_t * const & parent(node_t const * node) noexcept
        {
            return node->left_tree_data.parent;
        }

        static Left & value(node_t * node) noexcept
        {
            return node->left_value;
        }

        static Left const & value(node_t const * node) noexcept
        {
            return node->left_value;
        }
    };

    struct right_descriptor_t
    {
        static node_t *& left(node_t * node) noexcept
        {
            return node->right_tree_data.left;
        }

        static node_t * const & left(node_t const * node) noexcept
        {
            return node->right_tree_data.left;
        }

        static node_t *& right(node_t * node) noexcept
        {
            return node->right_tree_data.right;
        }

        static node_t * const & right(node_t const * node) noexcept
        {
            return node->right_tree_data.right;
        }

        static node_t *& parent(node_t * node) noexcept
        {
            return node->right_tree_data.parent;
        }

        static node_t * const & parent(node_t const * node) noexcept
        {
            return node->right_tree_data.parent;
        }

        static Right & value(node_t * node) noexcept
        {
            return node->right_value;
        }

        static Right const & value(node_t const * node) noexcept
        {
            return node->right_value;
        }
    };

    template <typename MainDescriptor, typename FlipDescriptor, typename MainType, typename FlipType>
    class basic_iterator
    {
    protected:
        friend class bimap<MainType, FlipType, LeftComparator, RightComparator>;

        using tree_t = bimap<MainType, FlipType, LeftComparator, RightComparator>;

        basic_iterator(tree_t const * tree, node_t const * node) noexcept
            : tree(tree)
            , node(node)
        {
        }

        void increment() noexcept
        {
            node = tree->next<MainDescriptor>(node);
        }

        void decrement() noexcept
        {
            if (node == nullptr) {
                node = sink_right<MainDescriptor>(tree->left_root);
            }
            else {
                node = tree->previous<MainDescriptor>(node);
            }
        }

        MainType const & get() noexcept
        {
            return MainDescriptor::value(node);
        }

        bimap const * tree;
        node_t const * node;

    public:
        bool operator==(basic_iterator const & other) const noexcept
        {
            return (this->tree == other.tree && this->node == other.node);
        }

        bool operator!=(basic_iterator const & other) const noexcept
        {
            return !(*this == other);
        }

        basic_iterator & operator++() noexcept
        {
            this->increment();
            return *this;
        }

        basic_iterator operator++(int) noexcept
        {
            auto copy = *this;
            this->increment();
            return copy;
        }

        basic_iterator & operator--() noexcept
        {
            this->decrement();
            return *this;
        }

        basic_iterator operator--(int) noexcept
        {
            auto copy = *this;
            this->decrement();
            return copy;
        }

        MainType const & operator*() const noexcept
        {
            return MainDescriptor::value(node);
        }

        auto flip() const noexcept
        {
            return basic_iterator<FlipDescriptor, MainDescriptor, FlipType, MainType>(tree, node);
        }
    };

    template <typename Descriptor>
    static void set_parent(node_t * t, node_t const * p) noexcept
    {
        if (t != nullptr) {
            Descriptor::parent(t) = const_cast<node_t *>(p);
        }
    }

    template <typename Descriptor>
    static node_t * rotate_left(node_t * x, node_t * p) noexcept
    {
        node_t * q = Descriptor::parent(p);
        if (q != nullptr) {
            if (Descriptor::left(q) == p) {
                Descriptor::left(q) = x;
            }
            else {
                Descriptor::right(q) = x;
            }
        }
        Descriptor::parent(x) = Descriptor::parent(p);
        Descriptor::parent(p) = x;
        Descriptor::right(p) = Descriptor::left(x);
        set_parent<Descriptor>(Descriptor::left(x), p);
        Descriptor::left(x) = p;
        return x;
    }

    template <typename Descriptor>
    static node_t * rotate_right(node_t * x, node_t * p) noexcept
    {
        node_t * q = Descriptor::parent(p);
        if (q != nullptr) {
            if (Descriptor::left(q) == p) {
                Descriptor::left(q) = x;
            }
            else {
                Descriptor::right(q) = x;
            }
        }
        Descriptor::parent(x) = Descriptor::parent(p);
        Descriptor::parent(p) = x;
        Descriptor::left(p) = Descriptor::right(x);
        set_parent<Descriptor>(Descriptor::right(x), p);
        Descriptor::right(x) = p;
        return x;
    }

    template <typename Descriptor>
    static node_t * splay(node_t * t) noexcept
    {
        if (t == nullptr) {
            return nullptr;
        }
        while (Descriptor::parent(t) != nullptr) {
            node_t * p = Descriptor::parent(t);
            if (Descriptor::parent(p) == nullptr) {
                if (t == Descriptor::right(p)) {
                    t = rotate_left<Descriptor>(t, p);
                }
                else {
                    t = rotate_right<Descriptor>(t, p);
                }
            }
            else {
                node_t * g = Descriptor::parent(p);
                if (Descriptor::left(g) == p && Descriptor::left(p) == t) {
                    p = rotate_right<Descriptor>(p, g);
                    t = rotate_right<Descriptor>(t, p);
                }
                else if (Descriptor::right(g) == p && Descriptor::right(p) == t) {
                    p = rotate_left<Descriptor>(p, g);
                    t = rotate_left<Descriptor>(t, p);
                }
                else if (Descriptor::left(g) == p && Descriptor::right(p) == t) {
                    t = rotate_left<Descriptor>(t, p);
                    t = rotate_right<Descriptor>(t, g);
                }
                else {
                    t = rotate_right<Descriptor>(t, p);
                    t = rotate_left<Descriptor>(t, g);
                }
            }
        }
        return t;
    }

    template <typename Descriptor>
    static node_t * sink_left(node_t const * t) noexcept
    {
        while (Descriptor::left(t) != nullptr) {
            t = Descriptor::left(t);
        }
        return const_cast<node_t *>(t);
    }

    template <typename Descriptor>
    static node_t * sink_right(node_t const * t) noexcept
    {
        while (Descriptor::right(t) != nullptr) {
            t = Descriptor::right(t);
        }
        return const_cast<node_t *>(t);
    }

    template <typename Descriptor, typename T, typename Comparator>
    static node_t * find(node_t * t, T const & x, Comparator const & compare) noexcept
    {
        if (t == nullptr) {
            return nullptr;
        }
        if (compare(x, Descriptor::value(t)) && Descriptor::left(t) != nullptr) {
            return find<Descriptor>(Descriptor::left(t), x, compare);
        }
        if (compare(Descriptor::value(t), x) && Descriptor::right(t) != nullptr) {
            return find<Descriptor>(Descriptor::right(t), x, compare);
        }

        return splay<Descriptor>(t);
    }

    template <typename Descriptor, typename Iterator, typename T, typename Comparator>
    Iterator find_element(node_t *& root, T const & desired, Comparator const & compare) const noexcept
    {
        node_t * found = find<Descriptor>(root, desired, compare);
        if (found != nullptr) {
            root = found;
            if (Descriptor::value(root) == desired) {
                return Iterator(this, root);
            }
        }
        return Iterator(this, nullptr);
    }

    template <typename Descriptor, typename T, typename Comparator>
    static std::pair<node_t *, node_t *>
    split(node_t * t, T const & x, Comparator const & compare) noexcept
    {
        if (t == nullptr) {
            return std::make_pair(nullptr, nullptr);
        }
        t = find<Descriptor>(t, x, compare);
        if (!compare(x, Descriptor::value(t))) {
            node_t * q = Descriptor::right(t);
            set_parent<Descriptor>(q, nullptr);
            Descriptor::right(t) = nullptr;
            return std::make_pair(t, q);
        }
        else {
            node_t * q = Descriptor::left(t);
            set_parent<Descriptor>(q, nullptr);
            Descriptor::left(t) = nullptr;
            return std::make_pair(q, t);
        }
    }

    template <typename Descriptor, typename Comparator>
    static node_t * merge(node_t * a, node_t * b, Comparator const & compare) noexcept
    {
        if (a == nullptr) {
            return b;
        }
        if (b == nullptr) {
            return a;
        }
        b = find<Descriptor>(b, Descriptor::value(a), compare);
        Descriptor::left(b) = a;
        set_parent<Descriptor>(a, b);
        return b;
    }

    template <typename Descriptor, typename Comparator>
    static void insert(node_t *& root, node_t * new_node, Comparator const & compare)
    {
        std::pair<node_t *, node_t *> p = split<Descriptor>(root, Descriptor::value(new_node), compare);
        root = new_node;
        Descriptor::left(root) = p.first;
        Descriptor::right(root) = p.second;
        set_parent<Descriptor>(p.first, root);
        set_parent<Descriptor>(p.second, root);
    }

    node_t * insert_by_values(Left left, Right right)
    {
        left_root = find<left_descriptor_t>(left_root, left, left_compare);
        right_root = find<right_descriptor_t>(right_root, right, right_compare);
        if ((left_root == nullptr || !(left_descriptor_t::value(left_root) == left)) &&
            (right_root == nullptr || !(right_descriptor_t::value(right_root) == right))) {
            node_t * new_node;
            new_node = new node_t(std::move(left), std::move(right));
            insert<left_descriptor_t>(left_root, new_node, left_compare);
            insert<right_descriptor_t>(right_root, new_node, right_compare);
            ++elements_count;
            return left_root;
        }
        return nullptr;
    }

    template <typename Descriptor, typename Comparator>
    static node_t * erase(node_t *& root, Comparator const & compare) noexcept
    {
        set_parent<Descriptor>(Descriptor::left(root), nullptr);
        set_parent<Descriptor>(Descriptor::right(root), nullptr);
        node_t * new_root = merge<Descriptor>(Descriptor::left(root), Descriptor::right(root), compare);
        node_t * result = root;
        root = new_root;
        return result;
    }

    template <typename FirstDescriptor, typename SecondDescriptor, typename FirstComparator, typename SecondComparator>
    static void erase_root(node_t *& first_root, node_t *& second_root, FirstComparator const & first_compare, SecondComparator const & second_compare, size_t & elements_count) noexcept
    {
        node_t * excess = erase<FirstDescriptor>(first_root, first_compare);
        erase<SecondDescriptor>(second_root, second_compare);
        delete excess;
        --elements_count;
    }

    template <typename FirstDescriptor, typename SecondDescriptor, typename T, typename FirstComparator, typename SecondComparator>
    static void erase_element(node_t *& first_root, node_t *& second_root, T const & key, FirstComparator const & first_compare, SecondComparator const & second_compare, size_t & elements_count) noexcept
    {
        first_root = find<FirstDescriptor>(first_root, key, first_compare);
        second_root = splay<SecondDescriptor>(first_root);
        if (first_root != nullptr && FirstDescriptor::value(first_root) == key) {
            erase_root<FirstDescriptor, SecondDescriptor>(first_root, second_root, first_compare, second_compare, elements_count);
        }
    }

    template <typename Descriptor>
    static node_t * next(node_t const * node) noexcept
    {
        if (Descriptor::right(node) != nullptr) {
            return sink_left<Descriptor>(Descriptor::right(node));
        }

        for (; Descriptor::parent(node) != nullptr; node = Descriptor::parent(node)) {
            if (Descriptor::left(Descriptor::parent(node)) == node) {
                return const_cast<node_t *>(Descriptor::parent(node));
            }
        }

        return nullptr;
    }

    template <typename Descriptor>
    static node_t * previous(node_t const * node) noexcept
    {
        if (Descriptor::left(node) != nullptr) {
            return sink_right(Descriptor::left(node));
        }

        for (; Descriptor::parent(node) != nullptr; node = Descriptor::parent(node)) {
            if (Descriptor::right(Descriptor::parent(node)) == node) {
                return const_cast<node_t *>(Descriptor::parent(node));
            }
        }

        return nullptr;
    }

    void swap(bimap & other) noexcept
    {
        std::swap(left_root, other.left_root);
        std::swap(right_root, other.right_root);
        std::swap(left_compare, other.left_compare);
        std::swap(right_compare, other.right_compare);
        std::swap(elements_count, other.elements_count);
    }

    template <typename Descriptor>
    static node_t * minimum(node_t * node)
    {
        return splay<Descriptor>(sink_left<Descriptor>(node));
    }

    template <typename Descriptor, typename T, typename Comparator>
    static node_t * lower_bound(node_t *& root, T const & x, Comparator const & compare) noexcept
    {
        root = find<Descriptor>(root, x, compare);
        if (root != nullptr) {
            if (!compare(Descriptor::value(root), x)) {
                return root;
            }
            else if (Descriptor::right(root) != nullptr) {
                root = minimum<Descriptor>(Descriptor::right(root));
                return root;
            }
        }
        return nullptr;
    }

    template <typename Descriptor, typename T, typename Comparator>
    static node_t * upper_bound(node_t *& root, T const & x, Comparator const & compare) noexcept
    {
        root = find<Descriptor>(root, x, compare);
        if (root != nullptr) {
            if (compare(x, Descriptor::value(root))) {
                return root;
            }
            else if (Descriptor::right(root) != nullptr) {
                root = minimum<Descriptor>(Descriptor::right(root));
                return root;
            }
        }
        return nullptr;
    }

    template <typename FirstDescriptor, typename SecondDescriptor, typename FirstType, typename SecondType, typename Comparator>
    static SecondType const & at_element(node_t *& root, FirstType const & key, Comparator const & compare)
    {
        root = find<FirstDescriptor>(root, key, compare);
        if (root == nullptr || !(FirstDescriptor::value(root) == key)) {
            throw std::out_of_range("No matching element.");
        }
        return SecondDescriptor::value(root);
    }

    template <typename FirstDescriptor, typename SecondDescriptor, typename FirstType, typename SecondType, typename FirstComparator, typename SecondComparator, typename InsertFunction>
    static SecondType const & at_element_or_default(node_t *& first_root, node_t *& second_root, FirstType const & key, FirstComparator const & first_compare, SecondComparator const & second_compare, InsertFunction const & insert_function, size_t & elements_count) noexcept
    {
        first_root = find<FirstDescriptor>(first_root, key, first_compare);
        if (first_root != nullptr && FirstDescriptor::value(first_root) == key) {
            return SecondDescriptor::value(first_root);
        }
        else {
            SecondType default_value = SecondType();
            erase_element<SecondDescriptor, FirstDescriptor>(second_root, first_root, default_value, second_compare, first_compare, elements_count);
            insert_function();
            return SecondDescriptor::value(second_root);
        }
    }

    mutable node_t * left_root;
    mutable node_t * right_root;
    LeftComparator left_compare;
    RightComparator right_compare;
    size_t elements_count;

public:
    explicit bimap(LeftComparator left_compare = std::move(LeftComparator()),
                   RightComparator right_compare = std::move(RightComparator()))
        : left_root(nullptr)
        , right_root(nullptr)
        , left_compare(left_compare)
        , right_compare(right_compare)
        , elements_count(0)
    {
    }

    bimap(bimap const & other)
        : left_root(nullptr)
        , right_root(nullptr)
        , left_compare(other.left_compare)
        , right_compare(other.right_compare)
        , elements_count(other.elements_count)
    {
        for (left_iterator it = other.begin_left(); it != other.end_left(); ++it) {
            Left const & left = *it;
            Right const & right = *it.flip();
            left_root = find<left_descriptor_t>(left_root, left, left_compare);
            right_root = find<right_descriptor_t>(right_root, right, right_compare);
            node_t * new_node;
            try {
                new_node = new node_t(std::move(left), std::move(right));
            }
            catch (...) {
                throw;
            }
            insert<left_descriptor_t>(left_root, new_node, left_compare);
            insert<right_descriptor_t>(right_root, new_node, right_compare);
        }
    }

    bimap(bimap && other) noexcept
    {
        left_root = other.left_root;
        other.left_root = nullptr;
        left_root = other.left_root;
        other.left_root = nullptr;
        left_compare = std::move(other.left_compare);
        right_compare = std::move(other.right_compare);
        elements_count = other.elements_count;
        other.elements_count = 0;
    }

    bimap & operator=(bimap other)
    {
        swap(other);
        return *this;
    }

    bimap & operator=(bimap && other) noexcept
    {
        swap(other);
        return *this;
    }

    ~bimap()
    {
        std::function<void(node_t *)> dfs;
        dfs = [&dfs](node_t * node) {
            if (node == nullptr) {
                return;
            }
            dfs(left_descriptor_t::left(node));
            dfs(left_descriptor_t::right(node));
            delete node;
        };
        dfs(left_root);
    }

    using left_iterator = basic_iterator<left_descriptor_t, right_descriptor_t, Left, Right>;
    using right_iterator = basic_iterator<right_descriptor_t, left_descriptor_t, Right, Left>;

    left_iterator begin_left() const noexcept
    {
        return left_iterator(this, sink_left<left_descriptor_t>(left_root));
    }

    left_iterator end_left() const noexcept
    {
        return left_iterator(this, nullptr);
    }

    right_iterator begin_right() const noexcept
    {
        return right_iterator(this, sink_left<right_descriptor_t>(right_root));
    }

    right_iterator end_right() const noexcept
    {
        return right_iterator(this, nullptr);
    }

    bool empty() const noexcept
    {
        return (elements_count == 0);
    }

    size_t size() const noexcept
    {
        return elements_count;
    }

    left_iterator find_left(Left const & desired) const noexcept
    {
        return find_element<left_descriptor_t, left_iterator>(left_root, desired, left_compare);
    }

    right_iterator find_right(Right const & desired) const noexcept
    {
        return find_element<right_descriptor_t, right_iterator>(right_root, desired, right_compare);
    }

    left_iterator insert(Left const & left, Right const & right)
    {
        return left_iterator(this, insert_by_values(left, right));
    }

    left_iterator insert(Left const & left, Right && right)
    {
        return left_iterator(this, insert_by_values(left, std::move(right)));
    }

    left_iterator insert(Left && left, Right const & right)
    {
        return left_iterator(this, insert_by_values(std::move(left), right));
    }

    left_iterator insert(Left && left, Right && right)
    {
        return left_iterator(this, insert_by_values(std::move(left), std::move(right)));
    }

    bool erase_left(Left const & key) noexcept
    {
        size_t previous_elements_count = elements_count;
        erase_element<left_descriptor_t, right_descriptor_t>(left_root, right_root, key, left_compare, right_compare, elements_count);
        return (elements_count < previous_elements_count);
    }

    bool erase_right(Right const & key) noexcept
    {
        size_t previous_elements_count = elements_count;
        erase_element<right_descriptor_t, left_descriptor_t>(right_root, left_root, key, right_compare, left_compare, elements_count);
        return (elements_count < previous_elements_count);
    }

    left_iterator erase_left(left_iterator const & it) noexcept
    {
        left_root = splay<left_descriptor_t>(const_cast<node_t *>(it.node));
        right_root = splay<right_descriptor_t>(const_cast<node_t *>(it.node));
        erase_root<left_descriptor_t, right_descriptor_t>(left_root, right_root, left_compare, right_compare, elements_count);
        return left_iterator(this, left_root);
    }

    right_iterator erase_right(right_iterator const & it) noexcept
    {
        left_root = splay<left_descriptor_t>(const_cast<node_t *>(it.node));
        right_root = splay<right_descriptor_t>(const_cast<node_t *>(it.node));
        erase_root<right_descriptor_t, left_descriptor_t>(right_root, left_root, right_compare, left_compare, elements_count);
        return right_iterator(this, right_root);
    }

    left_iterator erase_left(left_iterator first, left_iterator const & last) noexcept
    {
        while (first != last) {
            erase_left(first++);
        }
        return last;
    }

    right_iterator erase_right(right_iterator first, right_iterator const & last) noexcept
    {
        while (first != last) {
            erase_right(first++);
        }
        return last;
    }

    left_iterator lower_bound_left(Left const & value) const noexcept
    {
        return left_iterator(this, lower_bound<left_descriptor_t>(left_root, value, left_compare));
    }

    left_iterator upper_bound_left(Left const & value) const noexcept
    {
        return left_iterator(this, upper_bound<left_descriptor_t>(left_root, value, left_compare));
    }

    right_iterator lower_bound_right(Right const & value) const noexcept
    {
        return right_iterator(this, lower_bound<right_descriptor_t>(right_root, value, right_compare));
    }

    right_iterator upper_bound_right(Right const & value) const noexcept
    {
        return right_iterator(this, upper_bound<right_descriptor_t>(right_root, value, right_compare));
    }

    Right const & at_left(Left const & key) const
    {
        return at_element<left_descriptor_t, right_descriptor_t, Left, Right>(left_root, key, left_compare);
    }

    Left const & at_right(Right const & key) const
    {
        return at_element<right_descriptor_t, left_descriptor_t, Right, Left>(right_root, key, right_compare);
    }

    Right const & at_left_or_default(Left const & key)
    {
        auto insert_function = [this, &key] {
            insert_by_values(key, Right());
        };
        return at_element_or_default<left_descriptor_t, right_descriptor_t, Left, Right>(left_root, right_root, key, left_compare, right_compare, insert_function, elements_count);
    }

    Left const & at_right_or_default(Right const & key)
    {
        auto insert_function = [this, &key] {
            insert_by_values(Left(), key);
        };
        return at_element_or_default<right_descriptor_t, left_descriptor_t, Right, Left>(right_root, left_root, key, right_compare, left_compare, insert_function, elements_count);
    }

    bool operator==(bimap const & other) const noexcept
    {
        if (size() != other.size()) {
            return false;
        }
        for (left_iterator first = begin_left(), second = other.begin_left();
             first != end_left();
             first++, second++) {
            if (*first != *second) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(bimap const & other) const noexcept
    {
        return !(*this == other);
    }
};
