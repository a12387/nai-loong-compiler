#pragma once

#include <unordered_map>

using namespace std;

class StackFrame {
private:
    int currentPos;
    unordered_map<void *, int> stackFrame;
public:
    StackFrame() {
        currentPos = 0;
        stackFrame = {};
    }
    int find(void *ptr) {
        return stackFrame[ptr];
    }
    void add(void *ptr, int size) {
        stackFrame[ptr] = currentPos;
        currentPos += size;
    }
};