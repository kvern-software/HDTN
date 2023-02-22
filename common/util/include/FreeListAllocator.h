/**
 * @file FreeListAllocator.h
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 *
 * @section DESCRIPTION
 *
 * Idea from https://stackoverflow.com/questions/24278803/how-can-i-write-a-stateful-allocator-in-c11-given-requirements-on-copy-constr
 * An allocator designed for node-based containers like an std::set which preserves up to MaxListSize
 * "unused elements prior to deletion" in order to reduce new and delete operations.
 */

#ifndef FREE_LIST_ALLOCATOR_H
#define FREE_LIST_ALLOCATOR_H 1

 /*
 //motivation behind FreeListAllocator is as follows, the first is fastest and doesn't spend time in operator new
     { //fastest by far
         std::set<int, std::less<int>, FreeListAllocator<int> > s;
         boost::timer::auto_cpu_timer timer;
         for (unsigned int i = 0; i < 10000000; ++i) {
             s.insert(5); s.insert(10); s.insert(2); s.clear();
         }
     },
     { //slowest
         std::set<int> s;
         boost::timer::auto_cpu_timer timer;
         for (unsigned int i = 0; i < 10000000; ++i) {
             s.insert(5); s.insert(10); s.insert(2); s.clear();
         }
     }
     { //second fastest
         boost::container::set<int, std::less<int>, boost::container::adaptive_pool<int> > s;
         boost::timer::auto_cpu_timer timer;
         for (unsigned int i = 0; i < 10000000; ++i) {
             s.insert(5); s.insert(10); s.insert(2); s.clear();
         }
     }
     { //slow
         std::set<int, std::less<int>, boost::container::adaptive_pool<int> > s;
         boost::timer::auto_cpu_timer timer;
         for (unsigned int i = 0; i < 10000000; ++i) {
             s.insert(5); s.insert(10); s.insert(2); s.clear();
         }
     }
 */

#include <algorithm>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

template <typename T, std::size_t MaxListSize = 100>
class FreeListAllocator {
    union node {
        node* next;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };
    
    void clear() noexcept {
        node* p = list;
        while (p) {
            node* tmp = p;
            p = p->next;
            delete tmp;
        }
        list = nullptr;
        listSize = 0;
    }

    //This technique is useful in classes that have multiple constructors
    node* list = nullptr;
public:
    std::size_t listSize = 0;
    std::size_t listSizeCopy = 0; //for unit testing

    using value_type = T;
    using size_type = std::size_t;
    using propagate_on_container_move_assignment = std::true_type;

    //the three constructor versions must be defined (even if they do nothing) to allow for copy-constructions from allocator objects of other types.
    FreeListAllocator() noexcept = default;
    FreeListAllocator(const FreeListAllocator& other) noexcept :
        listSizeCopy(other.listSize) {} //do not copy the list to avoid double delete
    template <typename U>
    FreeListAllocator(const FreeListAllocator<U, MaxListSize>& other) noexcept : //used by get_allocator()
        listSizeCopy(other.listSize) {} //do not copy the list to avoid double delete

    FreeListAllocator(FreeListAllocator&& other) noexcept : list(other.list), listSize(other.listSize) {
        other.list = nullptr;
        other.listSize = 0;
        other.listSizeCopy = 0;
    }

    std::size_t GetCurrentListSizeFromGetAllocatorCopy() const noexcept {
        return listSizeCopy;
    }

    std::size_t GetMaxListSize() const {
        return MaxListSize;
    }

    FreeListAllocator& operator = (const FreeListAllocator& other) noexcept {
        // noop
        listSizeCopy = other.listSize; //do not copy the list to avoid double delete
        return *this;
}

    FreeListAllocator& operator = (FreeListAllocator&& other) noexcept {
        clear();
        list = other.list;
        listSize = other.listSize;
        other.list = nullptr;
        other.listSize = 0;
        return *this;
    }

    ~FreeListAllocator() noexcept { clear(); }

    T* allocate(size_type n) {
        //std::cout << "Allocate(" << n << ") from ";
        if (n == 1) {
            node* ptr = list;
            if (ptr) {
                //std::cout << "freelist\n";
                list = list->next;
                --listSize;
            }
            else {
                //std::cout << "new node\n";
                ptr = new node;
            }
            return reinterpret_cast<T*>(ptr);
        }

        //std::cout << "::operator new\n";
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* ptr, size_type n) noexcept {
        //std::cout << "Deallocate(" << static_cast<void*>(ptr) << ", " << n << ") to ";
        if ((n == 1) && (listSize < MaxListSize)) {
            //std::cout << "freelist\n";
            node* node_ptr = reinterpret_cast<node*>(ptr);
            node_ptr->next = list;
            list = node_ptr;
            ++listSize;
        }
        else {
            //std::cout << "::operator delete\n";
            ::operator delete(ptr);
        }
    }

    template<typename U>
    struct rebind {
        typedef FreeListAllocator<U, MaxListSize> other;
    };
};


template <typename T, typename U>
inline bool operator == (const FreeListAllocator<T>&, const FreeListAllocator<U>&) {
    //std::cout << "ret true\n";
    return true;
}

template <typename T, typename U>
inline bool operator != (const FreeListAllocator<T>&, const FreeListAllocator<U>&) {
    //std::cout << "ret false\n";
    return false;
}












template <typename T, std::size_t MaxInitialListSize = 100>
class FreeListAllocatorDynamic {
    union node {
        node* next;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };

    void clear() noexcept {
        node* p = list;
        while (p) {
            node* tmp = p;
            p = p->next;
            delete tmp;
        }
        list = nullptr;
        listSize = 0;
    }


    //This technique is useful in classes that have multiple constructors
    node* list = nullptr;
public:
    std::size_t listSize = 0;
    std::size_t listSizeCopy = 0; //for unit testing
    std::shared_ptr<std::size_t> maxListSizePtr = std::make_shared<std::size_t>(MaxInitialListSize);

    using value_type = T;
    using size_type = std::size_t;
    using propagate_on_container_move_assignment = std::true_type;

    
    //the three constructor versions must be defined (even if they do nothing) to allow for copy-constructions from allocator objects of other types.
    FreeListAllocatorDynamic() noexcept {} // = default; will fail compile because of shared_ptr
    FreeListAllocatorDynamic(const FreeListAllocatorDynamic& other) noexcept :
        listSizeCopy(other.listSize), //do not copy the list to avoid double delete
        maxListSizePtr(other.maxListSizePtr) {} //increase use_count of shared_ptr so SetMaxListSize will work after get_allocator() copies
    template <typename U>
    FreeListAllocatorDynamic(const FreeListAllocatorDynamic<U, MaxInitialListSize>& other) noexcept : //used by get_allocator()
        listSizeCopy(other.listSize), //do not copy the list to avoid double delete
        maxListSizePtr(other.maxListSizePtr) {} //increase use_count of shared_ptr so SetMaxListSize will work after get_allocator() copies

    FreeListAllocatorDynamic(FreeListAllocatorDynamic&& other) noexcept :
        list(other.list),
        listSize(other.listSize),
        maxListSizePtr(std::make_shared<std::size_t>(*(other.maxListSizePtr))) //copy to new shared_ptr
    {
        other.list = nullptr;
        other.listSize = 0;
        other.listSizeCopy = 0;
        //other.maxListSizePtr.reset(); //keep other in a valid state
    }

    std::size_t GetCurrentListSizeFromGetAllocatorCopy() const noexcept {
        return listSizeCopy;
    }

    std::size_t GetMaxListSize() const {
        return *maxListSizePtr;
    }

    void SetMaxListSizeFromGetAllocatorCopy(std::size_t newMaxListSize) const {
        *maxListSizePtr = newMaxListSize;
    }

    FreeListAllocatorDynamic& operator = (const FreeListAllocatorDynamic& other) noexcept {
        // noop
        listSizeCopy = other.listSize; //do not copy the list to avoid double delete
        return *this;
    }

    FreeListAllocatorDynamic& operator = (FreeListAllocatorDynamic&& other) noexcept {
        clear();
        list = other.list;
        listSize = other.listSize;
        other.list = nullptr;
        other.listSize = 0;
        return *this;
    }

    ~FreeListAllocatorDynamic() noexcept { clear(); }

    T* allocate(size_type n) {
        //std::cout << "Allocate(" << n << ") from ";
        if (n == 1) {
            node* ptr = list;
            if (ptr) {
                //std::cout << "freelist\n";
                list = list->next;
                --listSize;
            }
            else {
                //std::cout << "new node\n";
                ptr = new node;
            }
            return reinterpret_cast<T*>(ptr);
        }

        //std::cout << "::operator new\n";
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* ptr, size_type n) noexcept {
        //std::cout << "Deallocate(" << static_cast<void*>(ptr) << ", " << n << ") to ";
        if ((n == 1) && (listSize < (*maxListSizePtr))) {
            //std::cout << "freelist\n";
            node* node_ptr = reinterpret_cast<node*>(ptr);
            node_ptr->next = list;
            list = node_ptr;
            ++listSize;
        }
        else {
            //std::cout << "::operator delete\n";
            ::operator delete(ptr);
        }
    }

    template<typename U>
    struct rebind {
        typedef FreeListAllocatorDynamic<U, MaxInitialListSize> other;
    };
};


template <typename T, typename U>
inline bool operator == (const FreeListAllocatorDynamic<T>&, const FreeListAllocatorDynamic<U>&) {
    return true;
}

template <typename T, typename U>
inline bool operator != (const FreeListAllocatorDynamic<T>&, const FreeListAllocatorDynamic<U>&) {
    return false;
}

#endif //FREE_LIST_ALLOCATOR_H

