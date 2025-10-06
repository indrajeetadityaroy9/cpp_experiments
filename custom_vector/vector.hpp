#include <concepts>
#include <cstddef>
#include <cstring>
#include <expected>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace customvector {
    using std::copy_constructible;
    using std::destructible;
    using std::expected;
    using std::out_of_range;
    using std::size_t;
    using std::unexpected;

    enum class VectorError {
        IndexOutOfBounds,
        Empty
    };

    template <typename Element>
        requires destructible<Element>
    class vector {
    public:
        using value_type = Element;
        static constexpr size_t kInitialCapacity = 8;
        static constexpr bool is_trivially_copyable = std::is_trivially_copyable_v<Element>;

        vector()
            : data_(allocate(kInitialCapacity)), size_(0), capacity_(kInitialCapacity) {}

        vector(const vector& other)
            : data_(allocate(other.capacity_)), size_(0), capacity_(other.capacity_) {
            try {
                for (size_t i = 0; i < other.size_; ++i) {
                    new (data_ + i) Element(other.data_[i]);
                    ++size_;
                }
            } catch (...) {
                destroy_elements(0, size_);
                ::operator delete(data_);
                throw;
            }
        }

        vector(vector&& other) noexcept
            : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        vector& operator=(const vector& other) {
            if (this == &other) {
                return *this;
            }
            vector temp(other);
            swap(temp);
            return *this;
        }

        vector& operator=(vector&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            clear();
            ::operator delete(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            return *this;
        }

        ~vector() {
            clear();
            ::operator delete(data_);
        }

        void push_back(const Element& element) {
            emplace_back(element);
        }

        void push_back(Element&& element) {
            emplace_back(std::move(element));
        }

        template <typename... Args>
        void emplace_back(Args&&... args) {
            ensure_capacity_for_append();
            new (data_ + size_) Element(std::forward<Args>(args)...);
            ++size_;
        }

        expected<void, VectorError> insert(size_t index, const Element& element) {
            return emplace_impl(index, element);
        }

        expected<void, VectorError> insert(size_t index, Element&& element) {
            return emplace_impl(index, std::move(element));
        }

        template <typename... Args>
        expected<void, VectorError> emplace(size_t index, Args&&... args) {
            return emplace_impl(index, std::forward<Args>(args)...);
        }

        [[nodiscard]] constexpr const Element& at(size_t index) const {
            if (index >= size_) {
                throw out_of_range("customvector::vector::at-index out of bounds");
            }
            return data_[index];
        }

        [[nodiscard]] constexpr expected<Element, VectorError>
        get_checked(size_t index) const requires copy_constructible<Element> {
            if (index >= size_) {
                return unexpected(VectorError::IndexOutOfBounds);
            }
            return data_[index];
        }

        [[nodiscard]] constexpr size_t getSize() const noexcept {
            return size_;
        }

        [[nodiscard]] constexpr size_t getCapacity() const noexcept {
            return capacity_;
        }

        [[nodiscard]] constexpr size_t size() const noexcept {
            return size_;
        }

        [[nodiscard]] constexpr size_t capacity() const noexcept {
            return capacity_;
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return size_ == 0;
        }

        void reserve(size_t newCapacity) {
            if (newCapacity <= capacity_) {
                return;
            }
            reallocate(newCapacity);
        }

        void shrinkToFit() {
            if (capacity_ == size_) {
                return;
            }
            if (size_ == 0) {
                destroy_elements(0, size_);
                ::operator delete(data_);
                data_ = nullptr;
                capacity_ = 0;
                return;
            }
            reallocate(size_);
        }

        expected<void, VectorError> pop_back() {
            if (size_ == 0) {
                return unexpected(VectorError::Empty);
            }
            data_[size_ - 1].~Element();
            --size_;
            return expected<void, VectorError>{};
        }

        [[nodiscard]] constexpr Element* begin() noexcept {
            return data_;
        }

        [[nodiscard]] constexpr const Element* begin() const noexcept {
            return data_;
        }

        [[nodiscard]] constexpr Element* end() noexcept {
            return data_ + size_;
        }

        [[nodiscard]] constexpr const Element* end() const noexcept {
            return data_ + size_;
        }

        [[nodiscard]] constexpr const Element* cbegin() const noexcept {
            return data_;
        }

        [[nodiscard]] constexpr const Element* cend() const noexcept {
            return data_ + size_;
        }

    private:
        template <typename... Args>
        expected<void, VectorError> emplace_impl(size_t index, Args&&... args) {
            if (index > size_) {
                return unexpected(VectorError::IndexOutOfBounds);
            }

            ensure_capacity_for_append();

            if (index == size_) {
                new (data_ + size_) Element(std::forward<Args>(args)...);
                ++size_;
                return expected<void, VectorError>{};
            }

            if constexpr (is_trivially_copyable) {
                const size_t tailCount = size_ - index;
                if (tailCount > 0) {
                    std::memmove(static_cast<void*>(data_ + index + 1),
                                 static_cast<void*>(data_ + index),
                                 tailCount * sizeof(Element));
                }
                data_[index] = Element(std::forward<Args>(args)...);
            } else {
                Element temp(std::forward<Args>(args)...);

                new (data_ + size_) Element(std::move_if_noexcept(data_[size_ - 1]));
                for (size_t i = size_ - 1; i > index; --i) {
                    data_[i] = std::move_if_noexcept(data_[i - 1]);
                }
                data_[index] = std::move(temp);
            }
            ++size_;
            return expected<void, VectorError>{};
        }

        void ensure_capacity_for_append() {
            if (size_ == capacity_) {
                const size_t nextCapacity = capacity_ == 0 ? kInitialCapacity : capacity_ * 2;
                reallocate(nextCapacity);
            }
        }

        void reallocate(size_t newCap) {
            Element* newData = newCap > 0 ? allocate(newCap) : nullptr;
            size_t constructed = 0;
            try {
                if constexpr (is_trivially_copyable) {
                    if (size_ > 0) {
                        std::memcpy(newData, data_, size_ * sizeof(Element));
                    }
                    constructed = size_;
                } else {
                    for (; constructed < size_; ++constructed) {
                        new (newData + constructed) Element(std::move_if_noexcept(data_[constructed]));
                    }
                }
            } catch (...) {
                destroy_range(newData, constructed);
                ::operator delete(newData);
                throw;
            }
            destroy_elements(0, size_);
            ::operator delete(data_);
            data_ = newData;
            capacity_ = newCap;
        }

        static Element* allocate(size_t count) {
            if (count == 0) {
                return nullptr;
            }
            return static_cast<Element*>(::operator new(count * sizeof(Element)));
        }

        static void destroy_range(Element* data, size_t count) noexcept {
            for (size_t i = count; i > 0; --i) {
                data[i - 1].~Element();
            }
        }

        void clear() noexcept {
            destroy_elements(0, size_);
            size_ = 0;
        }

        void destroy_elements(size_t from, size_t to) noexcept {
            for (size_t i = to; i > from; --i) {
                data_[i - 1].~Element();
            }
        }

        void swap(vector& other) noexcept {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }

        Element* data_;
        size_t size_;
        size_t capacity_;
    };
}
