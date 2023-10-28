#pragma once

struct UERandomGenerator {
    using result_type = uint32_t;

    static constexpr result_type min() {
        return 0;  // Minimum possible random value
    }

    static constexpr result_type max() {
        return UINT32_MAX;  // Maximum possible random value
    }

    uint32_t seed;
    uint32_t value;

    UERandomGenerator(uint32_t seed, uint32_t value) {
        this->seed = seed;
        this->value = value;
    }

    result_type operator()() {
        const uint64_t a = 1103515245;
        const uint64_t c = 12345;

        seed = (uint32_t)((a * seed + c) % 0x7fffffff);
        value = ((uint64_t)seed * max()) >> 31;

        return value;
    }
};
