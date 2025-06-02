
import sys
import os

def convert_f16_to_c_array(input_file, output_file=None):
    """
    Convert an F16 font file to a C array format matching the one in flanterm/backends/fb.c
    
    Args:
        input_file: Path to the F16 font file
        output_file: Path to output file (if None, prints to stdout)
    """
    try:
        with open(input_file, 'rb') as f:
            font_data = f.read()
        
        if len(font_data) != 4096:
            print(f"Warning: Input file size is {len(font_data)} bytes, expected 4096 bytes for a standard F16 font.")

        output = "#include <stdint.h> \n// generated from {} \n static const uint8_t builtin_font[] = {\n " 

        for i, byte in enumerate(font_data):
            output += f"0x{byte:02x}"

            if i < len(font_data) - 1:
                output += ", "
                
                if (i + 1) % 12 == 0:
                    output += "\n  "
        
        output += "\n};"
        
        # Output the result
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)
            print(f"Converted font written to {output_file}")
        else:
            print(output)
            
        return True
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python f16_to_c_array.py <input_f16_file> [output_file]")
        print("\nConverts an F16 font file to a C array format matching the one in flanterm/backends/fb.c")
        print("\nIf no output file is specified, the result is printed to stdout.")
        return 1
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found.", file=sys.stderr)
        return 1
    
    success = convert_f16_to_c_array(input_file, output_file)
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
