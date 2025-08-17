#!/usr/bin/env python3
"""
Memory Allocation Analyzer for OS Debug Logs
Visualizes memory allocation patterns from SLAB allocator and physical memory manager
"""

import re
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from collections import defaultdict, OrderedDict
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import seaborn as sns
import argparse
import sys

class MemoryAnalyzer:
    def __init__(self, log_file):
        self.log_file = log_file
        self.allocations = []
        self.frees = []
        self.slab_caches = {}
        self.physical_allocations = []
        self.memory_stats = []
        self.timeline = []

    def parse_logs(self):
        """Parse the log file and extract memory-related information"""

        # Patterns for different types of memory operations
        patterns = {
            'slab_alloc': re.compile(r'SLAB alloc\(\): size=(\d+) cache=([x\w]+) ptr=([x\w]+)'),
            'slab_free': re.compile(r'free: Freeing a slab of size (\d+) and capacity (\d+)'),
            'slab_cache': re.compile(r'SLAB: new cache: size (\d+) \| align (\d+) \| true size (\d+) \| obj count (\d+)'),
            'pm_alloc': re.compile(r'pm_get: .*allocated (\d+) pages at (0x[\w]+)'),
            'memory_stats': re.compile(r'(Total|Free|Used) Memory: (\d+) MB \((\d+) bytes\)'),
            'timestamp': re.compile(r'^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) (\d+)')
        }

        current_time = None

        try:
            with open(self.log_file, 'r') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()

                    # Extract timestamp
                    ts_match = patterns['timestamp'].match(line)
                    if ts_match and ts_match.group(1) != '0000-00-00 00:00:00':
                        try:
                            current_time = datetime.strptime(ts_match.group(1), '%Y-%m-%d %H:%M:%S')
                            current_time += timedelta(milliseconds=int(ts_match.group(2)))
                        except:
                            pass

                    # Parse SLAB allocations
                    slab_alloc = patterns['slab_alloc'].search(line)
                    if slab_alloc:
                        self.allocations.append({
                            'type': 'slab',
                            'size': int(slab_alloc.group(1)),
                            'cache': slab_alloc.group(2),
                            'ptr': slab_alloc.group(3),
                            'timestamp': current_time,
                            'line': line_num
                        })

                    # Parse SLAB frees
                    slab_free = patterns['slab_free'].search(line)
                    if slab_free:
                        self.frees.append({
                            'type': 'slab',
                            'size': int(slab_free.group(1)),
                            'capacity': int(slab_free.group(2)),
                            'timestamp': current_time,
                            'line': line_num
                        })

                    # Parse SLAB cache creation
                    slab_cache = patterns['slab_cache'].search(line)
                    if slab_cache:
                        cache_size = int(slab_cache.group(1))
                        self.slab_caches[cache_size] = {
                            'size': cache_size,
                            'align': int(slab_cache.group(2)),
                            'true_size': int(slab_cache.group(3)),
                            'obj_count': int(slab_cache.group(4))
                        }

                    # Parse physical memory allocations
                    pm_alloc = patterns['pm_alloc'].search(line)
                    if pm_alloc:
                        self.physical_allocations.append({
                            'pages': int(pm_alloc.group(1)),
                            'address': pm_alloc.group(2),
                            'timestamp': current_time,
                            'line': line_num
                        })

                    # Parse memory statistics
                    mem_stats = patterns['memory_stats'].search(line)
                    if mem_stats:
                        self.memory_stats.append({
                            'type': mem_stats.group(1).lower(),
                            'mb': int(mem_stats.group(2)),
                            'bytes': int(mem_stats.group(3)),
                            'timestamp': current_time,
                            'line': line_num
                        })

        except FileNotFoundError:
            print(f"Error: File '{self.log_file}' not found")
            sys.exit(1)
        except Exception as e:
            print(f"Error parsing log file: {e}")
            sys.exit(1)

    def create_allocation_timeline(self, output_dir="output"):
        """Create a timeline visualization of allocations"""
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(15, 10))

        # SLAB allocations over time
        if self.allocations:
            slab_times = [a['timestamp'] for a in self.allocations if a['timestamp']]
            slab_sizes = [a['size'] for a in self.allocations if a['timestamp']]

            if slab_times:
                ax1.scatter(slab_times, slab_sizes, alpha=0.6, s=20, c='blue')
                ax1.set_ylabel('SLAB Allocation Size (bytes)')
                ax1.set_title('SLAB Allocations Over Time')
                ax1.tick_params(axis='x', rotation=45)

        # Physical memory allocations over time
        if self.physical_allocations:
            pm_times = [a['timestamp'] for a in self.physical_allocations if a['timestamp']]
            pm_pages = [a['pages'] for a in self.physical_allocations if a['timestamp']]

            if pm_times:
                ax2.scatter(pm_times, pm_pages, alpha=0.6, s=20, c='red')
                ax2.set_ylabel('Pages Allocated')
                ax2.set_xlabel('Time')
                ax2.set_title('Physical Memory Allocations Over Time')
                ax2.tick_params(axis='x', rotation=45)

        plt.tight_layout()
        filename = f"{output_dir}/allocation_timeline.png"
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"Timeline visualization saved to: {filename}")

    def create_size_distribution(self, output_dir="output"):
        """Create histograms showing allocation size distributions"""
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

        # SLAB allocation sizes
        slab_sizes = [a['size'] for a in self.allocations]
        if slab_sizes:
            ax1.hist(slab_sizes, bins=50, alpha=0.7, color='blue', edgecolor='black')
            ax1.set_xlabel('Allocation Size (bytes)')
            ax1.set_ylabel('Frequency')
            ax1.set_title('SLAB Allocation Size Distribution')
            ax1.set_yscale('log')  # Log scale for better visibility

        # Physical memory allocation sizes (in pages)
        pm_sizes = [a['pages'] * 4096 for a in self.physical_allocations]  # Assuming 4KB pages
        if pm_sizes:
            ax2.hist(pm_sizes, bins=30, alpha=0.7, color='red', edgecolor='black')
            ax2.set_xlabel('Allocation Size (bytes)')
            ax2.set_ylabel('Frequency')
            ax2.set_title('Physical Memory Allocation Size Distribution')
            ax2.set_yscale('log')

        plt.tight_layout()
        filename = f"{output_dir}/size_distribution.png"
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"Size distribution chart saved to: {filename}")

    def create_cache_usage_chart(self, output_dir="output"):
        """Show SLAB cache usage patterns"""
        if not self.slab_caches:
            print("No SLAB cache information found")
            return

        cache_usage = defaultdict(int)
        for alloc in self.allocations:
            if alloc['type'] == 'slab':
                cache_usage[alloc['size']] += 1

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

        # Cache configuration
        cache_sizes = list(self.slab_caches.keys())
        true_sizes = [self.slab_caches[size]['true_size'] for size in cache_sizes]
        obj_counts = [self.slab_caches[size]['obj_count'] for size in cache_sizes]

        ax1.bar(range(len(cache_sizes)), true_sizes, alpha=0.7, color='green')
        ax1.set_xlabel('Cache Index')
        ax1.set_ylabel('True Size (bytes)')
        ax1.set_title('SLAB Cache Configurations')
        ax1.set_xticks(range(len(cache_sizes)))
        ax1.set_xticklabels([f"{size}b" for size in cache_sizes], rotation=45)

        # Cache usage frequency
        if cache_usage:
            sizes = list(cache_usage.keys())
            counts = list(cache_usage.values())
            ax2.bar(range(len(sizes)), counts, alpha=0.7, color='orange')
            ax2.set_xlabel('Allocation Size')
            ax2.set_ylabel('Allocation Count')
            ax2.set_title('SLAB Cache Usage Frequency')
            ax2.set_xticks(range(len(sizes)))
            ax2.set_xticklabels([f"{size}b" for size in sizes], rotation=45)

        plt.tight_layout()
        filename = f"{output_dir}/cache_usage.png"
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"Cache usage chart saved to: {filename}")

    def create_memory_map_visualization(self, output_dir="output"):
        """Create a visual memory map of allocations"""
        if not self.physical_allocations:
            print("No physical memory allocation data found")
            return

        fig, ax = plt.subplots(1, 1, figsize=(15, 10))

        # Sort allocations by address
        sorted_allocs = sorted(self.physical_allocations,
                             key=lambda x: int(x['address'], 16) if x['address'].startswith('0x') else 0)

        y_pos = 0
        colors = plt.cm.Set3(np.linspace(0, 1, len(sorted_allocs)))

        for i, alloc in enumerate(sorted_allocs):
            try:
                addr = int(alloc['address'], 16)
                size = alloc['pages'] * 4096  # Assuming 4KB pages

                # Create rectangle for this allocation
                rect = patches.Rectangle((addr, y_pos), size, 1,
                                       facecolor=colors[i], alpha=0.7,
                                       edgecolor='black', linewidth=0.5)
                ax.add_patch(rect)

                # Add label
                ax.text(addr + size/2, y_pos + 0.5, f"{alloc['pages']}p",
                       ha='center', va='center', fontsize=8)

                y_pos += 1.2
            except ValueError:
                continue

        ax.set_xlim(0, max([int(a['address'], 16) + a['pages'] * 4096
                           for a in sorted_allocs if a['address'].startswith('0x')]))
        ax.set_ylim(0, y_pos)
        ax.set_xlabel('Memory Address')
        ax.set_ylabel('Allocation Order')
        ax.set_title('Physical Memory Layout')
        plt.tight_layout()
        filename = f"{output_dir}/memory_map.png"
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"Memory map visualization saved to: {filename}")

    def generate_summary_report(self, output_dir="output"):
        """Generate a text summary of memory usage"""
        report_lines = []
        report_lines.append("=" * 60)
        report_lines.append("MEMORY ALLOCATION ANALYSIS REPORT")
        report_lines.append("=" * 60)

        report_lines.append(f"\nTotal SLAB Allocations: {len(self.allocations)}")
        report_lines.append(f"Total SLAB Frees: {len(self.frees)}")
        report_lines.append(f"Total Physical Memory Allocations: {len(self.physical_allocations)}")

        if self.allocations:
            total_slab_bytes = sum(a['size'] for a in self.allocations)
            avg_slab_size = total_slab_bytes / len(self.allocations)
            report_lines.append(f"Total SLAB Memory Allocated: {total_slab_bytes:,} bytes ({total_slab_bytes/1024/1024:.2f} MB)")
            report_lines.append(f"Average SLAB Allocation Size: {avg_slab_size:.2f} bytes")

        if self.physical_allocations:
            total_pages = sum(a['pages'] for a in self.physical_allocations)
            total_pm_bytes = total_pages * 4096
            report_lines.append(f"Total Physical Pages Allocated: {total_pages:,} pages ({total_pm_bytes/1024/1024:.2f} MB)")

        report_lines.append(f"\nNumber of SLAB Caches: {len(self.slab_caches)}")
        if self.slab_caches:
            report_lines.append("Cache Sizes: " + str(sorted(self.slab_caches.keys())))

        # Memory leak analysis
        net_slab_allocs = len(self.allocations) - len(self.frees)
        report_lines.append(f"\nNet SLAB Allocations (potential leaks): {net_slab_allocs}")

        # Latest memory statistics
        if self.memory_stats:
            latest_stats = {}
            for stat in reversed(self.memory_stats):
                if stat['type'] not in latest_stats:
                    latest_stats[stat['type']] = stat

            report_lines.append("\nLatest Memory Statistics:")
            for stat_type, stat in latest_stats.items():
                report_lines.append(f"  {stat_type.title()}: {stat['mb']} MB ({stat['bytes']:,} bytes)")

        # Print to console
        for line in report_lines:
            print(line)

        # Save to file
        filename = f"{output_dir}/memory_analysis_report.txt"
        with open(filename, 'w') as f:
            f.write('\n'.join(report_lines))
        print(f"\nDetailed report saved to: {filename}")

def main():
    parser = argparse.ArgumentParser(description='Analyze OS memory allocation logs')
    parser.add_argument('logfile', help='Path to the log file')
    parser.add_argument('--timeline', action='store_true', help='Show allocation timeline')
    parser.add_argument('--distribution', action='store_true', help='Show size distribution')
    parser.add_argument('--caches', action='store_true', help='Show cache usage')
    parser.add_argument('--memmap', action='store_true', help='Show memory map')
    parser.add_argument('--all', action='store_true', help='Show all visualizations')
    parser.add_argument('--output', '-o', default='output', help='Output directory (default: output)')

    args = parser.parse_args()

    # Set style
    plt.style.use('default')
    sns.set_palette("husl")

    # Create output directory
    import os
    os.makedirs(args.output, exist_ok=True)

    analyzer = MemoryAnalyzer(args.logfile)
    print(f"Parsing log file: {args.logfile}")
    analyzer.parse_logs()

    # Generate summary report
    analyzer.generate_summary_report(args.output)

    # Show requested visualizations
    if args.all or args.timeline:
        print("\nGenerating allocation timeline...")
        analyzer.create_allocation_timeline(args.output)

    if args.all or args.distribution:
        print("Generating size distribution charts...")
        analyzer.create_size_distribution(args.output)

    if args.all or args.caches:
        print("Generating cache usage charts...")
        analyzer.create_cache_usage_chart(args.output)

    if args.all or args.memmap:
        print("Generating memory map visualization...")
        analyzer.create_memory_map_visualization(args.output)

    if not any([args.timeline, args.distribution, args.caches, args.memmap, args.all]):
        print(f"\nTo generate visualizations, use one of: --timeline, --distribution, --caches, --memmap, or --all")
        print(f"All output will be saved to the '{args.output}' directory")

    print(f"\nAnalysis complete! All files saved to '{args.output}' directory")

if __name__ == "__main__":
    main()