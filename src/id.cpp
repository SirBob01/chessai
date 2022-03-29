#include "id.h"

uint64_t IDGen::get_id() {
    if (_unused.size()) {
        uint64_t id = _unused.back();
        _unused.pop_back();
        if (_unused.size() == _unused.capacity() / 4) {
            _unused.shrink_to_fit();
        }
        return id;
    }
    return _id_counter++;
}

void IDGen::unregister_id(uint64_t id) { _unused.push_back(id); }