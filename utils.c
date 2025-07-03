#include "utils.h"
#include "compressor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Funciones específicas para el formato ParZip
int write_parzip_header(FILE *fp, const parzip_header_t *header) {
    if (!fp || !header) return -1;
    size_t written = fwrite(header, sizeof(parzip_header_t), 1, fp);
    return (written == 1) ? 0 : -1;
}

int read_parzip_header(FILE *fp, parzip_header_t *header) {
    if (!fp || !header) return -1;
    size_t read = fread(header, sizeof(parzip_header_t), 1, fp);
    return (read == 1) ? 0 : -1;
}

int write_parzip_block_info(FILE *fp, const block_info_t *info) {
    if (!fp || !info) return -1;
    size_t written = fwrite(info, sizeof(block_info_t), 1, fp);
    return (written == 1) ? 0 : -1;
}

int read_parzip_block_info(FILE *fp, block_info_t *info) {
    if (!fp || !info) return -1;
    size_t read = fread(info, sizeof(block_info_t), 1, fp);
    return (read == 1) ? 0 : -1;
}

// Funciones de E/O genéricas para compatibilidad
int write_header(FILE *fp, const void *header) {
    return write_parzip_header(fp, (const parzip_header_t*)header);
}

int read_header(FILE *fp, void *header) {
    return read_parzip_header(fp, (parzip_header_t*)header);
}

int write_block_info(FILE *fp, const void *info) {
    return write_parzip_block_info(fp, (const block_info_t*)info);
}

int read_block_info(FILE *fp, void *info) {
    return read_parzip_block_info(fp, (block_info_t*)info);
}

// Funciones de utilidad para archivos
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void print_progress(int current, int total, const char *message) {
    int percent = (current * 100) / total;
    int bar_length = 50;
    int filled = (current * bar_length) / total;
    
    printf("\r%s [", message);
    for (int i = 0; i < bar_length; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("] %d%% (%d/%d)", percent, current, total);
    fflush(stdout);
    
    if (current == total) printf("\n");
}

// Funciones de validación
int validate_block_size(int block_size) {
    if (block_size < 1024 || block_size > 16777216) { // 1KB - 16MB
        fprintf(stderr, "Error: Tamaño de bloque debe estar entre 1KB y 16MB\n");
        return -1;
    }
    return 0;
}

int validate_threads(int threads) {
    if (threads < 1 || threads > MAX_THREADS) {
        fprintf(stderr, "Error: Número de hilos debe estar entre 1 y %d\n", MAX_THREADS);
        return -1;
    }
    return 0;
}

int validate_compression_level(int level) {
    if (level < 0 || level > 9) {
        fprintf(stderr, "Error: Nivel de compresión debe estar entre 0 y 9\n");
        return -1;
    }
    return 0;
}
