#ifndef MAXHEAP_H
#define MAXHEAP_H

#include <iostream>
#include <vector>
using namespace std;

/*
    MaxHeap<T>
    ----------
    A binary max-heap stored in a contiguous array.

    Heap property:
        Every parent is >= its children (using operator< on T).
        So the largest element is always at index 0.

    Index math (0-based):
        parent(i) = (i - 1) / 2
        left(i)   = 2*i + 1
        right(i)  = 2*i + 2

    Type requirement:
        T must support operator< and have a default constructor.

    Time complexity:
        push      : O(log n)
        pop       : O(log n)
        top       : O(1)
        buildHeap : O(n)

    Space:
        O(n) total, contiguous memory, no per-node overhead.

    Empty-access policy:
        top() and pop() on an empty heap print an error message to
        cerr and return a default-constructed T. The caller is
        expected to check empty() first.
*/
template <class T>
class MaxHeap {
private:
    vector<T> data;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i)   { return 2 * i + 1; }
    int right(int i)  { return 2 * i + 2; }

    void siftUp(int i) {
        while (i > 0) {
            int p = parent(i);
            if (data[p] < data[i]) {
                T tmp = data[p];
                data[p] = data[i];
                data[i] = tmp;
                i = p;
            } else {
                break;
            }
        }
    }

    void siftDown(int i) {
        int n = data.size();
        while (true) {
            int l = left(i);
            int r = right(i);
            int largest = i;

            if (l < n && data[largest] < data[l]) largest = l;
            if (r < n && data[largest] < data[r]) largest = r;

            if (largest == i) break;

            T tmp = data[i];
            data[i] = data[largest];
            data[largest] = tmp;
            i = largest;
        }
    }

    // Floyd's bottom-up build: heapify in O(n).
    void buildHeap() {
        int n = data.size();
        if (n < 2) return;
        for (int i = n / 2 - 1; i >= 0; i--) {
            siftDown(i);
        }
    }

public:
    MaxHeap() {}

    // Bulk-load constructor: takes an unsorted array, becomes a heap in O(n).
    MaxHeap(const vector<T>& items) {
        data = items;
        buildHeap();
    }

    bool empty() const { return data.empty(); }
    int  size()  const { return (int)data.size(); }

    // Returns the largest element. If the heap is empty, prints an error
    // and returns a default-constructed T. Caller should check empty() first.
    T top() const {
        if (data.empty()) {
            cerr << "[MaxHeap] top() called on empty heap" << endl;
            return T();
        }
        return data[0];
    }

    void push(const T& value) {
        data.push_back(value);
        siftUp((int)data.size() - 1);
    }

    // Removes and returns the largest element. If the heap is empty,
    // prints an error and returns a default-constructed T.
    T pop() {
        if (data.empty()) {
            cerr << "[MaxHeap] pop() called on empty heap" << endl;
            return T();
        }
        T result = data[0];

        // Move the last element to the root, shrink, then restore the heap.
        data[0] = data.back();
        data.pop_back();
        if (!data.empty()) {
            siftDown(0);
        }
        return result;
    }

    void clear() { data.clear(); }

    // Read-only view of the underlying array (used by tests).
    const vector<T>& raw() const { return data; }

    // Mutable view of the underlying array.
    // CAUTION: only use this for changes that preserve heap order
    // (for example: adding the same number to every element's priority).
    // If a change breaks the heap property, call rebuild() afterwards.
    vector<T>& rawMutable() { return data; }

    // Restore the heap property in O(n) after external code has modified
    // elements via rawMutable() in a way that may have broken ordering.
    // Internally this is Floyd's bottom-up heapify.
    void rebuild() { buildHeap(); }
};

#endif
