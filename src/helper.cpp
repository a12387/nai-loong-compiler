#include "helper.hpp"

void addItemToSlice(koopa_raw_slice_t &slice, void *item) {
    auto newbuf = new const void *[slice.len + 1];
    if(slice.buffer != nullptr) {
        memcpy(newbuf, slice.buffer, sizeof(const void *) * slice.len);
        delete[] slice.buffer;
    }
        
    newbuf[slice.len++] = item;
    slice.buffer = newbuf;
}