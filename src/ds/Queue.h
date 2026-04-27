#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
using namespace std;

/*
    Queue<T>
    --------
    A first-in / first-out queue built from a singly linked list.
    We allocate every node ourselves with `new` and free it with `delete`.

    Why a linked list?
        - enqueue and dequeue must both be O(1) worst case.
        - Keeping a tail pointer lets us append at the end in O(1)
          without ever resizing or shifting elements.

    Layout:
        head -> [v1] -> [v2] -> ... -> [vn] <- tail
        dequeue removes from head, enqueue appends after tail.

    Time complexity:
        enqueue : O(1)
        dequeue : O(1)
        front   : O(1)
        back    : O(1)
        size    : O(1)   (we keep a counter)

    Space:
        O(n) total, with one pointer of overhead per node.

    Empty-access policy:
        front(), back() and dequeue() on an empty queue print an
        error message to cerr and return a default-constructed T.
        The caller is expected to check empty() first.

    The class follows the Rule of Three:
        destructor, copy constructor, copy assignment.
    Without these, copying a Queue would shallow-copy the head/tail
    pointers and the second destructor would double-delete the nodes.
*/
template <class T>
class Queue {
private:
    struct Node {
        T     value;
        Node* next;

        Node(T v) {
            value = v;
            next = NULL;
        }
    };

    Node* head;
    Node* tail;
    int   count;

    void copyFrom(const Queue& other) {
        Node* current = other.head;
        while (current != NULL) {
            enqueue(current->value);
            current = current->next;
        }
    }

public:
    Queue() {
        head = NULL;
        tail = NULL;
        count = 0;
    }

    Queue(const Queue& other) {
        head = NULL;
        tail = NULL;
        count = 0;
        copyFrom(other);
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ~Queue() {
        clear();
    }

    bool empty() const { return count == 0; }
    int  size()  const { return count; }

    void enqueue(T value) {
        Node* node = new Node(value);
        if (tail == NULL) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        count++;
    }

    T dequeue() {
        if (head == NULL) {
            cerr << "[Queue] dequeue() called on empty queue" << endl;
            return T();
        }
        Node* old = head;
        T result = old->value;

        head = old->next;
        if (head == NULL) {
            tail = NULL;
        }
        delete old;
        count--;
        return result;
    }

    T front() const {
        if (head == NULL) {
            cerr << "[Queue] front() called on empty queue" << endl;
            return T();
        }
        return head->value;
    }

    T back() const {
        if (tail == NULL) {
            cerr << "[Queue] back() called on empty queue" << endl;
            return T();
        }
        return tail->value;
    }

    void clear() {
        Node* current = head;
        while (current != NULL) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head = NULL;
        tail = NULL;
        count = 0;
    }
};

#endif
