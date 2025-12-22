#!/bin/bash
# Quick tokenizer SIMD vs Scalar benchmark with table output (10 repetitions)

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/../build/benchmarks"
OUTPUT_FILE="${1:-tokenizer_quick_results.json}"

echo "========================================"
echo "  Tokenizer SIMD Benchmark (Quick)"
echo "========================================"
echo ""

# Check if benchmark executable exists
if [ ! -f "$BUILD_DIR/tokenizer_simd_benchmark" ]; then
    echo "Error: Benchmark executable not found!"
    echo "Please build the project first:"
    echo "  cd build && cmake .. && make tokenizer_simd_benchmark"
    exit 1
fi

# Run benchmarks
echo "Running benchmarks with 10 repetitions..."
echo "Output will be saved to: $BUILD_DIR/$OUTPUT_FILE"
echo ""

cd "$BUILD_DIR"
./tokenizer_simd_benchmark \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true \
    --benchmark_out="$OUTPUT_FILE" \
    --benchmark_out_format=json \
    --benchmark_counters_tabular=true \
    --benchmark_format=console

echo ""
echo "========================================"
echo "  Benchmark completed!"
echo "========================================"
echo ""

# Generate summary table
echo "Generating performance summary table..."
echo ""
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║                    SIMD vs Scalar Performance Summary                    ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Extract key results and format as table
if command -v jq &> /dev/null; then
    echo "┌──────────────────────┬─────────────┬──────────────┬───────────┬──────────────┐"
    echo "│ Benchmark            │   SIMD (ns) │  Scalar (ns) │  Speedup  │ Throughput   │"
    echo "├──────────────────────┼─────────────┼──────────────┼───────────┼──────────────┤"
    
    # Parse JSON and create table rows
    jq -r '.benchmarks[] | 
        select(.name | contains("_mean")) | 
        "\(.name)\t\(.real_time)\t\(.["bytes_per_second"])"' "$OUTPUT_FILE" | \
    awk 'BEGIN {FS="\t"; simd_short=0; scalar_short=0; simd_med=0; scalar_med=0; simd_long=0; scalar_long=0}
    /SIMD_Short_mean/ {simd_short=$2; simd_short_bps=$3}
    /Scalar_Short_mean/ {scalar_short=$2; scalar_short_bps=$3}
    /SIMD_Medium_mean/ {simd_med=$2; simd_med_bps=$3}
    /Scalar_Medium_mean/ {scalar_med=$2; scalar_med_bps=$3}
    /SIMD_Long_mean/ {simd_long=$2; simd_long_bps=$3}
    /Scalar_Long_mean/ {scalar_long=$2; scalar_long_bps=$3}
    END {
        if (simd_short && scalar_short) {
            speedup = scalar_short/simd_short;
            printf "│ Short (~50 words)    │ %10.0f  │ %11.0f  │   %.3fx   │ %8.1f MB/s │\n", simd_short, scalar_short, speedup, simd_short_bps/1048576
        }
        if (simd_med && scalar_med) {
            speedup = scalar_med/simd_med;
            printf "│ Medium (~500 words)  │ %10.0f  │ %11.0f  │   %.3fx   │ %8.1f MB/s │\n", simd_med, scalar_med, speedup, simd_med_bps/1048576
        }
        if (simd_long && scalar_long) {
            speedup = scalar_long/simd_long;
            printf "│ Long (~5000 words)   │ %10.0f  │ %11.0f  │   %.3fx   │ %8.1f MB/s │\n", simd_long, scalar_long, speedup, simd_long_bps/1048576
        }
    }'
    
    echo "└──────────────────────┴─────────────┴──────────────┴───────────┴──────────────┘"
    echo ""
    echo "Note: Speedup = Scalar Time / SIMD Time (higher is better)"
else
    echo "Install 'jq' for formatted table output: brew install jq"
fi
echo ""

# Analyze results if Python script exists
if [ -f "$SCRIPT_DIR/compare_tokenizer_benchmarks.py" ]; then
    echo "Generating detailed comparison report..."
    echo ""
    python3 "$SCRIPT_DIR/compare_tokenizer_benchmarks.py" "$BUILD_DIR/$OUTPUT_FILE"
else
    echo "Note: Python script not found for detailed analysis"
fi

echo ""
echo "Results saved to: $BUILD_DIR/$OUTPUT_FILE"
echo ""
