#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <zlib.h>

#define DEFAULT_BLOCK_SIZE 65536  // 64KB blocks
#define DEFAULT_THREADS 4
#define MAX_THREADS 32
#define MAGIC_NUMBER 0x504152574F52ULL // "PARZIP" in hex

// Estructura para el header del archivo comprimido
typedef struct {
    uint64_t magic;           // Número mágico para identificar el formato
    uint32_t num_blocks;      // Número total de bloques
    uint32_t block_size;      // Tamaño de cada bloque
    uint32_t compression_level; // Nivel de compresión usado
    uint64_t original_size;   // Tamaño original del archivo
} parzip_header_t;

// Estructura para información de un bloque
typedef struct {
    uint32_t block_id;        // ID del bloque
    uint32_t original_size;   // Tamaño original del bloque
    uint32_t compressed_size; // Tamaño comprimido del bloque
    uint64_t offset;          // Offset en el archivo comprimido
} block_info_t;

// Estructura para datos de un hilo de trabajo
typedef struct {
    int thread_id;
    const char *input_file;
    const char *output_file;
    uint32_t block_id;
    uint32_t block_size;
    uint64_t file_offset;
    uint32_t actual_size;
    int compression_level;
    pthread_mutex_t *output_mutex;
    FILE *output_fp;
    block_info_t *block_info;
    int *error_flag;
} thread_data_t;

// Funciones principales
int compress_file(const char *input_file, const char *output_file, int threads, int block_size, int compression_level);
int decompress_file(const char *input_file, const char *output_file, int threads);
int get_cpu_count(void);

// Funciones auxiliares
void* compress_block_thread(void* arg);
void* decompress_block_thread(void* arg);
int write_parzip_header(FILE *fp, const parzip_header_t *header);
int read_parzip_header(FILE *fp, parzip_header_t *header);
int write_parzip_block_info(FILE *fp, const block_info_t *info);
int read_parzip_block_info(FILE *fp, block_info_t *info);

#endif
