#include "test_framework.h"
#include "ds/MaxHeap.h"

#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>
using namespace std;

TEST_CASE("MaxHeap: empty heap reports size 0 and empty == true") {
    MaxHeap<int> h;
    ASSERT_TRUE(h.empty());
    ASSERT_EQ(h.size(), 0);
}

TEST_CASE("MaxHeap: single element top and pop") {
    MaxHeap<int> h;
    h.push(42);
    ASSERT_EQ(h.size(), 1);
    ASSERT_EQ(h.top(), 42);
    ASSERT_EQ(h.pop(), 42);
    ASSERT_TRUE(h.empty());
}

TEST_CASE("MaxHeap: push three elements, root is the largest") {
    MaxHeap<int> h;
    h.push(5);
    h.push(20);
    h.push(10);
    ASSERT_EQ(h.top(), 20);
    ASSERT_EQ(h.size(), 3);
}

TEST_CASE("MaxHeap: pop returns elements in non-increasing order") {
    MaxHeap<int> h;
    int input[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    int n = sizeof(input) / sizeof(int);
    for (int i = 0; i < n; i++) h.push(input[i]);

    vector<int> out;
    while (!h.empty()) out.push_back(h.pop());

    vector<int> expected(input, input + n);
    sort(expected.begin(), expected.end(), greater<int>());
    ASSERT_TRUE(out == expected);
}

TEST_CASE("MaxHeap: stress 1000 random ints come out sorted descending") {
    srand(20260427);

    vector<int> values;
    MaxHeap<int> h;
    for (int i = 0; i < 1000; i++) {
        int v = (rand() % 20001) - 10000;
        values.push_back(v);
        h.push(v);
    }
    ASSERT_EQ(h.size(), 1000);

    vector<int> out;
    while (!h.empty()) out.push_back(h.pop());

    sort(values.begin(), values.end(), greater<int>());
    ASSERT_TRUE(out == values);
}

TEST_CASE("MaxHeap: bulk-load constructor heapifies in O(n)") {
    vector<int> raw;
    raw.push_back(1); raw.push_back(8); raw.push_back(3);
    raw.push_back(7); raw.push_back(2); raw.push_back(9);
    raw.push_back(4); raw.push_back(6); raw.push_back(5);

    MaxHeap<int> h(raw);
    ASSERT_EQ(h.size(), (int)raw.size());

    vector<int> out;
    while (!h.empty()) out.push_back(h.pop());

    vector<int> expected = raw;
    sort(expected.begin(), expected.end(), greater<int>());
    ASSERT_TRUE(out == expected);
}

TEST_CASE("MaxHeap: heap property holds at every internal node") {
    vector<int> raw;
    raw.push_back(15); raw.push_back(3);  raw.push_back(9);
    raw.push_back(1);  raw.push_back(12); raw.push_back(7);
    raw.push_back(4);  raw.push_back(8);  raw.push_back(11);
    raw.push_back(2);  raw.push_back(6);  raw.push_back(10);

    MaxHeap<int> h(raw);
    const vector<int>& a = h.raw();
    for (int i = 0; i < (int)a.size(); i++) {
        int l = 2 * i + 1;
        int r = 2 * i + 2;
        if (l < (int)a.size()) ASSERT_FALSE(a[i] < a[l]);
        if (r < (int)a.size()) ASSERT_FALSE(a[i] < a[r]);
    }
}

TEST_CASE("MaxHeap: clear() resets the heap") {
    MaxHeap<int> h;
    for (int i = 0; i < 50; i++) h.push(i);
    h.clear();
    ASSERT_TRUE(h.empty());
    ASSERT_EQ(h.size(), 0);
}
