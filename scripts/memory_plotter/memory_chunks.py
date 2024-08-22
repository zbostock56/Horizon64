import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import os
import parse

def save_memory_map_chunks(memory_map, chunk_size=1024*1024, min_sections=2, tick_marks=1024*1024):
    # Create the output directory if it doesn't exist
    output_dir = 'chunks'
    os.makedirs(output_dir, exist_ok=True)
    
    # Get the range of memory addresses
    min_addr = min([s["start"] for s in memory_map])
    max_addr = max([s["end"] for s in memory_map])
    
    chunk_start = min_addr
    chunk_index = 1

    start = chunk_start
    
    while chunk_start < max_addr:
        chunk_end = min(chunk_start + chunk_size, max_addr)

        # Filter memory sections within the current chunk
        sections_in_chunk = [section for section in memory_map 
                             if section["end"] > chunk_start and section["start"] < chunk_end]
        
        # Skip chunks that have fewer than the minimum required sections
        if len(sections_in_chunk) < min_sections:
            chunk_start = chunk_end
            continue
        
        #print_progress_bar(chunk_start - start, max_addr - start)
        fig, ax = plt.subplots(figsize=(12, 8))
        
        # Plot each memory segment as a colored rectangle within the chunk
        for section in sections_in_chunk:
            # Plot as hash marked if not mapped
            hatch = 'x' if section['hash'] else None
            
            rect = patches.Rectangle((0.3, max(section["start"], chunk_start)), 0.4, min(section["end"], chunk_end) - max(section["start"], chunk_start),
                                     edgecolor="black", facecolor=section["color"], hatch=hatch)
            ax.add_patch(rect)
            # Place the label in the middle of each section
            midpoint = (max(section["start"], chunk_start) + min(section["end"], chunk_end)) / 2
            ax.text(0.5, midpoint, section["name"], ha="center", va="center", fontsize=6, clip_on=True)

        # Set limits and use linear scale for y-axis with 1 MB ticks
        ax.set_xlim([0, 1])
        ax.set_ylim([chunk_start, chunk_end])
        ax.set_yticks([s for s in range(chunk_start, chunk_end, tick_marks)])
        ax.set_yticklabels([f"0x{tick:X}" for tick in ax.get_yticks()])

        # Title and save the plot
        plt.title(f"Memory Map Chunk {chunk_index}", fontsize=14)
        plt.tight_layout()
        plt.savefig(f"{output_dir}/memory_map_chunk_{chunk_index}_0x{chunk_start:X}_to_0x{chunk_end:X}.png", dpi=300, bbox_inches='tight')
        plt.close()

        chunk_start = chunk_end
        chunk_index += 1


def print_progress_bar(current_value, total_value, bar_length=50):
    progress = current_value / total_value
    block = int(round(bar_length * progress))
    
    progress_bar = "#" * block + "-" * (bar_length - block)
    percent = round(progress * 100, 2)
    
    print(f"\rProgress: [{progress_bar}] {percent}%", end="", flush=True)

# Call the function to create and save the chunks
memory_map = parse.parse_memory_map("mem_map.txt")
save_memory_map_chunks(memory_map, chunk_size=1024*256, min_sections=2, tick_marks=1024*64)
