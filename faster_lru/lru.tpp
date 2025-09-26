template<typename T>
LRUCache<T>::LRUCache(int item_limit)
    : capacity(std::max(0, item_limit)), head(new Node()), tail(new Node()), size(0) {
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
void LRUCache<T>::addAfterHead(Node* node) {
    node->prev = head;
    node->next = head->next;
    head->next->prev = node;
    head->next = node;
}

template<typename T>
void LRUCache<T>::removeNode(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

template<typename T>
void LRUCache<T>::moveToHead(Node* node) {
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
bool LRUCache<T>::has(const std::string& key) const {
    return map.find(key) != map.end();
}

template<typename T>
std::optional<T> LRUCache<T>::get(const std::string& key) {
    auto it = map.find(key);
    if (it == map.end()) {
        return std::nullopt;
    }
    moveToHead(it->second);
    return it->second->value;
}

template<typename T>
void LRUCache<T>::set(const std::string& key, const T& value) {
    if (capacity == 0) {
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
    map.emplace(key, newNode);
    addAfterHead(newNode);
    ++size;

    if (size > capacity) {
        Node* lruNode = popLRU();
        map.erase(lruNode->key);
        delete lruNode;
        --size;
    }
}

template<typename T>
void LRUCache<T>::clearAll() {
    Node* current = head->next;
    while (current != tail) {
        Node* next = current->next;
        delete current;
        current = next;
    }

    head->next = tail;
    tail->prev = head;
    map.clear();
    size = 0;
}
