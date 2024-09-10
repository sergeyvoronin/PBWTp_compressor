#!/usr/bin/env python3
import os
import sys
import json
import re
import numpy as np
from sklearn.cluster import KMeans
from concurrent.futures import ThreadPoolExecutor
import random

def parse_size(size_str):
    units = {"B": 1, "KB": 1024, "MB": 1024**2, "GB": 1024**3}
    match = re.match(r"(\d+(\.\d+)?)([KMG]?B)", size_str.upper())
    if not match:
        raise ValueError("Invalid size format")
    size = float(match.group(1))
    unit = match.group(3)
    size_in_bytes = int(size * units[unit])
    return size_in_bytes


def calculate_byte_frequency(data):
    frequency = np.zeros(256, dtype=int)
    for byte in data:
        frequency[byte] += 1
    return frequency


def group_blocks_by_similarity(block_frequencies, block_sizes, num_clusters, max_megablock_size):
    frequencies = np.array(block_frequencies)
    kmeans = KMeans(n_clusters=num_clusters, random_state=0, init='k-means++', n_init='auto').fit(frequencies)
    labels = kmeans.labels_

    clusters = {i: [] for i in range(num_clusters)}
    cluster_sizes = {i: 0 for i in range(num_clusters)}

    for i, label in enumerate(labels):
        block_size = block_sizes[i]

        if cluster_sizes[label] + block_size > max_megablock_size:
            # Try to find a suitable existing cluster
            found_cluster = False
            for alt_label in range(num_clusters):
                if cluster_sizes[alt_label] + block_size <= max_megablock_size:
                    label = alt_label
                    found_cluster = True
                    break

            if not found_cluster:
                # Create a new cluster if no suitable one exists
                new_label = max(clusters.keys()) + 1
                clusters[new_label] = []
                cluster_sizes[new_label] = 0
                label = new_label

        clusters[label].append(i)
        cluster_sizes[label] += block_size

    return list(clusters.values())


def save_megablock(megablock_data, megablock_file):
    with open(megablock_file, 'wb') as mb_file:
        mb_file.write(megablock_data)


def split_into_megablocks(input_file, output_dir, block_size, num_clusters, max_megablock_size):
    os.makedirs(output_dir, exist_ok=True)

    blocks = []
    block_frequencies = []
    block_positions = []
    block_sizes = []

    with open(input_file, 'rb') as f:
        position = 0
        while True:
            size_variation = random.randint(-block_size // 10, block_size // 10)
            adjusted_block_size = max(1, block_size + size_variation)

            data = f.read(adjusted_block_size)
            if not data:
                break

            blocks.append(data)
            block_frequencies.append(calculate_byte_frequency(data))
            block_positions.append(position)
            block_sizes.append(len(data))
            position += len(data)

    megablock_groups = group_blocks_by_similarity(block_frequencies, block_sizes, num_clusters, max_megablock_size)

    metadata = {
        "megablocks": []
    }

    with ThreadPoolExecutor() as executor:
        futures = []
        for mb_index, group in enumerate(megablock_groups):
            megablock_data = b''.join(blocks[i] for i in group)
            megablock_file = os.path.join(output_dir, f"megablock_{mb_index}.dat")
            futures.append(executor.submit(save_megablock, megablock_data, megablock_file))

            metadata["megablocks"].append({
                "megablock_file": megablock_file,
                "block_positions": [{"position": block_positions[i], "size": len(blocks[i])} for i in group]
            })

        # Ensure all writes are complete
        for future in futures:
            future.result()

        # Explicitly shutdown the executor
        executor.shutdown(wait=True)

    save_metadata(metadata, output_dir)
    print(f"Total Megablocks Created: {len(megablock_groups)}")


def save_metadata(metadata, output_dir):
    metadata_path = os.path.join(output_dir, 'metadata.json')
    with open(metadata_path, 'w') as f:
        json.dump(metadata, f, indent=4)

def main():
    if len(sys.argv) != 6:
        print("Usage: python forward.py <input_file> <output_dir> <block_size_str> <num_clusters> <max_megablock_size_str>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    block_size_str = sys.argv[3]
    block_size = parse_size(block_size_str)
    num_clusters = int(sys.argv[4])
    max_megablock_size_str = sys.argv[5]
    max_megablock_size = parse_size(max_megablock_size_str)

    os.makedirs(output_dir, exist_ok=True)

    split_into_megablocks(input_file, output_dir, block_size, num_clusters, max_megablock_size)

    print("Forward processing complete.")

if __name__ == "__main__":
    main()
