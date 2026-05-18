# os-sim

A virtual memory simulator written in C++17. Built to understand how operating systems manage physical memory — from frame allocation and address translation to page replacement and copy-on-write.

This was a personal project to solidify what I learned in my OS course.

---

## What it simulates

**Virtual address translation**
Each process has its own page table mapping virtual page numbers to physical frame indices. The `MemoryManager` handles address translation, detects page faults, and loads pages into physical frames.

**Physical frame management**
A `FramePool` manages a fixed set of physical frames. It tracks which frames are in use, maintains reference counts (for CoW sharing), and zeroes frames on deallocation.

**Page replacement policies**
When all frames are occupied and a new page must be loaded, a replacement policy picks the victim frame:

- **FIFO** — evicts the frame that has been in memory the longest
- **LRU** — evicts the frame that was least recently accessed, tracked with a doubly-linked list and hash map for O(1) updates
- **CLOCK** — approximates LRU using a reference bit per frame and a circular hand; recently accessed frames get one second chance before eviction, with no pointer manipulation on every access
- **OPT** — evicts the frame whose next use is farthest in the future (Belady's algorithm); requires the full access sequence upfront, so it serves as a theoretical lower bound on faults

**Copy-on-Write (CoW)**
`fork_process(parent, child)` creates a child process that shares the parent's physical frames rather than copying them. All shared pages are marked read-only. On the first write by either process, the writing process gets its own private copy of that frame. Once only one process owns a frame, it becomes writable again with no further overhead.

Reference counts on frames track how many processes share each frame. A frame is only freed when its reference count reaches zero.

**Translation Lookaside Buffer (TLB)**
Each process has a 16-entry fully-associative TLB that caches recent virtual-to-physical translations. On every access the TLB is checked first; only on a miss does the simulator fall through to the page table. The TLB uses LRU eviction among its entries. When a page is evicted from physical memory all owning processes have their TLB entry for that page invalidated (TLB shootdown). CoW writes that relocate a page to a new frame update the TLB immediately so subsequent accesses hit without a page-table walk. The simulator tracks TLB hits, misses, and hit rate alongside the existing page-fault and eviction metrics.

---

## What I learned

- Why page tables exist and how virtual-to-physical translation works at the hardware/OS boundary
- The trade-offs between FIFO, LRU, and OPT — LRU is practical but still gets more faults than OPT; FIFO is simpler but suffers from Belady's anomaly (more frames can cause more faults)
- How LRU can be implemented in O(1) using a linked list + hash map rather than scanning all frames on every access
- How fork() in Unix systems avoids copying the entire address space by deferring copies until a write actually happens — and how reference counting makes this safe
- How metrics like copies-avoided show that CoW is most efficient under read-heavy workloads after a fork, and most costly when both processes immediately write to the same pages
- How the TLB acts as a cache in front of the page table, and why TLB coherence requires shootdowns when frames are evicted or CoW remaps a page to a new frame
- How TLB hit rate is driven entirely by access locality, not by whether pages are in memory — a tight 8-page working set gets 93.8% hit rate while a 32-page sequential scan (larger than the 16-entry TLB) gets 0%, the same thrashing dynamic that affects page replacement but one level up
- How TLB hit rate has a sharp cliff at the working-set boundary rather than a gradual curve — with a 20-page working set, TLB sizes of 4, 8, and 16 all get 0% (thrashing), while size 32 jumps to 90% because the full working set fits and the first-pass cold misses are amortized over all subsequent rounds
- How evictions and TLB hit rate are tightly coupled through shootdowns — with enough frames to hold the full working set there are zero evictions and a 90% hit rate, but reducing frames below the working-set size causes constant evictions that shoot down TLB entries faster than they can be reused, collapsing hit rate to 0%
- Why CLOCK is what real OS kernels use instead of LRU — exact LRU requires updating a recency structure on every single memory access, while CLOCK only sets a bit on access and does the recency work at eviction time; the approximation quality is close enough in practice that the per-access overhead saving dominates

---

## Project structure

```
src/
  frame_pool.h/cpp         physical frame allocator with ref counting
  page_table.h/cpp         per-process virtual-to-physical mapping
  tlb.h/cpp                per-process TLB: 16-entry fully-associative, LRU eviction
  memory_manager.h/cpp     core simulator: fault handling, CoW, TLB, metrics
  process.h                process abstraction (holds a page table and TLB)
  metrics.h/cpp            tracks faults, hits, evictions, CoW events, TLB hits/misses
  constants.h              PAGE_SIZE, NUM_FRAMES, TLB_SIZE, address space size
  policy/
    replacement_policy.h   abstract base class
    fifo.h/cpp
    lru.h/cpp
    clock.h/cpp            second-chance approximation of LRU
    opt.h/cpp

tests/
  test_frame_pool.cpp
  test_page_table.cpp
  test_tlb.cpp             12 unit tests: lookup, insert, LRU eviction, invalidate, flush
  test_memory_manager.cpp  14 core tests + 5 TLB integration tests
  test_fifo.cpp
  test_lru.cpp
  test_clock.cpp           6 tests: second chance, access protection, fault count
  test_opt.cpp
  test_cow.cpp             14 tests covering fork, CoW reads, CoW writes,
                           last-owner write restoration, multi-page forks

experiments/
  exp_utils.h                       shared table formatting
  exp_a_algorithm_comparison.cpp    FIFO vs LRU vs OPT on the same sequence
  exp_b_frame_sensitivity.cpp       how fault count changes as frame count grows
  exp_c_cow_read_heavy.cpp          fork + reads only — zero copies needed
  exp_d_cow_write_heavy.cpp         fork + writes to all pages — full copy cost
  exp_e_write_timing.cpp            pre-fork vs post-fork write cost comparison
  exp_f_tlb_locality.cpp            TLB hit rate vs access pattern locality
  exp_g_tlb_size_sweep.cpp          TLB hit rate vs TLB size (working-set cliff)
  exp_h_tlb_shootdown_cost.cpp      how evictions suppress TLB hit rate
```

---

## Future work

**Multi-level page tables**
The current page table is a flat hash map over the full virtual address space. Real systems (x86-64 uses 4 levels) break the virtual address into chunks, each indexing into a tree of smaller tables. Most of the tree is never allocated — only the paths to pages actually in use. Worth implementing to understand how the OS avoids storing entries for the entire address space while keeping lookup depth bounded.

**Alternative page table structures**
Beyond hierarchical tables, there are two other designs worth understanding:

- *Hashed page tables* — the VPN is hashed into a bucket; collisions are chained. Common in systems with large, sparse address spaces. Lookup is O(1) average but degrades under hash collisions.
- *Inverted page tables* — instead of one table per process indexed by VPN, there is one global table indexed by physical frame number, storing which process and VPN owns each frame. Scales with physical memory rather than virtual address space size, but makes forward lookup (VPN → frame) expensive without a hash index on top.

**Working set model**
Rather than evicting based on recency alone, the working set model tracks which pages each process has accessed within a sliding time window and only keeps those in memory. Pages that fall out of the window are candidates for eviction even if frames are available. This models the principle that processes have phases of execution with distinct locality, and that keeping cold pages in memory to avoid future faults is not always worth the cost.

**Custom memory allocator**
The logical next step from this project. The frame pool here manages fixed-size physical frames — a real allocator has to handle variable-size requests, fragmentation, and alignment. Two allocator designs that build directly on the ideas here:

- *Slab allocator* — pre-allocates slabs of fixed-size objects for common allocation sizes (used by the Linux kernel for kernel objects). Eliminates fragmentation for known sizes and makes alloc/free O(1).
- *Buddy allocator* — splits memory into power-of-two blocks and merges adjacent free blocks back together on free. Balances fragmentation and coalescing cost, and is what the Linux kernel uses for page-level allocation under the hood.

---

## Results

**Page replacement — algorithm comparison** (`exp_a`, sequence `[1,2,3,4,1,2,5,1,2,3,4,5]`, 3 frames)

```
Algorithm       Faults    Hits   Evictions
----------------------------------------------
FIFO                 9       3           6
LRU                 10       2           7
CLOCK                9       3           6
OPT                  8       4           5
```

CLOCK matches FIFO on this sequence and sits one fault above OPT, the theoretical minimum. LRU underperforms here due to Belady's anomaly on this specific access pattern. OPT serves as the lower bound — unachievable in practice since it requires knowing the future.

**TLB size vs hit rate** (`exp_g`, 20-page working set, 200 accesses)

```
  TLB Size    TLB Hits  TLB Misses    Hit Rate
----------------------------------------------
         4           0         200        0.0%
         8           0         200        0.0%
        16           0         200        0.0%
        32         180          20       90.0%
        64         180          20       90.0%
```

Hit rate is binary around the working-set boundary: TLB sizes below 20 thrash to 0%, size 32 jumps to 90% because the full working set fits and cold misses are only paid once. Doubling from 32 to 64 yields no further gain.

---

## Build

Requires CMake 3.15+ and a C++17 compiler.

```bash
cmake -S . -B build
cmake --build build
```

## Run tests

```bash
./build/test_frame_pool
./build/test_page_table
./build/test_tlb
./build/test_fifo
./build/test_lru
./build/test_clock
./build/test_opt
./build/test_memory_manager
./build/test_cow
```

## Run experiments

```bash
./build/exp_a    # algorithm comparison
./build/exp_b    # frame count sensitivity
./build/exp_c    # CoW read-heavy
./build/exp_d    # CoW write-heavy
./build/exp_e    # write timing
./build/exp_f    # TLB hit rate vs access locality
./build/exp_g    # TLB hit rate vs TLB size
./build/exp_h    # TLB shootdown impact on hit rate
```