#include "test_framework.h"
#include "ds/Queue.h"

#include <string>
using namespace std;

TEST_CASE("Queue: empty queue reports size 0") {
    Queue<int> q;
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
}

TEST_CASE("Queue: single element enqueue then dequeue") {
    Queue<int> q;
    q.enqueue(7);
    ASSERT_EQ(q.size(),  1);
    ASSERT_EQ(q.front(), 7);
    ASSERT_EQ(q.back(),  7);
    ASSERT_EQ(q.dequeue(), 7);
    ASSERT_TRUE(q.empty());
}

TEST_CASE("Queue: FIFO order is preserved") {
    Queue<int> q;
    for (int i = 1; i <= 5; i++) q.enqueue(i);
    ASSERT_EQ(q.size(),  5);
    ASSERT_EQ(q.front(), 1);
    ASSERT_EQ(q.back(),  5);

    for (int i = 1; i <= 5; i++) {
        ASSERT_EQ(q.dequeue(), i);
    }
    ASSERT_TRUE(q.empty());
}

TEST_CASE("Queue: interleaved enqueue/dequeue keeps tail consistent") {
    Queue<int> q;
    q.enqueue(1);
    q.enqueue(2);
    ASSERT_EQ(q.dequeue(), 1);
    q.enqueue(3);
    ASSERT_EQ(q.front(), 2);
    ASSERT_EQ(q.back(),  3);
    ASSERT_EQ(q.dequeue(), 2);
    ASSERT_EQ(q.front(), 3);
    ASSERT_EQ(q.back(),  3);
    ASSERT_EQ(q.dequeue(), 3);
    ASSERT_TRUE(q.empty());

    // After draining, the next enqueue must work and become both head and tail.
    q.enqueue(99);
    ASSERT_EQ(q.front(), 99);
    ASSERT_EQ(q.back(),  99);
    ASSERT_EQ(q.size(),  1);
}

TEST_CASE("Queue: works with std::string") {
    Queue<string> q;
    q.enqueue("alpha");
    q.enqueue("beta");
    q.enqueue("gamma");

    ASSERT_EQ(q.dequeue(), string("alpha"));
    ASSERT_EQ(q.dequeue(), string("beta"));
    ASSERT_EQ(q.dequeue(), string("gamma"));
    ASSERT_TRUE(q.empty());
}

TEST_CASE("Queue: clear() drops every node") {
    Queue<int> q;
    for (int i = 0; i < 100; i++) q.enqueue(i);
    q.clear();
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
}

TEST_CASE("Queue: copy constructor performs a deep copy") {
    Queue<int> a;
    a.enqueue(1);
    a.enqueue(2);
    a.enqueue(3);

    Queue<int> b = a;             // calls the copy constructor
    a.dequeue();                  // mutate the original
    ASSERT_EQ(a.front(), 2);

    // b must be unaffected because its nodes are independent.
    ASSERT_EQ(b.size(),  3);
    ASSERT_EQ(b.dequeue(), 1);
    ASSERT_EQ(b.dequeue(), 2);
    ASSERT_EQ(b.dequeue(), 3);
    ASSERT_TRUE(b.empty());
}

TEST_CASE("Queue: copy assignment performs a deep copy") {
    Queue<int> a;
    a.enqueue(10);
    a.enqueue(20);

    Queue<int> b;
    b.enqueue(999);
    b = a;                        // overwrite b with a deep copy of a

    a.dequeue();
    ASSERT_EQ(b.size(),    2);
    ASSERT_EQ(b.dequeue(), 10);
    ASSERT_EQ(b.dequeue(), 20);
    ASSERT_TRUE(b.empty());
}

TEST_CASE("Queue: self-assignment is a no-op") {
    Queue<int> q;
    q.enqueue(1);
    q.enqueue(2);
    q = q;
    ASSERT_EQ(q.size(),  2);
    ASSERT_EQ(q.front(), 1);
    ASSERT_EQ(q.back(),  2);
}

TEST_CASE("Queue: stress 10k enqueue/dequeue cycles") {
    Queue<int> q;
    int N = 10000;
    for (int i = 0; i < N; i++) q.enqueue(i);
    ASSERT_EQ(q.size(), N);
    for (int i = 0; i < N; i++) {
        ASSERT_EQ(q.dequeue(), i);
    }
    ASSERT_TRUE(q.empty());
}
