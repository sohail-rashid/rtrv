#!/usr/bin/env python3
"""
Benchmark Comparison Tool
Compares two Google Benchmark JSON outputs and shows performance differences.
"""

import json
import sys
import argparse
from typing import Dict, List, Tuple
from collections import defaultdict


class Colors:
    """ANSI color codes for terminal output"""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    BOLD = '\033[1m'
    NC = '\033[0m'  # No Color


def load_benchmark_results(filepath: str) -> Dict:
    """Load benchmark results from JSON file"""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"{Colors.RED}Error: File not found: {filepath}{Colors.NC}")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"{Colors.RED}Error: Invalid JSON in file: {filepath}{Colors.NC}")
        sys.exit(1)


def extract_benchmarks(results: Dict) -> Dict[str, Dict]:
    """Extract benchmark data into a dictionary keyed by benchmark name"""
    benchmarks = {}
    for bench in results.get('benchmarks', []):
        name = bench.get('name', '')
        benchmarks[name] = bench
    return benchmarks


def format_time(time_ns: float) -> str:
    """Format time in appropriate units"""
    if time_ns < 1000:
        return f"{time_ns:.2f} ns"
    elif time_ns < 1000000:
        return f"{time_ns/1000:.2f} µs"
    elif time_ns < 1000000000:
        return f"{time_ns/1000000:.2f} ms"
    else:
        return f"{time_ns/1000000000:.2f} s"


def format_percentage(ratio: float) -> str:
    """Format percentage change with color"""
    pct = (ratio - 1) * 100
    if abs(pct) < 1.0:
        color = Colors.NC
        status = "≈"
    elif pct < 0:
        color = Colors.GREEN
        status = "↓"
    else:
        color = Colors.RED
        status = "↑"
    
    return f"{color}{status} {abs(pct):>6.2f}%{Colors.NC}"


def compare_benchmarks(baseline: Dict[str, Dict], current: Dict[str, Dict]) -> List[Tuple]:
    """Compare two sets of benchmarks"""
    comparisons = []
    
    # Find common benchmarks
    common_names = set(baseline.keys()) & set(current.keys())
    
    for name in sorted(common_names):
        base = baseline[name]
        curr = current[name]
        
        # Get CPU time (in nanoseconds)
        base_time = base.get('cpu_time', base.get('real_time', 0))
        curr_time = curr.get('cpu_time', curr.get('real_time', 0))
        
        if base_time == 0:
            continue
        
        ratio = curr_time / base_time
        
        comparisons.append({
            'name': name,
            'baseline_time': base_time,
            'current_time': curr_time,
            'ratio': ratio,
            'iterations_base': base.get('iterations', 0),
            'iterations_curr': curr.get('iterations', 0),
        })
    
    return comparisons


def print_comparison_table(comparisons: List[Dict]):
    """Print comparison results in a formatted table"""
    if not comparisons:
        print(f"{Colors.YELLOW}No common benchmarks found to compare{Colors.NC}")
        return
    
    # Header
    print(f"\n{Colors.BOLD}{'Benchmark':<50} {'Baseline':>12} {'Current':>12} {'Change':>15}{Colors.NC}")
    print("-" * 95)
    
    # Rows
    for comp in comparisons:
        name = comp['name']
        if len(name) > 47:
            name = name[:44] + "..."
        
        baseline_str = format_time(comp['baseline_time'])
        current_str = format_time(comp['current_time'])
        change_str = format_percentage(comp['ratio'])
        
        print(f"{name:<50} {baseline_str:>12} {current_str:>12} {change_str:>15}")


def print_summary(comparisons: List[Dict]):
    """Print summary statistics"""
    if not comparisons:
        return
    
    total = len(comparisons)
    improved = sum(1 for c in comparisons if c['ratio'] < 0.99)
    regressed = sum(1 for c in comparisons if c['ratio'] > 1.01)
    unchanged = total - improved - regressed
    
    # Calculate geometric mean of ratios
    import math
    geomean = math.exp(sum(math.log(c['ratio']) for c in comparisons) / total)
    
    print(f"\n{Colors.BOLD}=== Summary ==={Colors.NC}")
    print(f"Total benchmarks compared: {total}")
    print(f"{Colors.GREEN}Improved: {improved} ({improved/total*100:.1f}%){Colors.NC}")
    print(f"{Colors.RED}Regressed: {regressed} ({regressed/total*100:.1f}%){Colors.NC}")
    print(f"Unchanged: {unchanged} ({unchanged/total*100:.1f}%)")
    print(f"\nGeometric mean of speedup: {format_percentage(geomean)}")
    
    # Find biggest improvements and regressions
    if improved > 0:
        best = min(comparisons, key=lambda x: x['ratio'])
        speedup = 1 / best['ratio']
        print(f"\n{Colors.GREEN}Best improvement:{Colors.NC}")
        print(f"  {best['name']}: {speedup:.2f}x faster")
    
    if regressed > 0:
        worst = max(comparisons, key=lambda x: x['ratio'])
        slowdown = worst['ratio']
        print(f"\n{Colors.RED}Worst regression:{Colors.NC}")
        print(f"  {worst['name']}: {slowdown:.2f}x slower")


def main():
    parser = argparse.ArgumentParser(
        description='Compare Google Benchmark results',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Compare two benchmark runs
  %(prog)s baseline.json current.json
  
  # Show only regressions
  %(prog)s baseline.json current.json --regressions-only
  
  # Filter to specific benchmarks
  %(prog)s baseline.json current.json --filter "BM_TopK"
  
  # Export comparison to CSV
  %(prog)s baseline.json current.json --csv output.csv
        """
    )
    
    parser.add_argument('baseline', help='Baseline benchmark results (JSON)')
    parser.add_argument('current', help='Current benchmark results (JSON)')
    parser.add_argument('--filter', help='Only compare benchmarks matching this pattern')
    parser.add_argument('--regressions-only', action='store_true',
                       help='Show only regressions (slower results)')
    parser.add_argument('--improvements-only', action='store_true',
                       help='Show only improvements (faster results)')
    parser.add_argument('--csv', help='Export results to CSV file')
    parser.add_argument('--threshold', type=float, default=1.0,
                       help='Minimum percentage change to show (default: 1.0)')
    
    args = parser.parse_args()
    
    # Load results
    print(f"{Colors.BLUE}Loading baseline: {args.baseline}{Colors.NC}")
    baseline_data = load_benchmark_results(args.baseline)
    baseline_benchmarks = extract_benchmarks(baseline_data)
    
    print(f"{Colors.BLUE}Loading current: {args.current}{Colors.NC}")
    current_data = load_benchmark_results(args.current)
    current_benchmarks = extract_benchmarks(current_data)
    
    # Compare
    comparisons = compare_benchmarks(baseline_benchmarks, current_benchmarks)
    
    # Filter if requested
    if args.filter:
        import re
        pattern = re.compile(args.filter)
        comparisons = [c for c in comparisons if pattern.search(c['name'])]
    
    # Apply threshold
    threshold_ratio = 1 + (args.threshold / 100)
    comparisons = [c for c in comparisons 
                   if c['ratio'] < (1/threshold_ratio) or c['ratio'] > threshold_ratio]
    
    # Filter by improvement/regression
    if args.regressions_only:
        comparisons = [c for c in comparisons if c['ratio'] > 1.01]
    elif args.improvements_only:
        comparisons = [c for c in comparisons if c['ratio'] < 0.99]
    
    # Print results
    print_comparison_table(comparisons)
    print_summary(comparisons)
    
    # Export to CSV if requested
    if args.csv:
        import csv
        with open(args.csv, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Benchmark', 'Baseline (ns)', 'Current (ns)', 'Ratio', 'Change %'])
            for c in comparisons:
                change_pct = (c['ratio'] - 1) * 100
                writer.writerow([
                    c['name'],
                    c['baseline_time'],
                    c['current_time'],
                    f"{c['ratio']:.4f}",
                    f"{change_pct:+.2f}%"
                ])
        print(f"\n{Colors.GREEN}Results exported to: {args.csv}{Colors.NC}")


if __name__ == '__main__':
    main()
