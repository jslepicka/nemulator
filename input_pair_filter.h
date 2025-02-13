#pragma once
#include <vector>
class c_input_pair_filter
{
  public:
    c_input_pair_filter(std::vector<uint32_t> pairs)
    {
        this->pairs = pairs;
        prev_input = 0;
        input_mask = 0;
        for (auto p : pairs) {
            input_mask |= (p & (-p));
        }
        input_mask = ~input_mask;
    }
    uint32_t filter(uint32_t input)
    {
        uint32_t changed = input ^ prev_input;
        prev_input = input;
        uint32_t hidden = input & ~input_mask;

        for (auto p : pairs) {
            if ((changed & p) && (hidden & p)) {
                input_mask ^= p;
            }
        }
        return input & input_mask;
    }

  private:
    std::vector<uint32_t> pairs;
    uint32_t input_mask;
    uint32_t prev_input;
};