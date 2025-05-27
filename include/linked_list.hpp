#pragma once
#include <memory>
#include <optional>
#include <utility>

namespace rv {

/*
Single-linked list node with RAII, allocator-aware.
*/
template <typename K, typename V>
struct ListNode
{
    K key;
    V val;
    std::unique_ptr<ListNode> next{};

    ListNode(const K& k, const V& v)
        : key{k}, val{v} {}
    
    ListNode(K&& k, V&& v) noexcept(std::is_nothrow_move_constructible_v<K> && std::is_nothrow_move_constructible_v<V>)
        : key{std::move(k)}, val{std::move(v)} {}
};

/*
Minimal forward-list map used by the open-hash buckets (non-thread-safe).
*/
template <typename K, typename V>
class LinkedList
{
  public:
    [[nodiscard]] std::optional<V> find(const K& key) const
    {
        for (auto* n = head_.get(); n; n = n->next.get())
            if (n->key == key) return n->val;
        return std::nullopt;
    }

    /*
    Insert or assign.  Returns true if replaced, false if added.
    */
    bool put(const K& key, const V& val)
    {
        for (auto* n = head_.get(); n; n = n->next.get())
            if (n->key == key) { n->val = val; return true; }

        head_ = std::make_unique<ListNode<K,V>>(key, val, std::move(head_));
        ++size_; return false;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

  private:
    std::unique_ptr<ListNode<K,V>> head_{};
    std::size_t                    size_{0};
};

} // namespace rv
