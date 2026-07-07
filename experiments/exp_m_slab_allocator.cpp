#include "exp_utils.h"
#include "alloc/slab_allocator.h"
#include "constants.h"
#include <iostream>
#include <iomanip>

static constexpr int OBJECTS = 100;

int main() {
    print_exp_header(
        "Experiment M: Slab Allocator",
        "100 small objects vs one frame per object"
    );

    std::cout << std::right << std::setw(9)  << "Obj size"
                            << std::setw(12) << "Slab frames"
                            << std::setw(13) << "Naive frames"
                            << std::setw(13) << "Slab util" << "\n";
    print_separator('-');

    for (size_t size : {16, 24, 64, 200}) {
        FramePool pool(128);
        SlabAllocator slab(pool);
        for (int i = 0; i < OBJECTS; i++)
            slab.allocate(size);

        double util = 100.0 * slab.bytes_requested()
                    / (slab.frames_used() * PAGE_SIZE);
        std::cout << std::right << std::setw(9)  << size
                                << std::setw(12) << slab.frames_used()
                                << std::setw(13) << OBJECTS
                                << std::setw(12) << std::fixed << std::setprecision(1) << util << "%\n";
    }

    std::cout << "\nA naive allocator burning one " << PAGE_SIZE << "-byte frame per object\n";
    std::cout << "wastes over 90% of every frame. Slabs pack objects of one\n";
    std::cout << "size class per frame; the only waste is class round-up\n";
    std::cout << "(24 -> 32) and the tail of the last partially-filled slab.\n";
    print_exp_footer();
    return 0;
}
