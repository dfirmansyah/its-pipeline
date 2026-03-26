#ifndef IT_UTIL_H
#define IT_UTIL_H

#include <memory>
#include <type_traits>

/**
 * @brief Base case (Native types & Classes)
 */
template <typename T, typename Enable = void>
struct Cloner {
    static T execute(const T& input) {
        return input; 
    }
};

/**
 * @brief Specialization Cloner for shared_ptr
 */
template <typename T>
struct Cloner<std::shared_ptr<T>> {
    static std::shared_ptr<T> execute(const std::shared_ptr<T>& input) {
        return input ? std::make_shared<T>(*input) : nullptr;
    }
};

/**
 * @brief Specialization Cloner for unique_ptr
 */
template <typename T>
struct Cloner<std::unique_ptr<T>> {
    static std::unique_ptr<T> execute(const std::unique_ptr<T>& input) {
        return input ? std::unique_ptr<T>(new T(*input)) : nullptr;
    }
};

/**
 * @brief Specialization Cloner for raw pointer, but any array pointer will only clone the first element.
 */
template <typename T>
struct Cloner<T*> {
    static T* execute(const T* input) {
        std::cout << "Cloning via raw pointer allocation.\n";
        // If the pointer is null, return null. 
        // Otherwise, create a new T using the copy constructor of the pointed-to object.
        return input ? new T(*input) : nullptr;
    }
};

/**
 * @brief Clone function entry point
 *        TODO: Raw pointer type is not handled
 */
template <typename T>
auto clone(const T& input) -> decltype(Cloner<T>::execute(input)) {
    return Cloner<T>::execute(input);
}

#endif