# Parallel File Compressor

## Overview
This project implements a parallel file compressor using pthreads and zlib.

## Features
- File splitting into blocks
- Parallel compression using thread pool
- Custom archive format with metadata
- CLI options for threads, block size, and compression level
- Performance metrics output

## Usage
```bash
./parzip input.txt output.pz
./parzip -t 4 -b 1024 -l 9 input.txt output.pz
```

## Build
```bash
make
```

## Clean
```bash
make clean
```

## Performance Metrics
- Input/output size in bytes
- Compression ratio
- Total execution time (seconds)
- Average speed (MB/s)
