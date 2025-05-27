#pragma once
#include <atomic>
#include <memory>
#include <optional>

namespace rv {

template <typename K, typename V>
class LockFreeList
{
    struct Node {
        K     key;
        V     val;
        Node* next;
        Node(const K& k, const V& v, Node* n) : key{k}, val{v}, next{n} {}
        Node(K&& k, V&& v, Node* n) noexcept
            : key{std::move(k)}, val{std::move(v)}, next{n} {}
    };

    struct Head { Node* link; unsigned cnt; };

    std::atomic<Head> head_{ Head{nullptr,0} };
    std::atomic<std::size_t> sz_{0};

  public:
    LockFreeList() = default;
    ~LockFreeList() { clear(); }

    /*
    visit every node (read-only) - used by table rehash
    */
    template <typename Fn>
    void for_each(Fn&& fn) const
    {
        for (Node* n = head_.load().link; n; n = n->next) {
            fn(n->key, n->val);
        }
    }

    [[nodiscard]]
    std::optional<V> find(const K& key) const
    {
        for (Node* n = head_.load().link; n; n = n->next)
            if (n->key == key) return n->val;
        return std::nullopt;
    }

    /*
    insert-or-assign - lock-free
    */
    bool put(const K& key, const V& val)
    {
        Head exp = head_.load();
        Head nw;

        for (;;) {
            /* 1. search for existing key on read snapshot */
            for (Node* n = exp.link; n; n = n->next)
                if (n->key == key) { n->val = val; return true; }

            /* 2. not found -> create new node & try to CAS */
            Node* nn = new Node(key, val, exp.link);
            nw.link  = nn;
            nw.cnt   = exp.cnt + 1;

            if (head_.compare_exchange_weak(exp, nw)) {
                sz_.fetch_add(1);
                return false; // inserted
            }
            /* CAS failed -> exp is updated, retry loop */
            delete nn; // reclaim & retry
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return sz_.load(); }

    void clear() noexcept
    {
        Head h = head_.exchange( Head{nullptr,0} );
        Node* n = h.link;
        while (n) { Node* d = n; n = n->next; delete d; }
        sz_.store(0);
    }

    LockFreeList(const LockFreeList&)            = delete;
    LockFreeList& operator=(const LockFreeList&) = delete;
};

} // namespace rv
