import re

color = {
    'LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE': 'orange',
    'LIMINE_MEMMAP_USABLE': 'lightgreen',
    'LIMINE_MEMMAP_RESERVED': 'red',
    'LIMINE_MEMMAP_FRAMEBUFFER': 'purple',
    'LIMINE_MEMMAP_KERNEL_AND_MODULES': 'lightblue',
    'LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE': 'green'
}
def parse_memory_map(file_path):
    memory_map = []
    
    # Regular expressions for matching lines
    range_re = re.compile(r'(\S+) - (\S+)')
    type_re = re.compile(r'Type: (LIMINE_MEMMAP_\S+)')
    mapped_re = re.compile(r'Mapped \w+ (\S+)')
    
    # Read the file content
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    i = 0
    while i < len(lines):
        if 'Memory entry at range' in lines[i] or 'Mapped' in lines[i]:
            # Extract range and type
            range_match = range_re.search(lines[i])
            type_match = type_re.search(lines[i + 2])
            mapped_match = mapped_re.search(lines[i])
            
            if range_match and type_match:
                start_addr, end_addr = range_match.groups()
                start_addr = int(start_addr, 16)
                end_addr = int(end_addr, 16)
                
                # Determine the type
                mem_type = type_match.group(1)
                c = color.get(mem_type, 'gray')  # Default to gray if type not found
                
                # Adjust for mapped entries
                if mapped_match:
                    try:
                        virtual_start, virtual_end = range_match.groups()
                        s = int(virtual_start, 16)
                        e = int(virtual_end, 16)
                        if s - 0xffff800000000000 > 0:
                            start_addr = s - 0xffff800000000000
                        else:
                            start_addr = s
                        if e - 0xffff800000000000 > 0:
                            end_addr = e - 0xffff800000000000
                        else:
                            end_addr = e
                    except ValueError as e:
                        print(f"Warning: Failed to parse mapped addresses for entry: {lines[i]}")
                        print(f"Error: {e}")
                        # Set to original if parsing fails
                        start_addr = int(start_addr, 16)
                        end_addr = int(end_addr, 16)
                
                # Add entry to memory_map
                memory_map.append({
                    'name': mem_type.replace('LIMINE_MEMMAP_', ''),
                    'start': start_addr,
                    'end': end_addr,
                    'color': c,
                    'hash': mapped_match is None  # True if not mapped
                })
                
            i += 3  # Skip to the next entry
        else:
            i += 1  # Continue to the next line

    memory_map.sort(key=lambda x: x['start'])

    return memory_map