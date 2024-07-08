#include "random.h"

unsigned int get_rand()
{
    thread_local std::mt19937 mt(std::random_device{}());
    auto dist = std::uniform_int_distribution<unsigned int>(0, UINT_MAX);
    return dist(mt);
}