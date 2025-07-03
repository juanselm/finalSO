#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>

// Funciones de utilidad para E/O de archivos (genéricas)
int write_header(FILE *fp, const void *header);
int read_header(FILE *fp, void *header);
int write_block_info(FILE *fp, const void *info);
int read_block_info(FILE *fp, void *info);

// Funciones de utilidad para archivos
long get_file_size(const char *filename);
int file_exists(const char *filename);
void print_progress(int current, int total, const char *message);

// Funciones de validación
int validate_block_size(int block_size);
int validate_threads(int threads);
int validate_compression_level(int level);

#endif
