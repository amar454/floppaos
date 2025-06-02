import re

def extract_function_declarations(c_file):
    """Extract function declarations from a C file."""
    func_declarations = []
    pattern = re.compile(r'\s*([a-zA-Z_][a-zA-Z0-9_]*\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\)\s*;)\s*')
    with open(c_file, 'r') as file:
        for line in file:
            match = pattern.match(line.strip())
            if match:
                func_declarations.append(match.group(1))
    return func_declarations

def create_header_file(c_file, header_file):
    """Create a header file with function declarations from a C file."""
    function_declarations = extract_function_declarations(c_file)
    header_content = f"#ifndef {header_file.upper().replace('.', '_')}\n"
    header_content += f"#define {header_file.upper().replace('.', '_')}\n\n"
    for declaration in function_declarations:
        header_content += declaration + "\n"
    header_content += "\n#endif // " + header_file.upper().replace('.', '_') + "\n"
    with open(header_file, 'w') as file:
        file.write(header_content)
    print(f"Header file '{header_file}' created successfully!")
if __name__ == "__main__":
    c_file = "../" + input("Name of C file (from root directory)") 
    h_file = "../" + input ("Name of the header file (from root directory):")
    create_header_file(c_file, header_file=h_file)
  
    
