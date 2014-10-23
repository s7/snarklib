#ifndef _SNARKLIB_AUX_STL_HPP_
#define _SNARKLIB_AUX_STL_HPP_

#include <algorithm>
#include <cstdint>
#include <queue>
#include <vector>

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

    SparseVector(const std::size_t reserveSize)
    {
        m_index.reserve(reserveSize);
        m_value.reserve(reserveSize);
    }

    void clear() {
        m_index.clear();
        m_value.clear();
    }

    void reserve(const std::size_t capacity) {
        m_index.reserve(capacity);
        m_value.reserve(capacity);
    }

    std::size_t size() const {
        return m_value.size();
    }

    void pushBack(const std::size_t elementIndex, const T& elementValue) {
        m_index.push_back(elementIndex);
        m_value.emplace_back(elementValue);
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

private:
    std::vector<std::size_t> m_index;
    std::vector<T> m_value;
};

} // namespace snarklib

#endif