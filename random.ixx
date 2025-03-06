module;

export module random;
import std.compat;

export namespace random
{
unsigned int get_rand()
{
    thread_local std::mt19937 mt(std::random_device{}());
    auto dist = std::uniform_int_distribution<unsigned int>(0, std::numeric_limits<unsigned int>::max());
    return dist(mt);
}
} //namespace random