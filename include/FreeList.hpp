#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <vector>
#include <iterator>
#include <limits>
#include <cstddef>
#include <algorithm>
#include <exception>

template<typename T>
class FreeList {
private:
    struct Node {
        T data;
        size_t next;
        size_t prev;
        size_t nextFree;

        Node(const T& data) : data(data), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        Node(T&& data) : data(std::move(data)), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
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
    size_t size_;

    template <typename U>
    size_t allocateNode(U&& data) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            index = freeHead;
            freeHead = nodes[freeHead].nextFree;
            nodes[index] = Node(std::forward<T>(data));
        } else {
            index = nodes.size();
            nodes.emplace_back(std::forward<T>(data));
        }

        size_++;
        return index;
    }

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

        size_++;

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

	size_--;
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

    class ConstIterator {
    public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = const T*;
	using reference = const T&;

	ConstIterator() : list(nullptr), index(SIZE_MAX) {}

	ConstIterator(const FreeList* list, size_t index)
	    : list(list), index(index) {}

	ConstIterator(const Iterator& it)
	    : list(it.list), index(it.getIndex()) {}

	ConstIterator(const ConstIterator&) = default;
	ConstIterator(ConstIterator&&) noexcept = default;
	ConstIterator& operator=(const ConstIterator&) = default;
	ConstIterator& operator=(ConstIterator&&) noexcept = default;

	reference operator*() const {
	    return list->nodes[index].data;
	}

	pointer operator->() const {
	    return &list->nodes[index].data;
	}

	ConstIterator& operator++() {
	    index = list->nodes[index].next;
	    return *this;
	}

	ConstIterator operator++(int) {
	    ConstIterator temp = *this;
	    ++(*this);
	    return temp;
	}

	ConstIterator& operator--() {
	    if (index == SIZE_MAX) {
		index = list->tail;
	    } else {
		index = list->nodes[index].prev;
	    }
	    return *this;
	}

	ConstIterator operator--(int) {
	    ConstIterator temp = *this;
	    --(*this);
	    return temp;
	}

	bool operator==(const ConstIterator& other) const {
	    return (index == other.index) && (list == other.list);
	}

	bool operator!=(const ConstIterator& other) const {
	    return !(*this == other);
	}

	size_t getIndex() const {
	    return index;
	}

    private:
	const FreeList* list; // Pointer to const FreeList
	size_t index;
    };

    FreeList() : head(SIZE_MAX), tail(SIZE_MAX), freeHead(SIZE_MAX) {}

    FreeList(size_t count, const T& value = T()) : FreeList() {
	for (size_t i = 0; i < count; ++i) {
	    push_back(value);
	}
    }

    explicit FreeList(size_t count) : FreeList() {
	for (size_t i = 0; i < count; ++i) {
	    push_back(T());
	}
    }

    template <class InputIt>
    FreeList(InputIt first, InputIt last) : FreeList() {
	for (auto it = first; it != last; ++it) {
	    push_back(*it);
	}
    }

    FreeList(std::initializer_list<T> init) : FreeList() {
	for (const auto& value : init) {
	    push_back(value);
	}
    }

    ~FreeList() = default;

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

    template <typename U>
    void push_front(U&& data) {
        size_t index = allocateNode(std::forward<U>(data));
    
        if (head != SIZE_MAX) {
            nodes[index].next = head;
            nodes[head].prev = index;
        }
        head = index;
        if (tail == SIZE_MAX) {
            tail = index;
        }
    }
    
    template <typename U>
    void push_back(U&& data) {
        size_t index = allocateNode(std::forward<U>(data));
    
        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }
    }

    template<class... Args>
    Iterator emplace(ConstIterator pos, Args&&... args) {

	if (pos == end()) {

	    return insert(end(), T(std::forward<Args>(args)...));
	}

	size_t currentIndex = pos.getIndex();
	
	size_t newIndex = allocateNode(T(std::forward<Args>(args)...));

	nodes[newIndex].next = currentIndex;
	nodes[newIndex].prev = nodes[currentIndex].prev;

	if (nodes[currentIndex].prev != SIZE_MAX) {
	    nodes[nodes[currentIndex].prev].next = newIndex;
	} else {
	    head = newIndex;
	}
	
	nodes[currentIndex].prev = newIndex;

	return Iterator(this, newIndex);
    }

    template<typename... Args>
    void emplace_front(Args&&... args) {
	size_t index = allocateNode(T(std::forward<Args>(args)...));

        if (head != SIZE_MAX) {
            nodes[index].next = head;
            nodes[head].prev = index;
        }
        head = index;
        if (tail == SIZE_MAX) {
            tail = index;
        }
    }


    template<typename... Args>
    T& emplace_back(Args&&... args) {
	size_t index = allocateNode(T(std::forward<Args>(args)...));

        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }

	return nodes[index].data;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
	size_t index = allocateNode(T(std::forward<Args>(args)...));

        if (head == SIZE_MAX) {
            head = index;
            tail = index;
        } else {
            nodes[tail].next = index;
            nodes[index].prev = tail;
            tail = index;
        }
    }

    Iterator erase(Iterator pos) {
	Iterator next = pos;
	++next;
	remove(pos.getIndex());
	return next;
    }

    ConstIterator erase(ConstIterator pos) {
	ConstIterator next = pos;
	++next;
	remove(pos.getIndex());
	return next;
    }

    Iterator erase(Iterator first, Iterator last) {
	while (first != last) {
	    first = erase(first);
	}

	return last;
    }

    ConstIterator erase(ConstIterator first, ConstIterator last) {
	while (first != last) {
	    first = erase(first);
	}

	return last;
    }

    Iterator find(const T& value) {
	for (Iterator it = begin(); it != end(); ++it) {
	    if (*it == value) {
		return it;
	    }
	}
	return end();
    }

    template <typename U>
    Iterator insert(Iterator it, U&& data) {
        size_t newIndex = allocateNode(std::forward<U>(data));
    
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

    template<class InputIt>
    Iterator insert(Iterator pos, InputIt first, InputIt last) {
	size_t currentIndex = pos.getIndex();
	size_t firstNewIndex = SIZE_MAX;

	while (first != last) {
	    size_t newIndex = allocateNode(*first++);
	    
	    if (firstNewIndex == SIZE_MAX) {
		firstNewIndex = newIndex;
	    }

	    // Insert the new node in front of currentIndex
	    if (currentIndex != SIZE_MAX) {
		nodes[newIndex].next = currentIndex;
		nodes[newIndex].prev = nodes[currentIndex].prev;

		if (nodes[currentIndex].prev != SIZE_MAX) {
		    nodes[nodes[currentIndex].prev].next = newIndex;
		} else {
		    head = newIndex;
		}

		nodes[currentIndex].prev = newIndex;
		currentIndex = newIndex; // Update for the next insertion
	    } else {
		// Inserting at the end
		if (tail != SIZE_MAX) {
		    nodes[tail].next = newIndex;
		    nodes[newIndex].prev = tail;
		} else {
		    head = newIndex; // List was empty
		}
		tail = newIndex;
	    }
	}

	return Iterator(this, firstNewIndex);
    }

    Iterator insert(Iterator pos, std::initializer_list<T> ilist) {
	return insert(pos, ilist.begin(), ilist.end());
    }

    void swap(FreeList& other) noexcept {
	std::swap(head, other.head);
	std::swap(tail, other.tail);
	std::swap(freeHead, other.freeHead);
	nodes.swap(other.nodes);
	std::swap(size_, other.size_);
    }

    const T& front() const {
        return nodes[head].data;
    }

    const T& back() const {
        return nodes[tail].data;
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

    bool empty() const noexcept {
        return head == SIZE_MAX && tail == SIZE_MAX;
    }

    size_t size() const noexcept {
	return size_;
    }

    size_t capacity() const noexcept {
	return nodes.capacity();
    }

    void shrink_to_fit() {
	nodes.shrink_to_fit();
    }

    void clear() {
        head = tail = freeHead = SIZE_MAX;
	size_ = 0;
        nodes.clear();
    }

    using reverse_iterator = std::reverse_iterator<Iterator>;
    using const_reverse_iterator = std::reverse_iterator<ConstIterator>;

    Iterator begin() { return Iterator(this, head); }
    ConstIterator begin() const { return ConstIterator(this, head); }
    ConstIterator cbegin() const noexcept { return ConstIterator(this, head); }

    Iterator end() { return Iterator(this, SIZE_MAX); }
    ConstIterator end() const { return ConstIterator(this, SIZE_MAX); }
    ConstIterator cend() const noexcept { return ConstIterator(this, SIZE_MAX); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
};

#endif
