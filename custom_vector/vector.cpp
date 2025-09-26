#include <cstddef>
#include <stdexcept>
#include <new>
#include <utility>

namespace customvector {
    template <typename Element>
    class vector {
    public:
        vector(): data_(nullptr),size_(0),capacity_(1){
            data_ = static_cast<Element*>(::operator new(capacity_*sizeof(Element)));
        }
        vector(const vector& other): data_(nullptr),size_(0),capacity_(other.capacity_){
            data_ = static_cast<Element*>(::operator new(capacity_*sizeof(Element)));
            try{
                for(std::size_t i=0; i<other.size_; i++){
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
        const Element& at(std::size_t index) const {
            if(index >= size_){
                throw std::out_of_range("customvector::vector::at-index out of bounds");
            }
            return data_[index];
        }
        std::size_t getSize() const {
            return size_;
        }
        std::size_t getCapacity() const {
            return capacity_;
        }
        void shrinkToFit() {
            if(capacity_ == size_){
                return;
            }
            std::size_t newCap = size_;
            Element* newData = nullptr;
            if(newCap != 0){
                newData = static_cast<Element*>(::operator new(newCap*sizeof(Element)));
                std::size_t constructed = 0;
                try{
                    for(; constructed<size_; ++constructed){
                        new (newData+constructed) Element(std::move_if_noexcept(data_[constructed]));
                    }
                }catch(...){
                    for(std::size_t i=constructed; i>0; --i){
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
    private:
      void grow(){
        std::size_t newCap = (capacity_ == 0) ? 1 : capacity_*3;
        Element* newData = static_cast<Element*>(::operator new(newCap*sizeof(Element)));
        std::size_t constructed = 0;
        try{
            for(; constructed<size_; ++constructed){
                new (newData+constructed) Element(std::move_if_noexcept(data_[constructed]));
            }
        }catch(...){
            for(std::size_t i=constructed; i>0; --i){
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
      void destroy_elements(std::size_t from, std::size_t to){
        for(std::size_t i=to; i>from; --i){
            data_[i-1].~Element();
        }
      }
      void swap(vector& other) noexcept{
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
      }
      Element* data_;
      std::size_t size_;
      std::size_t capacity_;
    };
}

#ifdef CUSTOMVECTOR_ENABLE_DEMO
#include <iostream>
#include <string>

int main() {
    // Test with integers
    std::cout << "Testing custom vector with integers:\n";
    customvector::vector<int> intVec;
    
    // Push some elements
    for (int i = 1; i <= 5; ++i) {
        intVec.push_back(i * 10);
    }
    
    std::cout << "Size: " << intVec.getSize() << ", Capacity: " << intVec.getCapacity() << "\n";
    std::cout << "Elements: ";
    for (std::size_t i = 0; i < intVec.getSize(); ++i) {
        std::cout << intVec.at(i) << " ";
    }
    std::cout << "\n";
    
    // Test pop_back
    intVec.pop_back();
    std::cout << "After pop_back - Size: " << intVec.getSize() << "\n";
    std::cout << "Elements: ";
    for (std::size_t i = 0; i < intVec.getSize(); ++i) {
        std::cout << intVec.at(i) << " ";
    }
    std::cout << "\n\n";
    
    // Test with strings
    std::cout << "Testing custom vector with strings:\n";
    customvector::vector<std::string> stringVec;
    
    stringVec.push_back("Hello");
    stringVec.push_back("World");
    stringVec.push_back("Custom");
    stringVec.push_back("Vector");
    
    std::cout << "Size: " << stringVec.getSize() << ", Capacity: " << stringVec.getCapacity() << "\n";
    std::cout << "Elements: ";
    for (std::size_t i = 0; i < stringVec.getSize(); ++i) {
        std::cout << "\"" << stringVec.at(i) << "\" ";
    }
    std::cout << "\n";
    
    // Test copy constructor
    customvector::vector<std::string> stringVecCopy = stringVec;
    std::cout << "Copy - Size: " << stringVecCopy.getSize() << "\n";
    std::cout << "Copy elements: ";
    for (std::size_t i = 0; i < stringVecCopy.getSize(); ++i) {
        std::cout << "\"" << stringVecCopy.at(i) << "\" ";
    }
    std::cout << "\n";
    
    // Test shrink to fit
    stringVec.shrinkToFit();
    std::cout << "After shrinkToFit - Capacity: " << stringVec.getCapacity() << "\n";
    
    return 0;
}
#endif // CUSTOMVECTOR_ENABLE_DEMO
