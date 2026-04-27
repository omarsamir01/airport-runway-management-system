@echo off
REM Builds and runs the test suite, then builds the simulator binary.
REM Uses MinGW g++ (already installed on this machine).

setlocal
set CXX=g++
set CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic -O2 -Isrc -Itests

set TEST_OUT=build\run_tests.exe
set SIM_OUT=build\runway_sim.exe
set BENCH_OUT=build\runway_bench.exe

if not exist build mkdir build

REM ---------------------------------------------------------------
REM 1. Test binary
REM ---------------------------------------------------------------
%CXX% %CXXFLAGS% ^
    tests\test_main.cpp ^
    tests\test_framework.cpp ^
    tests\test_maxheap.cpp ^
    tests\test_queue.cpp ^
    tests\test_flight.cpp ^
    tests\test_runway.cpp ^
    tests\test_scheduler.cpp ^
    src\model\Flight.cpp ^
    src\model\Runway.cpp ^
    src\Scheduler.cpp ^
    -o %TEST_OUT%

if errorlevel 1 (
    echo.
    echo Test build FAILED.
    exit /b 1
)

echo.
echo Test build OK. Running tests...
echo.
%TEST_OUT%
if errorlevel 1 (
    echo.
    echo Tests FAILED.
    exit /b 1
)

REM ---------------------------------------------------------------
REM 2. Simulator binary
REM ---------------------------------------------------------------
%CXX% %CXXFLAGS% ^
    src\main.cpp ^
    src\Scheduler.cpp ^
    src\model\Flight.cpp ^
    src\model\Runway.cpp ^
    -o %SIM_OUT%

if errorlevel 1 (
    echo.
    echo Simulator build FAILED.
    exit /b 1
)

REM ---------------------------------------------------------------
REM 3. Benchmark binary
REM ---------------------------------------------------------------
%CXX% %CXXFLAGS% ^
    src\bench.cpp ^
    -o %BENCH_OUT%

if errorlevel 1 (
    echo.
    echo Benchmark build FAILED.
    exit /b 1
)

echo.
echo ============================================
echo  Simulator built: %SIM_OUT%
echo  Benchmark built: %BENCH_OUT%
echo.
echo  Run simulator: %SIM_OUT%
echo  Run benchmark: %BENCH_OUT%
echo ============================================
