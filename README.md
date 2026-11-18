# Basic implementation of optimisation functions

Implementation of Gradient Descent and Adam, tested on several test functions organised in an object-oriented manner.

![Report preview](docs/report_preview.png)

## Motivations

Originally developed for an Undergraduate project (L3 ISFA), and upgraded to explore more about the low-level language C++ using Claude Code.

## Installation

The project is plain C++20 with **no external library**. You only need a C++20 compiler.

### Windows (MSYS2 CLANG64, recommended, no admin rights needed)

1. Install [MSYS2](https://www.msys2.org/). The default location is `C:\msys64`. If you have no admin rights, pick a folder you own, such as `C:\Users\<you>\msys64`, and replace `C:\msys64` with it below.
2. Open the **"MSYS2 CLANG64"** terminal from the Start Menu (the prompt shows `CLANG64`) and install the toolchain:
   ```
   pacman -Syu
   pacman -S --needed mingw-w64-clang-x86_64-clang mingw-w64-clang-x86_64-lldb mingw-w64-clang-x86_64-clang-tools-extra
   ```
   `pacman -Syu` may ask you to close the terminal. If it does, reopen it and run the second command.
3. Add `C:\msys64\clang64\bin` to your **user** `Path`. In *Start → "Edit environment variables for your account"*, select `Path`, click *Edit*, then *New*. No admin rights are needed. Open a new terminal afterwards.
4. Check it:
   ```powershell
   clang++ --version
   ```

Use the **CLANG64** environment only. Packages from UCRT64/MINGW64 (`mingw-w64-ucrt-x86_64-*`, `mingw-w64-x86_64-*`), MSVC, and GCC use a different standard library and will not link with it.

### Linux / macOS, or Windows with CMake

Any of **Clang ≥ 17, GCC ≥ 13, or MSVC 2022** works, plus **CMake ≥ 3.20**. The code uses `<format>`, so older compilers will not build it. With MSYS2 you can add CMake and Ninja through pacman:
```
pacman -S --needed mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-ninja
```

### Get the code

```
git clone <url-of-this-repository>
cd <repository-folder>
```

## First run

### Build

**Windows (PowerShell, from the project root):**
```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```
This compiles `main.cpp` and every `.cpp` under `src/` into `build/benchmark.exe`. `-ExecutionPolicy Bypass` applies only to this command and lets Windows run the local script. Options:

| Flag | Effect |
|---|---|
| `-Debug` | No optimisation (`-O0`), for stepping through with LLDB |
| `-Static` | Standalone `.exe` that runs on a machine without MSYS2 |

**With CMake (any platform):**
```
cmake -S . -B build
cmake --build build --config Release
```
Add `-G Ninja` to the first command if Ninja is installed. `--config Release` only matters for Visual Studio, which can hold several build types in one folder. Other generators ignore it.

### Run

Run the program from the project root. The path depends on how you built it:

| Built with | Command |
|---|---|
| `build.ps1` (Windows) | `./build/benchmark.exe` |
| CMake + Ninja or Makefiles (Linux, macOS, MSYS2) | `./build/benchmark` (`.exe` on Windows) |
| CMake + Visual Studio (default generator on Windows) | `./build/Release/benchmark.exe` |

The program runs gradient descent and Adam on every benchmark function. It runs them in 2-D from fixed start points, then the n-dimensional functions in 10-D from a random start drawn with seed 42. Everything is deterministic, so you should see exactly this:

```
Function       n  Optimiser               lr  iters  status                f - f*
--------------------------------------------------------------------------------
Sphere         2  Gradient descent       0.1     94  converged          1.706e-17
Sphere         2  Adam                   0.1    402  converged          2.264e-17
Rosenbrock     2  Gradient descent     0.001   3000  max iterations     4.867e-02
Rosenbrock     2  Adam                  0.02   3000  max iterations     3.148e-11
Rastrigin      2  Gradient descent     0.002     17  converged          1.293e+01
Rastrigin      2  Adam                  0.05    425  converged          1.293e+01
Ackley         2  Gradient descent      0.05   3000  max iterations     9.480e+00
Ackley         2  Adam                   0.1    392  converged          7.962e+00
Booth          2  Gradient descent      0.05    204  converged          2.100e-17
Booth          2  Adam                   0.2    430  converged          1.544e-17
Beale          2  Gradient descent     0.001   3000  max iterations     9.988e-03
Beale          2  Adam                  0.05   2995  converged          1.648e-16
Himmelblau     2  Gradient descent      0.01     33  converged          1.622e-19
Himmelblau     2  Adam                   0.1    425  converged          3.604e-19
Sphere        10  Gradient descent       0.1     93  converged          1.680e-17
Sphere        10  Adam                   0.1    385  converged          2.407e-17
Rosenbrock    10  Gradient descent     0.001   3000  max iterations     5.438e-02
Rosenbrock    10  Adam                  0.02   3000  max iterations     9.022e-12
Rastrigin     10  Gradient descent     0.002     22  converged          4.378e+01
Rastrigin     10  Adam                  0.05    458  converged          4.378e+01
Ackley        10  Gradient descent      0.05     33  converged          1.640e+01
Ackley        10  Adam                   0.1    376  converged          1.640e+01

Report written to benchmark_report.html
```

This output was produced with clang 23.1 / libc++ (MSYS2 CLANG64). With another compiler or standard library, the last digits of very small values (around 1e-16) may differ, because `std::exp`, `std::cos`, etc. are not guaranteed to round identically everywhere. The iteration counts, statuses and conclusions stay the same.

### Open the report

```powershell
start benchmark_report.html     # Windows
xdg-open benchmark_report.html  # Linux
open benchmark_report.html      # macOS
```

The report is a single self-contained HTML file with no internet access needed. For each function it shows:
- the **convergence plot**: f(x<sub>k</sub>) − f* on a log scale vs iteration (hover for values);
- for 2-D functions, the **landscape** with both optimisation paths (hollow circle = start, filled circle = end, × = global minimum);
- a **summary table** of all runs.

### Troubleshooting

| Symptom | Fix |
|---|---|
| `clang++` is not recognised | `C:\msys64\clang64\bin` is not on your user `Path`, or the terminal was opened before you added it |
| `no member named 'format' in namespace 'std'` | The compiler is not in C++20 mode; build with `build.ps1` / CMake, or pass `-std=c++20` |
| The `.exe` complains about missing `libc++.dll` | Run it from a terminal where `C:\msys64\clang64\bin` is on `Path`, or rebuild with `build.ps1 -Static` |
| "running scripts is disabled on this system" | Use `powershell -ExecutionPolicy Bypass -File build.ps1` as shown above |

## Project structure

```
main.cpp                          benchmark driver: runs GD and Adam, writes the report
build.ps1                         Windows build script (clang++, no CMake needed)
CMakeLists.txt                    cross-platform build
src/optimisation_project/
  benchmark_functions/            BenchmarkFunction base class + Sphere, Rosenbrock,
                                  Rastrigin, Ackley, Booth, Beale, Himmelblau
  optimisation_algorithm/         Optimiser base class, GradientDescent, Adam,
                                  minimise() driver loop
  report/                         HTML/SVG report generator
docs/report_preview.png           screenshot used in this README
```

The code deliberately uses pointers in several ways: raw `const double*` / `double*` buffers, pointer arithmetic, owning `new[]`/`delete[]` buffers (with the rule of five) in `Adam`, `std::unique_ptr` for ownership, and non-owning base-class pointers for polymorphism.
