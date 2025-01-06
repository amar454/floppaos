from PIL import Image, ImageFont, ImageDraw
import numpy as np

def generate_vga_font_bitmap(font_path, font_size, output_file):
    # Create an image to draw characters
    char_width, char_height = 8, 16  # VGA character dimensions
    font = ImageFont.truetype(font_path, font_size)
    
    # Initialize the bitmap array for ASCII 32 to 127 (96 characters)
    vga_font = []

    for char_code in range(32, 128):
        char = chr(char_code)
        image = Image.new('1', (char_width, char_height), 0)  # 1-bit image (black and white)
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), char, font=font, fill=1)
        
        # Convert image to binary representation (rows of 8 bits)
        bitmap = []
        for y in range(char_height):
            row = 0
            for x in range(char_width):
                if image.getpixel((x, y)) > 0:
                    row |= (1 << (7 - x))
            bitmap.append(row)
        vga_font.append(bitmap)

    # Write the bitmap to a file
    with open(output_file, 'w') as f:
        f.write("static unsigned char g_8x16_font[96][16] = {\n")
        for bitmap in vga_font:
            f.write("    { " + ", ".join(f"0x{row:02X}" for row in bitmap) + " },\n")
        f.write("};\n")

    print(f"VGA font bitmap saved to {output_file}")

# Example usage
font_path = "path/to/your/font.ttf"  # Replace with your font path
font_size = 16  # Match VGA character height
output_file = "vga_font.c"  # Output C file

generate_vga_font_bitmap(font_path, font_size, output_file)