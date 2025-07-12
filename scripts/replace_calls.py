import re
import sys

def load_kernel_map(kernel_map_file):
    function_map = {}
    in_text_section = False
    recheck = False

    # Read the kernel map file and extract the function names and their addresses
    with open(kernel_map_file, 'r') as f:
        for line in f:
            # Check if we are inside the .text section
            if in_text_section:
                if line.startswith(".iplt"):
                    break
            if line.startswith(".text"):
                in_text_section = True

            # Using re.search to match function addresses and names
            # Regex: Match lines like: "  0xffffffff80000000       sec_in_years"
            if in_text_section:
                match = re.search(r"0[xX][0-9a-f]+\s\s+[0-9a-zA-Z_]+", line)
                if match:
                    # Extract address and function name using match.group()
                    address_function = match.group(0)  # the whole match
                    parts = address_function.split()
                    if recheck == False and parts[0] == '0xffffffff80000000':
                        recheck = True
                        continue
                    if len(parts) == 2:
                        address = parts[0]
                        function_name = parts[1]

                        # Make sure the function name doesn't start with 0x (which indicates it's an address)
                        if address not in function_map and not function_name.startswith("0x"):
                            function_map[address] = function_name
                            continue

                # Second pass for handling .text.<function> style entries
                # Regex: Match lines like: ".text.halt     0xffffffff80015a97       0x12 obj/src/sys/interrupts/isr.c.o"
                match_func = re.search(r"\.text\.(\S+)\s+0[xX][0-9a-f]+\s+", line)
                if match_func:
                    function_name = match_func.group(1)
                    address = match_func.group(0).split()[1]

                    # Avoid adding duplicate function names by checking if the address already exists in the map
                    if address not in function_map and not function_name.startswith("0x"):
                        function_map[address] = function_name

    return function_map

def process_assembly(input_file, function_map):
    # Regex to match the callq instructions
    callq_regex = re.compile(r"^0[xX][0-9a-f]+:[0-9a-f\s]+callq\s+0x([0-9a-f]+)")

    with open(input_file, 'r') as f:
        for line in f:
            # Check if the line matches the callq pattern using re.search
            match = re.search(callq_regex, line)
            if match:
                target_address = match.group(1)
                target_address = "0x" + target_address
                # Check if the target address is in the function map
                if target_address in function_map:
                    function_name = function_map[target_address]
                    # Replace the target address with the function name in the original line
                    new_line = line.replace(target_address, function_name)
                    # Output the modified line with function name
                    print(new_line.strip())
                else:
                    print(f"No function name found for target address {target_address}")
            else:
                # If no match, print the line as-is (you could modify this as needed)
                print(line.strip())

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 script.py <input-asm-file> <kernel-map-file>")
        sys.exit(1)

    input_file = sys.argv[1]
    kernel_map_file = sys.argv[2]

    # Step 1: Load the kernel map and build the function name map
    function_map = load_kernel_map(kernel_map_file)

    # Debugging: Print the function map to verify it's loaded correctly
    # print("Function Name Map:")
    # for addr, name in function_map.items():
    #     print(f"0x{addr} -> {name}")
    # print()

    # Step 2: Process the assembly file and replace the callq instructions with function names
    process_assembly(input_file, function_map)

if __name__ == "__main__":
    main()
