#!/usr/bin/env python3
# Reconstruct BWT output from clustered megablocks and metadata
import os
import sys
import json

def reconstruct_bwt(output_file, output_dir):
    """
    Reconstructs the original BWT from the megablocks using metadata.

    :param output_file: Path to save the reconstructed BWT.
    :param output_dir: Directory containing megablocks and metadata.
    """
    metadata_file = os.path.join(output_dir, 'metadata.json')

    # Load metadata
    with open(metadata_file, 'r') as f:
        metadata = json.load(f)

    # Prepare to write the reconstructed BWT
    with open(output_file, 'wb') as out_file:
        for megablock in metadata["megablocks"]:
            megablock_file = megablock["megablock_file"]
            block_positions = megablock["block_positions"]

            # Read the megablock data
            with open(megablock_file, 'rb') as mb_file:
                for block_info in block_positions:
                    start_pos = block_info["position"]
                    size = block_info["size"]

                    # Read the specific block from the megablock
                    block_data = mb_file.read(size)
                    
                    # Seek to the correct position in the output BWT file
                    out_file.seek(start_pos)
                    
                    # Write the block data to the output BWT file
                    out_file.write(block_data)

    print(f"Reconstruction complete. Output saved to {output_file}")

def main():
    if len(sys.argv) != 3:
        print("Usage: python reconstruct.py <output_dir> <output_file>")
        sys.exit(1)

    output_dir = sys.argv[1]
    output_file = sys.argv[2]

    reconstruct_bwt(output_file, output_dir)

if __name__ == "__main__":
    main()

