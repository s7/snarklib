#ifndef _SNARKLIB_AUX_STL_HPP_
#define _SNARKLIB_AUX_STL_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <istream>
#include <ostream>
#include <queue>
#include <vector>
#include "IndexSpace.hpp"

namespace snarklib {

////////////////////////////////////////////////////////////////////////////////
// Ordered pair of key and value
// Used only by the multiExp() max-heap.
//

template <typename KEY, typename VALUE>
struct OrdPair
{
    KEY key;
    VALUE value;

    OrdPair(const KEY& a, const VALUE& b)
        : key(a), value(b)
    {}

    bool operator< (const OrdPair& other) const {
        return key < other.key;
    }
};

////////////////////////////////////////////////////////////////////////////////
// STL priority queue (heap) with reservable memory
// Derives from the STL priority queue to access the vector container
// inside and reserve memory. This is bad engineering practice but
// expedient.
//

template <typename T>
class PriorityQueue : public std::priority_queue<T>
{
public:
    PriorityQueue() = default;

    PriorityQueue(const std::size_t capacity) {
        reserve(capacity);
    }

    void reserve(const std::size_t capacity) {
        this->c.reserve(capacity);
    }

    std::size_t capacity() const {
        return this->c.capacity();
    }
};

////////////////////////////////////////////////////////////////////////////////
// Sparse vector (of paired group knowledge commitments)
// Used for zero knowledge proving key A, B, and C queries.
//

template <typename T>
class SparseVector
{
public:
    SparseVector() = default;

    SparseVector(const std::size_t n)
        : m_index(n),
          m_value(n)
    {}

    SparseVector(const std::size_t n, const T& obj)
        : m_index(n),
          m_value(n, obj)
    {}

    void clear() {
        m_index.clear();
        m_value.clear();
    }

    bool empty() const {
        return m_value.empty();
    }

    void reserve(const std::size_t capacity) {
        m_index.reserve(capacity);
        m_value.reserve(capacity);
    }

    std::size_t size() const {
        return m_value.size();
    }

    void resize(const std::size_t n) {
        m_index.resize(n);
        m_value.resize(n);
    }

    void pushBack(const std::size_t elementIndex, const T& elementValue) {
        m_index.push_back(elementIndex);
        m_value.emplace_back(elementValue);
    }

    void setIndexElement(const std::size_t index,
                         const std::size_t elementIndex,
                         const T& elementValue) {
        m_index[index] = elementIndex;
        m_value[index] = elementValue;
    }

    void setElement(const std::size_t idx, const T& elementValue) {
        m_value[idx] = elementValue;
    }

    void setIndex(const std::size_t idx, const std::size_t elementIndex) {
        m_index[idx] = elementIndex;
    }

    const T& getElement(const std::size_t idx) const { return m_value[idx]; }
    std::size_t getIndex(const std::size_t idx) const { return m_index[idx]; }

    const T& getElementForIndex(const std::size_t elementIndex) const {
        const auto it = std::lower_bound(m_index.begin(),
                                         m_index.end(),
                                         elementIndex);

        if (it != m_index.end() && *it == elementIndex) {
            return m_value[it - m_index.begin()];

        } else {
            static const T dummy; // neutral zero element
            return dummy;
        }
    }

    bool operator== (const SparseVector<T>& other) const {
        if (size() != other.size()) {
            return false;
        }

        for (std::size_t i = 0; i < size(); ++i) {
            if ((getIndex(i) != other.getIndex(i)) ||
                (getElement(i) != other.getElement(i)))
                return false;
        }

        return true;
    }

    bool operator!= (const SparseVector<T>& other) const {
        return ! (*this == other);
    }

    // useful for map-reduce, concatenate sparse vectors from batchExp()
    void concat(const SparseVector<T>& other) {
        const std::size_t N = other.size();

        for (std::size_t i = 0; i < N; ++i) {
            m_index.emplace_back(other.m_index[i]);
            m_value.emplace_back(other.m_value[i]);
        }
    }

    void marshal_out(std::ostream& os) const {
        // size
        os << size() << std::endl;

        // index vector
        for (const auto& a : m_index) {
            os << a << std::endl;
        }

        // value vector
        for (const auto& a : m_value) {
            a.marshal_out(os);
            os << std::endl;
        }
    }

    bool marshal_in(std::istream& is) {
        // size
        std::size_t numberElems;
        is >> numberElems;
        if (!is) return false;

        // index vector
        m_index.resize(numberElems);
        for (std::size_t i = 0; i < numberElems; ++i) {
            if (!(is >> m_index[i])) return false;
        }

        // value vector
        m_value.resize(numberElems);
        for (std::size_t i = 0; i < numberElems; ++i) {
            if (! m_value[i].marshal_in(is)) return false;
        }

        return true; // ok
    }

private:
    std::vector<std::size_t> m_index;
    std::vector<T> m_value;
};

////////////////////////////////////////////////////////////////////////////////
// Vector subsection corresponding to map-reduce block partitioning
// Originates from mapping of constraint system through QAP ABCH.
//

template <typename T>
class BlockVector
{
public:
    // one-dimensional index space
    static IndexSpace<1> space(const std::vector<T>& a) {
        return IndexSpace<1>(a.size());
    }

    BlockVector()
        : m_block{0},
          m_startIndex(0),
          m_stopIndex(0)
    {}

    // zero block partition
    BlockVector(const IndexSpace<1>& space,
                const std::array<std::size_t, 1>& block)
        : m_space(space),
          m_block(block),
          m_startIndex(space.indexOffset(m_block)[0]),
          m_stopIndex(m_startIndex + space.indexSize(m_block)[0]),
          m_value(m_stopIndex - m_startIndex) // initializes to zero
    {}

    // zero block partition
    BlockVector(const IndexSpace<1>& space,
                const std::size_t block)
        : BlockVector{space, std::array<std::size_t, 1>{block}}
    {}

    // standard vector -> block partition
    BlockVector(const IndexSpace<1>& space,
                const std::array<std::size_t, 1>& block,
                const std::vector<T>& a)
        : m_space(space),
          m_block(block),
          m_startIndex(space.indexOffset(m_block)[0]),
          m_stopIndex(m_startIndex + space.indexSize(m_block)[0]),
          m_value(a.begin() + m_startIndex, a.begin() + m_stopIndex)
    {}

    // standard vector -> block partition
    BlockVector(const IndexSpace<1>& space,
                const std::size_t block,
                const std::vector<T>& a)
        : BlockVector{space, std::array<std::size_t, 1>{block}, a}
    {}

    const IndexSpace<1>& space() const { return m_space; }
    const std::array<std::size_t, 1>& block() const { return m_block; }

    std::size_t globalSize() const { return m_space.globalID()[0]; }
    std::size_t size() const { return m_stopIndex - m_startIndex; }

    std::size_t startIndex() const { return m_startIndex; }
    std::size_t stopIndex() const { return m_stopIndex; }

    const T& operator[] (const std::size_t index) const {
        return m_value[index - m_startIndex];
    }

    T& operator[] (const std::size_t index) {
        return m_value[index - m_startIndex];
    }

    const std::vector<T>& vec() const { return m_value; }
    std::vector<T>& lvec() { return m_value; }

    // in-place accumulation
    BlockVector& operator+= (const BlockVector& other) {
        if (m_space == other.m_space) {
            for (std::size_t i = 0; i < size(); ++i)
                m_value[i] = m_value[i] + other.m_value[i];
        }

        return *this;
    }

    // block partition -> standard vector
    void emplace(std::vector<T>& a) const {
        for (std::size_t i = m_startIndex; i < m_stopIndex; ++i)
            a[i] = m_value[i - m_startIndex];
    }

    void marshal_out(std::ostream& os) const {
        // index space
        m_space.marshal_out(os);

        // block
        for (const auto& a : m_block)
            os << a << std::endl;

        // value
        for (const auto& a : m_value)
            os << a << std::endl;
    }

    bool marshal_in(std::istream& is) {
        // index space
        if (! m_space.marshal_in(is)) return false;

        // block
        if (!(is >> m_block[0])) return false;

        // start and stop
        m_startIndex = m_space.indexOffset(m_block)[0];
        m_stopIndex = m_startIndex + m_space.indexSize(m_block)[0];

        // value
        const std::size_t len = m_stopIndex - m_startIndex;
        m_value.resize(len);
        for (std::size_t i = 0; i < len; ++i) {
            if (!(is >> m_value[i])) return false;
        }

        return true; // ok
    }

private:
    IndexSpace<1> m_space;
    std::array<std::size_t, 1> m_block;
    std::size_t m_startIndex, m_stopIndex;
    std::vector<T> m_value;
};

} // namespace snarklib

#endif
