template<typename T>
LRUCache<T>::LRUCache(int item_limit)
    : capacity(max(0, item_limit)), head(new Node()), tail(new Node()), size(0) {
    head->next = tail;
    tail->prev = head;
}

template<typename T>
LRUCache<T>::~LRUCache() {
    clearAll();
    delete head;
    delete tail;
}

template<typename T>
constexpr void LRUCache<T>::addAfterHead(Node* node) {
    node->prev = head;
    node->next = head->next;
    head->next->prev = node;
    head->next = node;
}

template<typename T>
constexpr void LRUCache<T>::removeNode(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

template<typename T>
constexpr void LRUCache<T>::moveToHead(Node* node) {
    removeNode(node);
    addAfterHead(node);
}

template<typename T>
typename LRUCache<T>::Node* LRUCache<T>::popLRU() {
    Node* node = tail->prev;
    removeNode(node);
    return node;
}

template<typename T>
bool LRUCache<T>::has(string_view key) const {
    return map.contains(key);
}

template<typename T>
expected<T, CacheError> LRUCache<T>::get(string_view key)
    requires is_copy_constructible_v<T>
{
    auto it = map.find(key);
    if (it == map.end()) {
        return unexpected(CacheError::KeyNotFound);
    }
    moveToHead(it->second);
    return it->second->value;
}

template<typename T>
expected<reference_wrapper<const T>, CacheError> LRUCache<T>::get_ref(string_view key) const {
    auto it = map.find(key);
    if (it == map.end()) {
        return unexpected(CacheError::KeyNotFound);
    }
    return cref(it->second->value);
}

template<typename T>
optional<T> LRUCache<T>::get_optional(string_view key)
    requires is_copy_constructible_v<T>
{
    return get(key)
        .transform([](const T& val) { return val; })
        .or_else([](CacheError) { return optional<T>{}; })
        .value();
}

template<typename T>
void LRUCache<T>::set(string_view key, const T& value) {
    if (capacity == 0) [[unlikely]] {
        if (!map.empty()) {
            clearAll();
        }
        return;
    }

    auto it = map.find(key);
    if (it != map.end()) {
        it->second->value = value;
        moveToHead(it->second);
        return;
    }

    Node* newNode = new Node(key, value);
    map.emplace(string(key), newNode);
    addAfterHead(newNode);
    ++size;

    if (size > capacity) [[unlikely]] {
        Node* lruNode = popLRU();
        map.erase(lruNode->key);
        delete lruNode;
        --size;
    }
}

template<typename T>
void LRUCache<T>::set(string_view key, T&& value) {
    if (capacity == 0) [[unlikely]] {
        if (!map.empty()) {
            clearAll();
        }
        return;
    }

    auto it = map.find(key);
    if (it != map.end()) {
        it->second->value = std::move(value);
        moveToHead(it->second);
        return;
    }

    Node* newNode = new Node(key, std::move(value));
    map.emplace(string(key), newNode);
    addAfterHead(newNode);
    ++size;

    if (size > capacity) [[unlikely]] {
        Node* lruNode = popLRU();
        map.erase(lruNode->key);
        delete lruNode;
        --size;
    }
}

template<typename T>
void LRUCache<T>::clearAll() {
    auto node_range = views::iota(0)
        | views::transform([this, node = head->next](int) mutable {
            Node* current = node;
            if (current != tail) {
                node = node->next;
            }
            return current;
        })
        | views::take_while([this](Node* n) { return n != tail; });

    for (Node* node : node_range) {
        delete node;
    }

    head->next = tail;
    tail->prev = head;
    map.clear();
    size = 0;
}
