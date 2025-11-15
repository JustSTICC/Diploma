#pragma once
#include <cstdint>

static_assert(sizeof(Handle<class Foo>) == sizeof(uint64_t));

template<typename ObjectType>
class Handle final {
public:
    Handle() = default;
    explicit Handle(void* ptr) :
        index_(reinterpret_cast<ptrdiff_t>(ptr) & 0xffffffff), gen_((reinterpret_cast<ptrdiff_t>(ptr) >> 32) & 0xffffffff) {
    }

    bool empty() const {
        return gen_ == 0;
    }
    bool valid() const {
        return gen_ != 0;
    }
    uint32_t index() const {
        return index_;
    }
    uint32_t gen() const {
        return gen_;
    }
    void* indexAsVoid() const {
        return reinterpret_cast<void*>(static_cast<ptrdiff_t>(index_));
    }
    void* handleAsVoid() const {
        static_assert(sizeof(void*) >= sizeof(uint64_t));
        return reinterpret_cast<void*>((static_cast<ptrdiff_t>(gen_) << 32) + static_cast<ptrdiff_t>(index_));
    }
    bool operator==(const Handle<ObjectType>& other) const {
        return index_ == other.index_ && gen_ == other.gen_;
    }
    bool operator!=(const Handle<ObjectType>& other) const {
        return index_ != other.index_ || gen_ != other.gen_;
    }
    // allow conditions 'if (handle)'
    explicit operator bool() const {
        return gen_ != 0;
    }

private:
    Handle(uint32_t index, uint32_t gen) : index_(index), gen_(gen) {};

    template<typename ObjectType_, typename ImplObjectType>
    friend class Pool;

    uint32_t index_ = 0;
    uint32_t gen_ = 0;
};
