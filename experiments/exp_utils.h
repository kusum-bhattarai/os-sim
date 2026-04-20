#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include "metrics.h"

static constexpr int TABLE_WIDTH = 46;

inline void print_separator(char c = '=') {
    std::cout << std::string(TABLE_WIDTH, c) << "\n";
}

inline void print_exp_header(const std::string& title, const std::string& subtitle = "") {
    print_separator();
    std::cout << title << "\n";
    if (!subtitle.empty()) std::cout << subtitle << "\n";
    print_separator();
}

inline void print_exp_footer() {
    print_separator();
    std::cout << "\n";
}

// --- Algorithm comparison table (exp_a) ---
inline void print_algo_table_header() {
    std::cout << std::left  << std::setw(12) << "Algorithm"
              << std::right << std::setw(10) << "Faults"
                            << std::setw(8)  << "Hits"
                            << std::setw(12) << "Evictions" << "\n";
    print_separator('-');
}

inline void print_algo_row(const std::string& name, const Metrics& m) {
    std::cout << std::left  << std::setw(12) << name
              << std::right << std::setw(10) << m.page_faults
                            << std::setw(8)  << m.hits
                            << std::setw(12) << m.evictions << "\n";
}

// --- Frame sensitivity table (exp_b) ---
inline void print_frame_table_header() {
    std::cout << std::right << std::setw(8)  << "Frames"
                            << std::setw(10) << "Faults"
                            << std::setw(8)  << "Hits"
                            << std::setw(12) << "Evictions" << "\n";
    print_separator('-');
}

inline void print_frame_row(int frames, const Metrics& m) {
    std::cout << std::right << std::setw(8)  << frames
                            << std::setw(10) << m.page_faults
                            << std::setw(8)  << m.hits
                            << std::setw(12) << m.evictions << "\n";
}

// --- CoW metrics block (exp_c, exp_d) ---
inline void print_cow_metrics(const Metrics& m) {
    auto row = [](const std::string& label, int val) {
        std::cout << std::left  << std::setw(26) << label
                  << std::right << std::setw(8)  << val << "\n";
    };
    row("Page Faults",     m.page_faults);
    row("Hits",            m.hits);
    row("Evictions",       m.evictions);
    print_separator('-');
    row("CoW Forks",       m.cow_forks);
    row("CoW Copies",      m.cow_copies);
    row("Copies Avoided",  m.cow_copies_avoided);
}

// --- Write timing phase table (exp_e) ---
inline void print_phase_table_header() {
    std::cout << std::left  << std::setw(22) << "Phase"
              << std::right << std::setw(8)  << "Faults"
                            << std::setw(8)  << "Hits"
                            << std::setw(8)  << "Copies" << "\n";
    print_separator('-');
}

inline void print_phase_row(const std::string& phase, const Metrics& m) {
    std::cout << std::left  << std::setw(22) << phase
              << std::right << std::setw(8)  << m.page_faults
                            << std::setw(8)  << m.hits
                            << std::setw(8)  << m.cow_copies << "\n";
}
