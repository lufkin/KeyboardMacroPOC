import sys
import struct

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END    = 0x0AB16F30
UF2_FLAG_FAMILY  = 0x00002000
RP2040_FAMILY_ID = 0xE48BFF56
FLASH_START_ADDR = 0x10000000
BLOCK_PAYLOAD_SIZE = 256

def convert_bin_to_uf2(bin_path, uf2_path):
    with open(bin_path, 'rb') as f:
        bin_data = f.read()

    total_len = len(bin_data)
    num_blocks = (total_len + BLOCK_PAYLOAD_SIZE - 1) // BLOCK_PAYLOAD_SIZE

    with open(uf2_path, 'wb') as out:
        for block_no in range(num_blocks):
            target_addr = FLASH_START_ADDR + block_no * BLOCK_PAYLOAD_SIZE
            start_offset = block_no * BLOCK_PAYLOAD_SIZE
            payload = bin_data[start_offset:start_offset + BLOCK_PAYLOAD_SIZE]
            if len(payload) < BLOCK_PAYLOAD_SIZE:
                payload = payload + b'\x00' * (BLOCK_PAYLOAD_SIZE - len(payload))

            header = struct.pack(
                '<8I',
                UF2_MAGIC_START0,
                UF2_MAGIC_START1,
                UF2_FLAG_FAMILY,
                target_addr,
                BLOCK_PAYLOAD_SIZE,
                block_no,
                num_blocks,
                RP2040_FAMILY_ID
            )
            padding = b'\x00' * (512 - 32 - BLOCK_PAYLOAD_SIZE - 4)
            footer = struct.pack('<I', UF2_MAGIC_END)

            out.write(header + payload + padding + footer)

    print(f"Generated {uf2_path} ({num_blocks} blocks)")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python bin2uf2.py <input.bin> <output.uf2>")
        sys.exit(1)
    convert_bin_to_uf2(sys.argv[1], sys.argv[2])
