#pragma once

#include <string>
#include <vector>

#include "../benchmark_functions/benchmark_function.hpp"
#include "../optimisation_algorithm/minimise.hpp"

// One benchmark function and the runs made on it. All pointers are
// non-owning: the BenchmarkFunction and RunResult objects must outlive the
// call to write_html_report.
struct ReportEntry {
    const BenchmarkFunction* function = nullptr;
    std::vector<const RunResult*> runs;  // series order = colour order (max 3)
};

struct ReportSection {
    std::string title;
    std::string description;
    std::vector<ReportEntry> entries;
};

// Writes a self-contained HTML page (inline SVG, no external dependency):
//   - convergence plot  log10(f(x_k) - f*)  vs iteration, for every entry
//   - landscape + optimisation paths, for 2-D functions
//   - a summary table of all runs
// Returns false if the file could not be opened.
bool write_html_report(const std::string& path, const std::string& title,
                       const std::vector<ReportSection>& sections);
