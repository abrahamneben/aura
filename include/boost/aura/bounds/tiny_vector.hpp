#pragma once

#include <boost/aura/config.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/less_equal.hpp>

#include <array>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace boost
{
namespace aura
{


/// tiny_vector, has a max size but also an actual size,
/// wrapps an std::array and a size.
template <typename T, std::size_t max_size_>
class tiny_vector
{

        // Make sure we have constructors for the specified size
        BOOST_MPL_ASSERT((boost::mpl::less_equal<boost::mpl::int_<max_size_>,
                boost::mpl::int_<AURA_TINY_VECTOR_MAX_SIZE>>));

public:
        typedef typename std::array<T, max_size_>::value_type value_type;
        typedef typename std::array<T, max_size_>::iterator iterator;
        typedef typename std::array<T, max_size_>::const_iterator
                const_iterator;

        /// Empty tiny vector
        inline tiny_vector()
                : size_(0)
        {
        }

        /// Create tiny vector with initializer list.
        explicit tiny_vector(std::initializer_list<T> l)
                : size_(0)
        {
                for (auto it = l.begin(); it != l.end(); it++)
                {
                        push_back(*it);
                }
        }

        /// create a new bundle of existing bundle, adding another element
        explicit tiny_vector(
                const tiny_vector<T, max_size_>& other, const T& another)
                : size_(other.size_)
                , data_(other.data_)
        {
                push_back(another);
        }


        /// Create a new tiny_vector of std::vector
        template <typename U>
        inline explicit tiny_vector(const std::vector<U>& other)
                : size_(0)
        {
                assert(other.size() <= max_size_);
                for (const U& a : other)
                {
                        push_back(static_cast<T>(a));
                }
        }

        /// Copy ctor and assignment provided by compiler

        /// Operator [], no bounds check
        T& operator[](const int& offset)
        {
                return data_[offset];
        }

        /// Operator [], no bounds check
        const T& operator[](const int& offset) const
        {
                return data_[offset];
        }

        /// Add element to vector and increment size, no bounds check
        void push_back(const T& e)
        {
                data_[size_] = e;
                size_++;
        }

        /// Return last element from vector and decrement size, no bounds check
        T pop_back()
        {
                size_--;
                return data_[size_];
        }

        // Begin
        iterator begin()
        {
                return data_.begin();
        }
        const_iterator begin() const
        {
                return data_.begin();
        }

        // End
        iterator end()
        {
                return data_.begin() + size_;
        }
        const_iterator end() const
        {
                return data_.begin() + size_;
        }

        /// return the size of the vector
        inline const std::size_t size() const
        {
                return size_;
        }

        /// Return the capacity of the tiny vector
        inline const std::size_t capacity() const
        {
                return max_size_;
        }

        /// Clear the tiny vector
        inline void clear()
        {
                size_ = 0;
        }

        /// Print contents of vector
        inline void debug__()
        {
                for (std::size_t i = 0; i < size_; i++)
                {
                        std::cout << i << " " << data_[i] << std::endl;
                }
        }

        /// Get data
        const std::array<T, max_size_>& array() const
        {
                return data_;
        }

        /// Cast to size operator
        operator T()
        {
                return product(*this);
        }

        /// Equal to
        bool operator==(const tiny_vector<T, AURA_TINY_VECTOR_MAX_SIZE>& b)
        {
                return size_ == b.size_ &&
                        std::equal(data_.begin(), data_.begin() + size_,
                                b.data_.begin());
        }

        /// Not equal to
        bool operator!=(const tiny_vector<T, AURA_TINY_VECTOR_MAX_SIZE>& b)
        {
                return !(*this == b);
        }

private:
        /// number of Ts in object
        std::size_t size_;

        /// array containing data
        std::array<T, max_size_> data_;
};

} // namespace aura
} // namespace boost