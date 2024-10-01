#include <unordered_map>
#include <iostream>
#include <cassert>
#include <list>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <random>

#include "FreeList.hpp"

using namespace std;

class LFUCache {
private:
    struct Node {
        FreeList<pair<int,int>> data;
        int freq;
        Node() : data(), freq(-1) {}
        Node(int f) : data(), freq(f) {}
    };

    FreeList<Node> nodeList;
    unordered_map<int,FreeList<Node>::Iterator> keyLFU;
    unordered_map<int,FreeList<pair<int,int>>::Iterator> keyLRU;
    int cap;
    int size;
    
public:
    LFUCache(int capacity) : nodeList(), keyLFU(), keyLRU(), cap(capacity), size(0) {
	nodeList.reserve(cap+1);
    }

     void print() {
        auto it = nodeList.begin();
        while (it != nodeList.end()) {
                const Node& curr_node = *it;
                std::cout << "{ freq: " << curr_node.freq << ", { ";
                auto it2 = curr_node.data.begin();
                while (it2 != curr_node.data.end()) {
                        const auto& [k,v] = *it2;
                        std::cout << "(" << k << "," << v << ") ";
                        it2++;
                }
                std::cout << "} }\t";
                it++;
        }
        std::cout << std::endl;
    }   

    int get(int key) {
        auto it = keyLFU.find(key);

        if (it == keyLFU.end()) return -1;

        // First get the k,v pair; add to next node with freq+1; remove from current node
        auto currIt = it->second;
        auto nextIt = next(currIt);
        const auto [_, value] = *keyLRU[key];

        auto listIt = (nextIt == nodeList.end() || nextIt->freq != (currIt->freq+1))
            ? nodeList.insert(nextIt, Node(currIt->freq+1))
            : nextIt;
        
        currIt->data.erase(keyLRU[key]);
        if (currIt->data.empty()) {
            nodeList.erase(currIt);
        }
            
        keyLRU[key] = listIt->data.insert(listIt->data.end(), {key,value});
        keyLFU[key] = listIt;

        return value;
    }
    
    void put(int key, int value) {
        if (cap == 0) return;

        auto it = keyLFU.find(key);

        if (it == keyLFU.end()) {
            if (size == cap) { // Eject LRU from LFU
                auto LFU = nodeList.begin();
                auto [k,_] = LFU->data.front();
                LFU->data.pop_front();
                keyLRU.erase(k);
                keyLFU.erase(k);

                if (LFU->data.empty()) {
                    nodeList.erase(LFU);
                }

                size--;
            }

            auto listIt = (nodeList.empty() || nodeList.begin()->freq != 1)
                ? nodeList.insert(nodeList.begin(), Node(1))
                : nodeList.begin();
            
            keyLRU[key] = listIt->data.insert(listIt->data.end(), {key, value});
            keyLFU[key] = listIt;
            size++;

            return;
        }

        // Increment in nodeList and update LFU/LRU iterators
        auto currIt = it->second;
        auto listIt = (next(currIt) == nodeList.end() || next(currIt)->freq != (currIt->freq+1))
            ? nodeList.insert(next(currIt), Node(currIt->freq+1))
            : next(currIt);

        currIt->data.erase(keyLRU[key]);
        if (currIt->data.empty()) {
            nodeList.erase(currIt);
        }
        
        keyLRU[key] = listIt->data.insert(listIt->data.end(), {key,value});
        keyLFU[key] = listIt;
    }
};

template<typename Container>
void measure_insertion(Container& container, size_t count) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        container.push_back(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Insertion time: " << duration.count() << " seconds\n";
}

template<typename Container>
void measure_deletion(Container& container) {
    auto start = std::chrono::high_resolution_clock::now();
    while (!container.empty()) {
        container.pop_back();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Deletion time: " << duration.count() << " seconds\n";
}

template<typename Container>
void measure_iteration(Container& container) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto it = container.begin(); it != container.end(); ++it) {}
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Iteration time: " << duration.count() << " seconds\n";
}

void test_performance() {
    const size_t count = 100000000;
    
    std::list<int> stdList;
    FreeList<int> freeList;

    freeList.reserve(count);

    std::cout << "Testing std::list...\n";
    measure_insertion(stdList, count);
    stdList.clear();
    measure_insertion(stdList, count); // Reinsert for iteration test
    measure_iteration(stdList);
    measure_deletion(stdList);

    std::cout << "\nTesting FreeList...\n";
    measure_insertion(freeList, count);
    freeList.clear();
    measure_insertion(freeList, count); // Reinsert for iteration test
    measure_iteration(freeList);
    measure_deletion(freeList);
}

void test_LFUCache() {
    LFUCache* obj = new LFUCache(2);
    obj->put(1,1);
    std::cout << "1\n";
    obj->print();

    obj->put(2,2);
    std::cout << "2\n";
    obj->print();
    
    int t = obj->get(1);
    std::cout << "3: " << t << "\n";
    assert(t == 1);
    obj->print();
    
    obj->put(3,3);
    std::cout << "4\n";
    obj->print();
    
    t = obj->get(2);
    std::cout << "5: " << t << "\n";
    assert(t == -1);
    obj->print();
    
    t = obj->get(3);
    std::cout << "6: " << t << "\n";
    assert(t == 3);
    obj->print();
    
    obj->put(5,5);
    std::cout << "7\n";
    obj->print();
    
    t = obj->get(1);
    std::cout << "8: " << t << "\n";
    assert(t == -1);
    obj->print();
    
    t = obj->get(3);
    std::cout << "9: " << t << "\n";
    assert(t == 3);
    obj->print();

    t = obj->get(5);
    std::cout << "10: " << t << "\n";
    obj->print();
    assert(t == 5);
}

void test_mergeSort() {
    FreeList<int> freeList;
    std::vector<int> vec;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(-99,99);

    for (int i = 0; i < 25; ++i) {
	const int t = dist(gen);
	vec.push_back(t);
	freeList.push_back(t);
    }

    std::cout << "Before sort\n";
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
	std::cout << *it << " ";
    }
    std::cout << "\n";

    freeList.sort(freeList.begin(), freeList.end());
    std::sort(vec.begin(), vec.end());

    std::cout << "After sort\n";
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
	std::cout << *it << " ";
    }

    for (const int i : vec) {
	assert(i == freeList.front());
	freeList.pop_front();
    }

    std::cout << "\n";
}

int main() {
    test_mergeSort();
    test_LFUCache();
    test_performance();
    return 0;
}

