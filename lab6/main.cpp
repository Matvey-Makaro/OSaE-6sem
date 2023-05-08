#include "profile.h"

#include <iostream>
#include <ctime>
#include <algorithm>
#include <vector>
using namespace std;

void fill_vec(vector<int>& vec)
{
    for(size_t i = 0; i < vec.size(); i++)
        vec[i] = rand() % 20;
}

template<typename T>
void print_vec(const vector<T>& vec)
{
    for(const auto& el : vec)
        cout << el << ' ';
}

struct ThreadArgs
{
    vector<int>& vec;
    size_t start;
    size_t end;
};

void* sort_part(void* arg)
{
    auto args = static_cast<ThreadArgs*>(arg);
    auto& vec = args->vec;
    sort(begin(vec) + args->start, begin(vec) + args->end);
    return nullptr;
}

void merge_parts(vector<int>& vec, vector<ThreadArgs> args) {
    vector<int> res(vec.size());

    for(size_t i = 0; i < vec.size(); i++)
    {
        auto min = numeric_limits<int>::max();
        size_t min_j = -1;
        for(size_t j = 0; j < args.size(); j++)
        {
            if (args[j].start >= args[j].end)
                continue;

            if (min > vec[args[j].start])
            {
                min = vec[args[j].start];
                min_j = j;
            }
        }
        res[i] = min;
        args[min_j].start++;
    }
    vec.swap(res);
}

void parallel_sort(vector<int>& vec, size_t numThreads)
{
    const auto size = vec.size();
    auto part_size = vec.size() / numThreads ;
    if (size <= part_size)
    {
        ThreadArgs arg = {vec, 0, size};
        sort_part(&arg);
        return;
    }

    std::vector<pthread_t> threads(numThreads);
    std::vector<ThreadArgs> args;
    args.reserve(numThreads);

    size_t cur_begin = 0, cur_end = 0;
    for (size_t i = 0; i < numThreads - 1; ++i)
    {
        cur_begin = cur_end;
        cur_end += part_size;
        args.push_back({vec, cur_begin, cur_end});
        pthread_create(&threads[i], NULL, sort_part, &args[i]);
    }
    args.push_back({vec, cur_end, size});
    pthread_create(&threads.back(), NULL, sort_part, &args.back());

    for (size_t i = 0; i < numThreads; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    merge_parts(vec, args);
}

int main() {
    srand(time(nullptr));
    constexpr size_t ARR_SIZE = 2000000;
    constexpr int THREADS_NUM = 10;
    vector<int> fvec(ARR_SIZE);
    fill_vec(fvec);
    vector<int> svec = fvec;

    {
        LOG_DURATION("One thread sort: ");
        sort(begin(svec), end(svec));
    }

    {
        LOG_DURATION(to_string(THREADS_NUM) + " threads sort: ");
        parallel_sort(fvec, THREADS_NUM);
    }
    if(is_sorted(begin(fvec), end(fvec)))
        cout << "Vector sorted" << endl;
    else cout << "Vector not sorted!" << endl;
//    print_vec(fvec);




    return 0;
}
