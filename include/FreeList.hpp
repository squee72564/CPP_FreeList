#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <vector>
#include <iterator>
#include <limits>
#include <cstddef>
#include <algorithm>

template<typename T>
class FreeList {
private:
    struct Node {
        T data;
        size_t next;
        size_t prev;
        size_t nextFree;

        Node(const T& data) : data(data), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        Node() : data(T{}), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        ~Node() = default;

	Node(const Node&) = default;
	Node(Node&&) noexcept = default;
	Node& operator=(const Node&) = default;
	Node& operator=(Node&&) noexcept = default;
    };

    std::vector<Node> nodes;
    size_t head;
    size_t tail;
    size_t freeHead;
    size_t size;

    size_t allocateNode(const T& data) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            index = freeHead;
            freeHead = nodes[freeHead].nextFree;
            nodes[index] = Node(data);
        } else {
            index = nodes.size();
            nodes.emplace_back(data);
        }

	size++;

        return index;
    }

    void remove(size_t index) {
        if (index >= nodes.size()) return;

        size_t nextIndex = nodes[index].next;
        size_t prevIndex = nodes[index].prev;

        if (prevIndex == SIZE_MAX) {
            head = nextIndex;
        } else {
            nodes[prevIndex].next = nextIndex;
        }

        if (nextIndex == SIZE_MAX) {
            tail = prevIndex;
        } else {
            nodes[nextIndex].prev = prevIndex;
        }

        nodes[index].nextFree = freeHead;
        freeHead = index;

	size--;
    }

    template <typename Compare = std::less<T> >
    size_t merge(const size_t first, const size_t second, size_t& tail, const Compare& comp = Compare()) {
	if (first == SIZE_MAX) return second;
	if (second == SIZE_MAX) return first;

	size_t result = SIZE_MAX;

	if (comp(nodes[first].data, nodes[second].data)) {
	    result = first;

	    nodes[first].next = merge(nodes[first].next, second, tail, comp);
	    if (nodes[first].next != SIZE_MAX) {
		nodes[nodes[first].next].prev = first;
	    } else {
		tail = first;
	    }
	    nodes[first].prev = SIZE_MAX;
	} else {
	    result = second;

	    nodes[second].next = merge(first, nodes[second].next, tail, comp);
	    if (nodes[second].next != SIZE_MAX) {
		nodes[nodes[second].next].prev = second;
	    } else {
		tail = second;
	    }
	    nodes[second].prev = SIZE_MAX;
	}
	return result;
    }

    std::pair<size_t,size_t> split(const size_t start, const size_t end) {
	if (start == end) return {start, start};

	size_t slow = start;
	size_t fast = start;

	while (fast != end && nodes[fast].next != end) {
	    slow = nodes[slow].next;
	    fast = nodes[nodes[fast].next].next;
	}

	size_t second_half = nodes[slow].next;
	nodes[slow].next = SIZE_MAX;

	if (second_half != SIZE_MAX) {
	    nodes[second_half].prev = SIZE_MAX;
	}
	return {second_half,slow};
    }

    template <typename Compare = std::less<T> >
    size_t mergeSort(size_t start, size_t end, const Compare& comp = Compare()) {
	if (start == SIZE_MAX || start == end) return start;

	auto [second_half_start, start_end] = split(start, end);

	start = mergeSort(start, start_end, comp);
	second_half_start = mergeSort(second_half_start, end, comp);

	size_t _tail = SIZE_MAX;
	size_t _head = merge(start, second_half_start, _tail, comp);
	tail = _tail;

	return _head;
    }

public:
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() : list(nullptr), index(SIZE_MAX) {}

        Iterator(FreeList* list, size_t index)
            : list(list), index(index) {}

        Iterator(const FreeList* list, size_t index)
            : list(const_cast<FreeList*>(list)), index(index) {}

	Iterator(const Iterator&) = default;
	Iterator(Iterator&&) noexcept = default;
	Iterator& operator=(const Iterator&) = default;
	Iterator& operator=(Iterator&&) noexcept = default;

        reference operator*() {
            return list->nodes[index].data;
        }

        pointer operator->() {
            return &list->nodes[index].data;
        }

        Iterator& operator++() {
            index = list->nodes[index].next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
	    ++(*this);
            return temp;
        }

        Iterator& operator--() {
	    if (index == SIZE_MAX) {
		index = list->tail;
	    } else {
		index = list->nodes[index].prev;
	    }
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
	    --(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const {
            return (index == other.index) && (list == other.list);
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        size_t getIndex() const {
            return index;
        }

    private:
        FreeList* list;
        size_t index;
    };

    FreeList() : head(SIZE_MAX), tail(SIZE_MAX), freeHead(SIZE_MAX) {}

    FreeList(const FreeList& other) = default;
    FreeList(FreeList&& other) noexcept = default;
    FreeList& operator=(const FreeList& other) = default;
    FreeList& operator=(FreeList&& other) noexcept = default;

    template <typename Compare = std::less<T> >
    void sort(const Compare& comp = Compare()) {
	if (empty()) return;	

	head = mergeSort(head, tail, comp);
    }

    template <typename Compare = std::less<T> >
    void sort(const Iterator start = begin(), const Iterator _end = end(), const Compare& comp = Compare()) {
	if (empty() || start == end() || start == _end) return;	

	size_t start_idx = start.getIndex();
	size_t end_idx = (_end == end()) ? tail : _end.getIndex();

	head = mergeSort(start_idx, end_idx, comp);
    }

    void reserve(size_t count) {
        nodes.reserve(count);
    }

    void push_front(const T& data) {
        size_t newIndex = allocateNode(data);
    
        if (head != SIZE_MAX) {
            nodes[newIndex].next = head;
            nodes[head].prev = newIndex;
        }
        head = newIndex;
        if (tail == SIZE_MAX) {
            tail = newIndex;
        }
    }
    
    size_t push_back(const T& data) {
        size_t index = allocateNode(data);
    
        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }
    
        return index;
    }
    void erase(Iterator it) {
        remove(it.getIndex());
    }

    Iterator find(const T& value) {
	for (Iterator it = begin(); it != end(); ++it) {
	    if (*it == value) {
		return it;
	    }
	}
	return end();
    }

    Iterator insert(Iterator it, const T& data) {
        size_t newIndex = allocateNode(data);
    
        if (!(it == end())) {
            size_t currentIndex = it.getIndex();
    
            nodes[newIndex].next = currentIndex;
            nodes[newIndex].prev = nodes[currentIndex].prev;
    
            if (nodes[currentIndex].prev != SIZE_MAX) {
                nodes[nodes[currentIndex].prev].next = newIndex;
            } else {
                head = newIndex;
            }
    
            nodes[currentIndex].prev = newIndex;
        } else {

            if (tail != SIZE_MAX) {
                nodes[tail].next = newIndex;
                nodes[newIndex].prev = tail;
            } else {
                head = newIndex;
            }
            tail = newIndex;
        }
    
        return Iterator(this, newIndex);
    }

    T& front() {
        return nodes[head].data;
    }

    T& back() {
        return nodes[tail].data;
    }

    void pop_front() {
        if (head == SIZE_MAX) return;
    
        remove(head);
    }

    void pop_back() {
        if (tail == SIZE_MAX) { 
            return;
        }
    
        remove(tail);
    }

    bool empty() const {
        return head == SIZE_MAX && tail == SIZE_MAX;
    }

    void clear() {
        head = tail = freeHead = SIZE_MAX;
        nodes.clear();
    }

    Iterator begin() {
        return Iterator(this, head);
    }

    Iterator begin() const {
        return Iterator(this, head);
    }

    Iterator end() {
        return Iterator(this, SIZE_MAX);
    }

    Iterator end() const {
        return Iterator(this, SIZE_MAX);
    }
};

#endif
