#ifndef IT_COMMON_H
#define IT_COMMON_H

#if __cplusplus == 201103L
#include <memory>
#include <utility>

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

#endif // IT_COMMON_H