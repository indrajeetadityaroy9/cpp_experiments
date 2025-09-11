#ifndef LRU_H
#define LRU_H

#include <unordered_map>
#include <string>
#include <cstdlib>

template<typename T>
class LRUCache {
private:
    struct Node {
        std::string key;
        T value;
        Node* prev;
        Node* next;
        
        Node() : prev(nullptr), next(nullptr) {}
        Node(const std::string& k, const T& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    
    int capacity;
    std::unordered_map<std::string, Node*> map;
    Node* head;
    Node* tail;
    int size;
    
    void addAfterHead(Node* node);
    void removeNode(Node* node);
    void moveToHead(Node* node);
    Node* popLRU();
    void clearAll();

public:
    explicit LRUCache(int item_limit);
    ~LRUCache();
    bool has(const std::string& key);
    T get(const std::string& key);
    void set(const std::string& key, const T& value);
    
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
};

#endif