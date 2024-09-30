#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <vector>
#include <iterator>
#include <limits>
#include <cstddef>

template<typename T>
class FreeList {
private:
    struct Node {
        T data;
        size_t next;
        size_t prev;
        size_t nextFree;  // Pointer to the next free node

        Node(const T& data) : data(data), next(SIZE_MAX), prev(SIZE_MAX), nextFree(SIZE_MAX) {}
        Node() : nextFree(SIZE_MAX) {}  // Default constructor for free nodes
    };

    std::vector<Node> nodes;
    size_t head;
    size_t tail;
    size_t freeHead;  // Pointer to the first free node

    size_t allocateNode(const T& data) {
        size_t index;

        if (freeHead != SIZE_MAX) {
            index = freeHead;  // Get the first free node
            freeHead = nodes[freeHead].nextFree;  // Update the free list head
            nodes[index] = Node(data);  // Initialize the new node
        } else {
            index = nodes.size();
            nodes.emplace_back(data);  // Create a new node
        }

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

        // Reset the node and add it to the free list
        nodes[index].nextFree = freeHead;  // Point to the previous free list head
        freeHead = index;                   // Update free list head
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
            index = list->nodes[index].next;
            return temp;
        }

        Iterator& operator--() {
            index = list->nodes[index].prev;
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
            index = list->nodes[index].prev;
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

size_t merge(size_t first, size_t second, size_t& tail) {
    if (first == SIZE_MAX) return second;
    if (second == SIZE_MAX) return first;
	std::cout << "merge\n";

    size_t result = SIZE_MAX;

    if (nodes[first].data < nodes[second].data) {
        result = first;
        // Merge the next node
        nodes[first].next = merge(nodes[first].next, second, tail);
        if (nodes[first].next != SIZE_MAX) {
            nodes[nodes[first].next].prev = first;  // Ensure previous link
        } else {
            tail = first;  // Update tail
        }
        nodes[first].prev = SIZE_MAX;  // First node has no previous
    } else {
        result = second;
        // Merge the next node
        nodes[second].next = merge(first, nodes[second].next, tail);
        if (nodes[second].next != SIZE_MAX) {
            nodes[nodes[second].next].prev = second;  // Ensure previous link
        } else {
            tail = second;  // Update tail
        }
        nodes[second].prev = SIZE_MAX;  // Second node has no previous
    }
    return result;
}

size_t split(size_t start, size_t end) {
    if (start == end) return end;
	std::cout << "split\n";

    size_t slow = start;
    size_t fast = start;

    // Ensure you are not exceeding the bounds
    while (fast != end && nodes[fast].next != end) {
	    std::cout << slow << " " << fast << " " << end << "\n";
        slow = nodes[slow].next;
        fast = nodes[nodes[fast].next].next;
    }

    size_t second_half = nodes[slow].next;
    nodes[slow].next = SIZE_MAX;  // End the first half
				  //
    if (second_half != SIZE_MAX) {
        nodes[second_half].prev = SIZE_MAX;  // Disconnect previous link of the second half
    }
    return second_half;
}

size_t mergeSort(size_t start, size_t end) {
    if (start == SIZE_MAX || start == end) return start;
	std::cout << "sort\n";

    size_t second_half = split(start, end);

    // Recursive sorting
    start = mergeSort(start, second_half);
    second_half = mergeSort(second_half, end);  // Corrected from using `tail`

    size_t _tail = SIZE_MAX;
    size_t _head = merge(start, second_half, _tail);
    tail = _tail;  // Ensure tail is updated correctly

    return _head;
}
    void sort(Iterator start, Iterator _end) {
	if (start == end() || start == _end) return;	

	size_t start_idx = start.getIndex();
	size_t end_idx = (_end == end()) ? tail : _end.getIndex();

	head = mergeSort(start_idx, end_idx);
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

    Iterator insert(Iterator it, const T& data) {
        size_t newIndex = allocateNode(data); // Allocate a new node with data
    
        if (!(it == end())) {
            size_t currentIndex = it.getIndex();
    
            // Update pointers
            nodes[newIndex].next = currentIndex; // New node points to current
            nodes[newIndex].prev = nodes[currentIndex].prev; // Previous node is set
    
            if (nodes[currentIndex].prev != SIZE_MAX) {
                nodes[nodes[currentIndex].prev].next = newIndex; // Link previous to new
            } else {
                head = newIndex; // New node becomes the head
            }
    
            nodes[currentIndex].prev = newIndex; // Link current to new
        } else {
            // Inserting at the end
            if (tail != SIZE_MAX) {
                nodes[tail].next = newIndex; // Link the last node to new
                nodes[newIndex].prev = tail; // New node points back to last
            } else {
                head = newIndex; // If list was empty, head also becomes the new node
            }
            tail = newIndex; // Update tail to the new node
        }
    
        return Iterator(this, newIndex); // Return an iterator to the new node
    }

    T& front() {
        return nodes[head].data;
    }

    T& back() {
        return nodes[tail].data;
    }

    void pop_front() {
        if (head == SIZE_MAX) return;  // No nodes to pop
    
        remove(head);
    }

    void pop_back() {
        if (tail == SIZE_MAX) { 
            return;  // No nodes to pop
        }
    
        remove(tail);
    }

    bool empty() const {
        return head == SIZE_MAX && tail == SIZE_MAX;
    }

    void clear() {
        head = SIZE_MAX;
        tail = SIZE_MAX;
        freeHead = SIZE_MAX;
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
