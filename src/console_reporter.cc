// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "benchmark/reporter.h"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "check.h"
#include "colorprint.h"
#include "string_util.h"
#include "walltime.h"

namespace benchmark {

bool ConsoleReporter::ReportContext(const Context& context) {
  name_field_width_ = context.name_field_width;

  std::cerr << "Run on (" << context.num_cpus << " X " << context.mhz_per_cpu
            << " MHz CPU " << ((context.num_cpus > 1) ? "s" : "") << ")\n";

  std::cerr << LocalDateTimeString() << "\n";

  if (context.cpu_scaling_enabled) {
    std::cerr << "***WARNING*** CPU scaling is enabled, the benchmark "
                 "real time measurements may be noisy and will incur extra "
                 "overhead.\n";
  }

#ifndef NDEBUG
  std::cerr << "***WARNING*** Library was built as DEBUG. Timings may be "
               "affected.\n";
#endif

  int output_width;
  if (context.manual_time_used) {
      output_width = fprintf(stdout, "%-*s %13s %13s %13s %10s\n",
                             static_cast<int>(name_field_width_), "Benchmark",
                             "Real time", "Manual time", "CPU", "Iterations");
      manual_time_used_ = true;
  } else {
      output_width = fprintf(stdout, "%-*s %13s %13s %10s\n",
                             static_cast<int>(name_field_width_), "Benchmark",
                             "Time", "CPU", "Iterations");
      manual_time_used_ = false;
  }
  std::cout << std::string(output_width - 1, '-') << "\n";

  return true;
}

void ConsoleReporter::ReportRuns(const std::vector<Run>& reports) {
  if (reports.empty()) {
    return;
  }

  for (Run const& run : reports) {
    CHECK_EQ(reports[0].benchmark_name, run.benchmark_name);
    PrintRunData(run);
  }

  if (reports.size() < 2) {
    // We don't report aggregated data if there was a single run.
    return;
  }

  Run mean_data;
  Run stddev_data;
  BenchmarkReporter::ComputeStats(reports, &mean_data, &stddev_data);

  // Output using PrintRun.
  PrintRunData(mean_data);
  PrintRunData(stddev_data);
}

void ConsoleReporter::PrintRunData(const Run& result) {
  // Format bytes per second
  std::string rate;
  if (result.bytes_per_second > 0) {
    rate = StrCat(" ", HumanReadableNumber(result.bytes_per_second), "B/s");
  }

  std::string manual_rate;
  if (result.bytes_per_manual_second > 0) {
    manual_rate = StrCat(" ", HumanReadableNumber(result.bytes_per_manual_second), "B/s (manual)");

    if (!result.both_manual_and_real_time)
        rate = std::string();
  }

  // Format items per second
  std::string items;
  if (result.items_per_second > 0) {
    items = StrCat(" ", HumanReadableNumber(result.items_per_second),
                   " items/s");
  }

  std::string manual_items;
  if (result.items_per_manual_second > 0) {
    manual_items = StrCat(" ", HumanReadableNumber(result.items_per_manual_second),
                          " items/s (manual)");

    if (!result.both_manual_and_real_time)
        items = std::string();
  }

  double multiplier;
  const char* timeLabel;
  std::tie(timeLabel, multiplier) = GetTimeUnitAndMultiplier(result.time_unit);

  ColorPrintf(COLOR_GREEN, "%-*s ",
              name_field_width_, result.benchmark_name.c_str());

  double iters = (result.iterations == 0)
      ? 1
      : static_cast<double>(result.iterations);

  // If any of the benchmarks uses manual time, make sure to print
  // three time columns for all lines so the times match up.
  if (manual_time_used_) {
    ColorPrintf(COLOR_YELLOW, "%10.0f %s %10.0f %s %10.0f %s ",
                (result.real_accumulated_time * multiplier) /
                    iters,
                timeLabel,
                (result.manual_accumulated_time * multiplier) /
                    iters,
                timeLabel,
                (result.cpu_accumulated_time * multiplier) /
                    iters,
                timeLabel);
  } else {
    ColorPrintf(COLOR_YELLOW, "%10.0f %s %10.0f %s ",
                (result.real_accumulated_time * multiplier) /
                    iters,
                timeLabel,
                (result.cpu_accumulated_time * multiplier) /
                    iters,
                timeLabel);
  }

  ColorPrintf(COLOR_CYAN, "%10lld", result.iterations);

  if (!rate.empty()) {
    ColorPrintf(COLOR_DEFAULT, " %*s", 13, rate.c_str());
  }

  if (!items.empty()) {
    ColorPrintf(COLOR_DEFAULT, " %*s", 18, items.c_str());
  }

  if (!manual_rate.empty()) {
    ColorPrintf(COLOR_DEFAULT, " %*s", 13, manual_rate.c_str());
  }

  if (!manual_items.empty()) {
    ColorPrintf(COLOR_DEFAULT, " %*s", 18, manual_items.c_str());
  }

  if (!result.report_label.empty()) {
    ColorPrintf(COLOR_DEFAULT, " %s", result.report_label.c_str());
  }

  ColorPrintf(COLOR_DEFAULT, "\n");
}

}  // end namespace benchmark
