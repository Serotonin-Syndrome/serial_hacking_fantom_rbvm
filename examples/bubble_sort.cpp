/*
    Bubble sort.

    This example shows arrays, functions and templates work correctly.
*/


#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <iterator>


template <class RandomAccessIterator,
          class Comparator = std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>>
void bubble_sort(RandomAccessIterator first, 
                 RandomAccessIterator last, Comparator cmp = Comparator()) {
    for (auto i = first; i != last; ++i)
        for (auto j = i + 1; j != last; ++j) 
            if (cmp(*j, *i))
                std::swap(*i, *j);
}


int main() {
#ifdef JUDGE
    size_t n = 7;
    int *array = (int *) malloc(n * sizeof(int));
    {
        int *p = array;
        *p++ = 1;
        *p++ = 7;
        *p++ = 2;
        *p++ = 6;
        *p++ = 4;
        *p++ = 5;
        *p++ = 3;
    }
#else
    size_t n = 0;
    scanf("%zu", &n);
    int* array = (int*)malloc(n * sizeof(int));
    for (size_t i = 0; i < n; ++i)
        scanf("%d", &array[i]);
#endif

    bubble_sort(array, array + n);

    for (size_t i = 0; i < n; ++i)
        printf("%d \n", array[i]);

    free(array);
}
