#pragma once
#include <cstddef>

constexpr size_t PAGE_SIZE = 1024;
constexpr size_t NUM_FRAMES = 100;
constexpr size_t VIRTUAL_ADDRESS_SPACE_SIZE = 1UL << 20; // 1 MiB
constexpr size_t NUM_VIRTUAL_PAGES = VIRTUAL_ADDRESS_SPACE_SIZE / PAGE_SIZE;