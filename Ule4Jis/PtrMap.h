#pragma once

#include <map>

template<typename K, typename V>
class PtrMap {
public:
    typedef std::map<K, V*> MapType;
    typedef typename MapType::iterator iterator;

    PtrMap() {}
    ~PtrMap() { clear(); }

    void clear() {
        for (iterator it = m.begin(); it != m.end(); ++it) {
            delete it->second;
        }
        m.clear();
    }

    // Insert and take ownership of the pointer. If key exists, replace and delete old value.
    std::pair<iterator, bool> insert(const K &k, V *v) {
        iterator it = m.find(k);
        if (it != m.end()) {
            delete it->second;
            it->second = v;
            return std::make_pair(it, true);
        } else {
            std::pair<iterator, bool> res = m.insert(std::make_pair(k, v));
            return res;
        }
    }

    iterator find(const K &k) { return m.find(k); }
    iterator begin() { return m.begin(); }
    iterator end() { return m.end(); }
    bool empty() const { return m.empty(); }
    size_t size() const { return m.size(); }

    void erase(iterator it) {
        if (it != m.end()) {
            delete it->second;
            m.erase(it);
        }
    }

private:
    MapType m;
};
