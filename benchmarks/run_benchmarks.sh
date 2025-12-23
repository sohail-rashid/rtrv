#!/bin/bash

# Search Engine Benchmark Runner
# Convenient wrapper for running Google Benchmark tests

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BENCHMARK_DIR}/../build/benchmarks"
FILTER=""
FORMAT="console"
OUTPUT_FILE=""
MIN_TIME=""
REPETITIONS=""
LIST_TESTS=false
ALL_BENCHMARKS=false
SPECIFIC_BENCHMARK=""

# Usage information
usage() {
    cat << EOF
${GREEN}Search Engine Benchmark Runner${NC}

Usage: $0 [OPTIONS] [BENCHMARK_NAME]

${YELLOW}Options:${NC}
  -h, --help              Show this help message
  -l, --list              List all available benchmarks
  -a, --all               Run all benchmarks
  -f, --filter PATTERN    Run only benchmarks matching PATTERN (regex)
  -o, --output FILE       Output results to FILE
  -j, --json              Output in JSON format
  -c, --csv               Output in CSV format
  -t, --time SECONDS      Minimum time per benchmark (default: auto)
  -r, --repetitions N     Number of repetitions for statistics
  --aggregate-only        Show only aggregate statistics

${YELLOW}Available Benchmarks:${NC}
  search          Search performance tests
  topk            Top-K heap optimization tests
  indexing        Document indexing performance
  concurrent      Concurrency and thread safety
  memory          Memory usage analysis
  tokenizer       Tokenization (SIMD) performance

${YELLOW}Examples:${NC}
  # Run all benchmarks
  $0 --all

  # Run specific benchmark suite
  $0 topk

  # Run with filter
  $0 search --filter "BM_Search.*10000"

  # Output to JSON
  $0 --all --json --output results.json

  # Run with repetitions for statistics
  $0 search --repetitions 10 --aggregate-only

  # Compare before/after changes
  $0 topk --output before.json --json
  # (make code changes)
  $0 topk --output after.json --json
  python3 compare_benchmarks.py before.json after.json

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -l|--list)
            LIST_TESTS=true
            shift
            ;;
        -a|--all)
            ALL_BENCHMARKS=true
            shift
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        -j|--json)
            FORMAT="json"
            shift
            ;;
        -c|--csv)
            FORMAT="csv"
            shift
            ;;
        -t|--time)
            MIN_TIME="$2"
            shift 2
            ;;
        -r|--repetitions)
            REPETITIONS="$2"
            shift 2
            ;;
        --aggregate-only)
            AGGREGATE_ONLY="true"
            shift
            ;;
        *)
            if [[ -z "$SPECIFIC_BENCHMARK" ]]; then
                SPECIFIC_BENCHMARK="$1"
            else
                echo -e "${RED}Error: Unknown option or multiple benchmarks specified: $1${NC}"
                usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    echo -e "${RED}Error: Build directory not found: $BUILD_DIR${NC}"
    echo "Please run: mkdir build && cd build && cmake .. && make"
    exit 1
fi

# Function to run a single benchmark
run_benchmark() {
    local bench_name=$1
    local bench_path="${BUILD_DIR}/${bench_name}"
    
    if [[ ! -f "$bench_path" ]]; then
        echo -e "${YELLOW}Warning: Benchmark not found: ${bench_name}${NC}"
        return 1
    fi
    
    echo -e "${BLUE}Running ${bench_name}...${NC}"
    
    # Build command line arguments
    local args=()
    
    if [[ -n "$FILTER" ]]; then
        args+=("--benchmark_filter=$FILTER")
    fi
    
    if [[ -n "$MIN_TIME" ]]; then
        # Add 's' suffix if not present
        if [[ ! "$MIN_TIME" =~ [smh]$ ]]; then
            MIN_TIME="${MIN_TIME}s"
        fi
        args+=("--benchmark_min_time=$MIN_TIME")
    fi
    
    if [[ -n "$REPETITIONS" ]]; then
        args+=("--benchmark_repetitions=$REPETITIONS")
    fi
    
    if [[ "$AGGREGATE_ONLY" == "true" ]]; then
        args+=("--benchmark_report_aggregates_only=true")
    fi
    
    if [[ "$LIST_TESTS" == "true" ]]; then
        args+=("--benchmark_list_tests=true")
    fi
    
    if [[ -n "$OUTPUT_FILE" && "$FORMAT" != "console" ]]; then
        args+=("--benchmark_out=$OUTPUT_FILE")
        args+=("--benchmark_out_format=$FORMAT")
    fi
    
    # Run the benchmark
    "$bench_path" "${args[@]}"
    
    return $?
}

# Determine which benchmarks to run
BENCHMARKS=()

if [[ "$ALL_BENCHMARKS" == "true" ]]; then
    BENCHMARKS=("search_benchmark" "topk_benchmark" "indexing_benchmark" "concurrent_benchmark" "memory_benchmark" "tokenizer_simd_benchmark")
elif [[ -n "$SPECIFIC_BENCHMARK" ]]; then
    case "$SPECIFIC_BENCHMARK" in
        search)
            BENCHMARKS=("search_benchmark")
            ;;
        topk)
            BENCHMARKS=("topk_benchmark")
            ;;
        indexing)
            BENCHMARKS=("indexing_benchmark")
            ;;
        concurrent)
            BENCHMARKS=("concurrent_benchmark")
            ;;
        memory)
            BENCHMARKS=("memory_benchmark")
            ;;
        tokenizer)
            BENCHMARKS=("tokenizer_simd_benchmark")
            ;;
        *)
            echo -e "${RED}Error: Unknown benchmark: $SPECIFIC_BENCHMARK${NC}"
            echo "Use --list to see available benchmarks"
            exit 1
            ;;
    esac
else
    echo -e "${YELLOW}No benchmark specified. Use --all or specify a benchmark name.${NC}"
    usage
    exit 1
fi

# Run benchmarks
echo -e "${GREEN}=== Search Engine Benchmarks ===${NC}"
echo ""

SUCCESS_COUNT=0
FAIL_COUNT=0

for bench in "${BENCHMARKS[@]}"; do
    if run_benchmark "$bench"; then
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        echo ""
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo -e "${RED}Failed to run $bench${NC}"
        echo ""
    fi
done

# Summary
echo -e "${GREEN}=== Summary ===${NC}"
echo -e "Benchmarks run: ${SUCCESS_COUNT}/${#BENCHMARKS[@]}"
if [[ $FAIL_COUNT -gt 0 ]]; then
    echo -e "${RED}Failed: $FAIL_COUNT${NC}"
fi

if [[ -n "$OUTPUT_FILE" && "$FORMAT" != "console" ]]; then
    echo -e "${GREEN}Results saved to: $OUTPUT_FILE${NC}"
fi

exit 0
