#include <cstddef>
#include <stdexcept>
#include <new>
#include <utility>
#include <expected>
#include <concepts>

namespace customvector {
    using std::size_t;
    using std::destructible;
    using std::copy_constructible;
    using std::expected;
    using std::unexpected;
    using std::out_of_range;
    enum class VectorError {
        IndexOutOfBounds,
        Empty
    };

    template <typename Element>
        requires destructible<Element>

    class vector {
    public:
        vector(): data_(nullptr),size_(0),capacity_(1){
            data_ = static_cast<Element*>(::operator new(capacity_*sizeof(Element)));
        }

        vector(const vector& other): data_(nullptr),size_(0),capacity_(other.capacity_){
            data_ = static_cast<Element*>(::operator new(capacity_*sizeof(Element)));
            try{
                for(size_t i=0; i<other.size_; i++){
                    new (data_+i) Element(other.data_[i]);
                    ++size_;
                }
            }catch(...){
                destroy_elements(0,size_);
                ::operator delete(data_);
                throw;
            }
        }

        vector(vector&& other) noexcept : data_(other.data_),size_(other.size_), capacity_(other.capacity_){
            other.data_ = nullptr;
            other.size_= 0;
            other.capacity_ = 0;
        }

        vector& operator=(const vector& other){
            if(this == &other){
                return *this;
            }
            vector temp(other);
            swap(temp);
            return *this;
        }

        vector& operator=(vector&& other) noexcept{
            if(this == &other){
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

        ~vector(){
            clear();
            ::operator delete(data_);
        }

        void push_back(Element element) {
            if(capacity_ == 0){
                capacity_ = 1;
                data_ = static_cast<Element*>(::operator new(capacity_*sizeof(Element)));
            }
            if(size_ == capacity_){
                grow();
            }
            new (data_ + size_) Element(std::move(element));
            ++size_;
        }
        [[nodiscard]] constexpr const Element& at(size_t index) const {
            if(index >= size_){
                throw out_of_range("customvector::vector::at-index out of bounds");
            }
            return data_[index];
        }

        [[nodiscard]] constexpr expected<Element, VectorError> get_checked(size_t index) const requires copy_constructible<Element>{
            if(index >= size_){
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

        [[nodiscard]] constexpr bool empty() const noexcept {
            return size_ == 0;
        }

        void shrinkToFit() {
            if(capacity_ == size_){
                return;
            }
            size_t newCap = size_;
            Element* newData = nullptr;
            if(newCap != 0){
                newData = static_cast<Element*>(::operator new(newCap*sizeof(Element)));
                size_t constructed = 0;
                try{
                    for(; constructed<size_; ++constructed){
                        new (newData+constructed) Element(std::move_if_noexcept(data_[constructed]));
                    }
                }catch(...){
                    for(size_t i=constructed; i>0; --i){
                        newData[i-1].~Element();
                    }
                    ::operator delete(newData);
                    throw;
                }
            }
            destroy_elements(0,size_);
            ::operator delete(data_);
            data_=newData;
            capacity_ = newCap;
        }

        void pop_back() {
            if(size_==0){
                return;
            }
            data_[size_-1].~Element();
            --size_;
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
      void grow(){
        size_t newCap = (capacity_ == 0) ? 1 : capacity_*3;
        Element* newData = static_cast<Element*>(::operator new(newCap*sizeof(Element)));
        size_t constructed = 0;
        try{
            for(; constructed<size_; ++constructed){
                new (newData+constructed) Element(std::move_if_noexcept(data_[constructed]));
            }
        }catch(...){
            for(size_t i=constructed; i>0; --i){
                newData[i-1].~Element();
            }
            ::operator delete(newData);
            throw;
        }
        destroy_elements(0,size_);
        ::operator delete(data_);
        data_ = newData;
        capacity_ = newCap;
      }

      void clear(){
        destroy_elements(0,size_);
        size_=0;
      }

      void destroy_elements(size_t from, size_t to){
        for(size_t i=to; i>from; --i){
            data_[i-1].~Element();
        }
      }

      void swap(vector& other) noexcept{
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
      }

      Element* data_;
      size_t size_;
      size_t capacity_;
    };
}
