#include <unordered_map>
#include <iostream>
#include <cassert>
#include <list>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <random>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <functional>

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
double measure_insertion(Container& container, size_t count) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        container.push_back(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Insertion time: " << duration.count() << " seconds\n";

    return duration.count();
}

template<typename Container>
double measure_deletion(Container& container) {
    auto start = std::chrono::high_resolution_clock::now();
    while (!container.empty()) {
        container.pop_back();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Deletion time: " << duration.count() << " seconds\n";

    return duration.count();
}

template<typename Container>
double measure_iteration(Container& container) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto it = container.begin(); it != container.end(); ++it) {}
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Iteration time: " << duration.count() << " seconds\n";

    return duration.count();
}

void test_performance() {
    const size_t count = 400000000;
    
    std::list<int> stdList;
    FreeList<int> freeList;

    freeList.reserve(count);

    double total = 0.0f;
    double total2 = 0.0f;

    std::cout << "Testing std::list with count == " << count << "\n";
    total += measure_insertion(stdList, count);
    total += measure_iteration(stdList);
    total += measure_deletion(stdList);
    stdList.clear();
    std::cout << "Total time: " << total << "\n";

    std::cout << "\nTesting FreeList with count == " << count << "\n";
    total2 += measure_insertion(freeList, count);
    total2 += measure_iteration(freeList);
    total2 += measure_deletion(freeList);
    freeList.clear();
    std::cout << "Total time: " << total2 << "\n";

    std::cout << "FreeList was " << (total/total2) << " times faster\n";

    std::cout << std::endl;
}

void test_STL_functions() {
    FreeList<int> freeList{1,2,1,1,3,3,3,4,5,4};

    std::cout << "Before std::unique()\n";
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
	std::cout << *it << " ";
    }

    auto last = std::unique(freeList.begin(), freeList.end());

    freeList.erase(last, freeList.end());

    std::cout << "\nAfter std::unique()\n";
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
	std::cout << *it << " ";
    }

    std::cout << "\n\n";

    FreeList<int> from_fl(10);
    std::iota(from_fl.begin(), from_fl.end(), 0);
 
    FreeList<int> to_fl;
    std::copy(from_fl.begin(), from_fl.end(),
              std::back_inserter(to_fl));
 
    std::cout << "to_fl contains: ";
 
    std::copy(to_fl.begin(), to_fl.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << "\n\n";
 
    std::cout << "odd numbers in to_fl are: ";
 
    std::copy_if(to_fl.begin(), to_fl.end(),
                 std::ostream_iterator<int>(std::cout, " "),
                 [](int x) { return x % 2 != 0; });
    std::cout << "\n\n";

    std::cout << "to_fl contains these multiples of 3: ";
 
    to_fl.clear();
    std::copy_if(from_fl.begin(), from_fl.end(),
                 std::back_inserter(to_fl),
                 [](int x) { return x % 3 == 0; });
 
    for (const int x : to_fl)
        std::cout << x << ' ';
    std::cout << "\n\n";

    auto print = [](const auto remark, const auto& v)
    {
        std::cout << remark;
        for (auto n : v)
            std::cout << n << ' ';
        std::cout << '\n';
    };

    FreeList<int> fl{2, 4, 2, 0, 5, 10, 7, 3, 7, 1};
    print("before sort:\t\t", fl);
 
    // insertion sort
    for (auto i = fl.begin(); i != fl.end(); ++i)
        std::rotate(std::upper_bound(fl.begin(), i, *i), i, std::next(i));
    print("after sort:\t\t", fl);
 
    // simple rotation to the left
    std::rotate(fl.begin(), std::next(fl.begin()), fl.end());
    print("simple rotate left:\t", fl);
 
    // simple rotation to the right
    std::rotate(fl.rbegin(), std::next(fl.rbegin()), fl.rend());
    print("simple rotate right:\t", fl);

    std::cout << "\n";

    fl = FreeList<int>(10, 2);
    std::partial_sum(fl.cbegin(), fl.cend(), fl.begin());
    std::cout << "Among the numbers: ";
    std::copy(fl.cbegin(), fl.cend(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << '\n';
 
    if (std::all_of(fl.cbegin(), fl.cend(), [](int i) { return i % 2 == 0; }))
        std::cout << "All numbers are even\n";
 
    if (std::none_of(fl.cbegin(), fl.cend(), std::bind(std::modulus<>(),
                                                     std::placeholders::_1, 2)))
        std::cout << "None of them are odd\n";
 
    struct DivisibleBy
    {
        const int d;
        DivisibleBy(int n) : d(n) {}
        bool operator()(int n) const { return n % d == 0; }
    };
 
    if (std::any_of(fl.cbegin(), fl.cend(), DivisibleBy(7)))
        std::cout << "At least one number is divisible by 7\n\n";
}

void test_LFUCache() {
    LFUCache* obj = new LFUCache(2);
    std::cout << "LFUCache(2)\n";

    obj->put(1,1);
    std::cout << "Put(1)\n";

    obj->put(2,2);
    std::cout << "Put(2)\n";
    
    int t = obj->get(1);
    std::cout << "Expected 1, get(1) = " << t << "\n";
    assert(t == 1);
    
    obj->put(3,3);
    std::cout << "Put(5)\n";
    
    t = obj->get(2);
    std::cout << "Expected -1, get(2) = " << t << "\n";
    assert(t == -1);
    
    t = obj->get(3);
    std::cout << "Expected 3, get(3) = " << t << "\n";
    assert(t == 3);
    
    obj->put(5,5);
    std::cout << "Put(5)\n";
    
    t = obj->get(1);
    std::cout << "Expected -1, get(1) = " << t << "\n";
    assert(t == -1);
    
    t = obj->get(3);
    std::cout << "Expected 3, get(3) = " << t << "\n";
    assert(t == 3);

    t = obj->get(5);
    std::cout << "Expected 5, get(5) = " << t << "\n\n";
    assert(t == 5);
}

void test_mergeSort() {
    FreeList<int> freeList;
    std::vector<int> vec;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(-999,999);

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

    freeList.sort(std::greater<int>());
    std::sort(vec.begin(), vec.end(), std::greater<int>());

    std::cout << "\nAfter sort\n";
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
	std::cout << *it << " ";
    }

    for (const int i : vec) {
	assert(i == freeList.front());
	freeList.pop_front();
    }

    std::cout << "\n\n";
}

int main() {
    test_mergeSort();
    test_LFUCache();
    test_STL_functions();
    test_performance();
    return 0;
}

