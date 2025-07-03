#include "compressor.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Función para obtener el número de CPUs
int get_cpu_count(void) {
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);
    return (cpus > 0) ? cpus : DEFAULT_THREADS;
}

// Función del hilo para comprimir un bloque
void* compress_block_thread(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    FILE *input_fp = NULL;
    unsigned char *input_buffer = NULL;
    unsigned char *output_buffer = NULL;
    uLongf compressed_size;
    int result = Z_OK;
    
    // Abrir archivo de entrada
    input_fp = fopen(data->input_file, "rb");
    if (!input_fp) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de entrada en hilo %d\n", data->thread_id);
        *data->error_flag = 1;
        return NULL;
    }
    
    // Allocar buffers
    input_buffer = malloc(data->actual_size);
    output_buffer = malloc(compressBound(data->actual_size));
    
    if (!input_buffer || !output_buffer) {
        fprintf(stderr, "Error: No se pudo allocar memoria en hilo %d\n", data->thread_id);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Leer bloque desde el archivo
    fseek(input_fp, data->file_offset, SEEK_SET);
    size_t bytes_read = fread(input_buffer, 1, data->actual_size, input_fp);
    if (bytes_read != data->actual_size) {
        fprintf(stderr, "Error: No se pudo leer el bloque completo en hilo %d\n", data->thread_id);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Comprimir el bloque
    compressed_size = compressBound(data->actual_size);
    result = compress2(output_buffer, &compressed_size, input_buffer, data->actual_size, data->compression_level);
    
    if (result != Z_OK) {
        fprintf(stderr, "Error: Fallo en compresión del bloque %d en hilo %d\n", data->block_id, data->thread_id);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Escribir bloque comprimido al archivo de salida (con mutex)
    pthread_mutex_lock(data->output_mutex);
    
    // Buscar la posición correcta en el archivo
    fseek(data->output_fp, data->block_info->offset, SEEK_SET);
    
    // Escribir datos comprimidos
    size_t written = fwrite(output_buffer, 1, compressed_size, data->output_fp);
    if (written != compressed_size) {
        fprintf(stderr, "Error: No se pudo escribir el bloque comprimido %d\n", data->block_id);
        *data->error_flag = 1;
    } else {
        // Actualizar información del bloque
        data->block_info->compressed_size = compressed_size;
        printf("✅ Bloque %d comprimido: %d -> %d bytes (%.1f%% reducción)\n", 
               data->block_id, data->actual_size, (int)compressed_size,
               100.0 * (1.0 - (double)compressed_size / data->actual_size));
    }
    
    pthread_mutex_unlock(data->output_mutex);
    
cleanup:
    if (input_fp) fclose(input_fp);
    if (input_buffer) free(input_buffer);
    if (output_buffer) free(output_buffer);
    return NULL;
}

// Función principal de compresión
int compress_file(const char *input_file, const char *output_file, int threads, int block_size, int compression_level) {
    FILE *input_fp = NULL, *output_fp = NULL;
    struct stat file_stat;
    parzip_header_t header;
    block_info_t *block_infos = NULL;
    thread_data_t *thread_data = NULL;
    pthread_t *thread_ids = NULL;
    pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
    int error_flag = 0;
    int result = 0;
    
    printf("🗂️ Iniciando compresión paralela de archivos...\n");
    printf("📁 Archivo entrada: %s\n", input_file);
    printf("📦 Archivo salida: %s\n", output_file);
    
    // Obtener información del archivo
    if (stat(input_file, &file_stat) != 0) {
        fprintf(stderr, "Error: No se pudo obtener información del archivo: %s\n", strerror(errno));
        return -1;
    }
    
    uint64_t file_size = file_stat.st_size;
    uint32_t num_blocks = (file_size + block_size - 1) / block_size;
    
    printf("📊 Tamaño archivo: %ld bytes\n", file_size);
    printf("🧩 Bloques: %d (tamaño: %d bytes)\n", num_blocks, block_size);
    printf("🧵 Hilos: %d\n", threads);
    printf("⚙️ Nivel compresión: %d\n", compression_level);
    
    // Preparar header
    header.magic = MAGIC_NUMBER;
    header.num_blocks = num_blocks;
    header.block_size = block_size;
    header.compression_level = compression_level;
    header.original_size = file_size;
    
    // Abrir archivos
    input_fp = fopen(input_file, "rb");
    output_fp = fopen(output_file, "wb");
    
    if (!input_fp || !output_fp) {
        fprintf(stderr, "Error: No se pudieron abrir los archivos\n");
        result = -1;
        goto cleanup;
    }
    
    // Escribir header
    if (write_parzip_header(output_fp, &header) != 0) {
        fprintf(stderr, "Error: No se pudo escribir el header\n");
        result = -1;
        goto cleanup;
    }
    
    // Allocar memoria para información de bloques
    block_infos = calloc(num_blocks, sizeof(block_info_t));
    thread_data = calloc(threads, sizeof(thread_data_t));
    thread_ids = calloc(threads, sizeof(pthread_t));
    
    if (!block_infos || !thread_data || !thread_ids) {
        fprintf(stderr, "Error: No se pudo allocar memoria\n");
        result = -1;
        goto cleanup;
    }
    
    // Calcular offsets para cada bloque en el archivo de salida
    uint64_t current_offset = sizeof(parzip_header_t) + num_blocks * sizeof(block_info_t);
    
    for (uint32_t i = 0; i < num_blocks; i++) {
        block_infos[i].block_id = i;
        block_infos[i].original_size = (i == num_blocks - 1) ? 
            (file_size % block_size == 0 ? block_size : file_size % block_size) : block_size;
        block_infos[i].offset = current_offset;
        current_offset += compressBound(block_infos[i].original_size);
    }
    
    printf("\n🚀 Iniciando compresión paralela...\n");
    
    // Procesar bloques con hilos
    uint32_t blocks_processed = 0;
    
    while (blocks_processed < num_blocks && !error_flag) {
        int active_threads = 0;
        
        // Lanzar hilos para procesar bloques
        for (int t = 0; t < threads && blocks_processed + t < num_blocks; t++) {
            uint32_t block_id = blocks_processed + t;
            
            thread_data[t].thread_id = t;
            thread_data[t].input_file = input_file;
            thread_data[t].output_file = output_file;
            thread_data[t].block_id = block_id;
            thread_data[t].block_size = block_size;
            thread_data[t].file_offset = (uint64_t)block_id * block_size;
            thread_data[t].actual_size = block_infos[block_id].original_size;
            thread_data[t].compression_level = compression_level;
            thread_data[t].output_mutex = &output_mutex;
            thread_data[t].output_fp = output_fp;
            thread_data[t].block_info = &block_infos[block_id];
            thread_data[t].error_flag = &error_flag;
            
            if (pthread_create(&thread_ids[t], NULL, compress_block_thread, &thread_data[t]) != 0) {
                fprintf(stderr, "Error: No se pudo crear el hilo %d\n", t);
                error_flag = 1;
                break;
            }
            active_threads++;
        }
        
        // Esperar que terminen todos los hilos
        for (int t = 0; t < active_threads; t++) {
            pthread_join(thread_ids[t], NULL);
        }
        
        blocks_processed += active_threads;
    }
    
    if (error_flag) {
        fprintf(stderr, "❌ Error durante la compresión\n");
        result = -1;
        goto cleanup;
    }
    
    // Escribir información de bloques al archivo
    fseek(output_fp, sizeof(parzip_header_t), SEEK_SET);
    for (uint32_t i = 0; i < num_blocks; i++) {
        if (write_parzip_block_info(output_fp, &block_infos[i]) != 0) {
            fprintf(stderr, "Error: No se pudo escribir información del bloque %d\n", i);
            result = -1;
            goto cleanup;
        }
    }
    
    // Calcular estadísticas
    uint64_t total_compressed = 0;
    for (uint32_t i = 0; i < num_blocks; i++) {
        total_compressed += block_infos[i].compressed_size;
    }
    
    printf("\n✅ Compresión completada exitosamente!\n");
    printf("📊 Tamaño original: %ld bytes\n", file_size);
    printf("📦 Tamaño comprimido: %ld bytes\n", total_compressed);
    printf("💾 Reducción: %.2f%%\n", 100.0 * (1.0 - (double)total_compressed / file_size));
    
cleanup:
    if (input_fp) fclose(input_fp);
    if (output_fp) fclose(output_fp);
    if (block_infos) free(block_infos);
    if (thread_data) free(thread_data);
    if (thread_ids) free(thread_ids);
    pthread_mutex_destroy(&output_mutex);
    
    return result;
}

// Función del hilo para descomprimir un bloque
void* decompress_block_thread(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    FILE *input_fp = NULL;
    unsigned char *input_buffer = NULL;
    unsigned char *output_buffer = NULL;
    uLongf decompressed_size;
    int result = Z_OK;
    
    // Abrir archivo de entrada
    input_fp = fopen(data->input_file, "rb");
    if (!input_fp) {
        fprintf(stderr, "Error: No se pudo abrir el archivo comprimido en hilo %d\n", data->thread_id);
        *data->error_flag = 1;
        return NULL;
    }
    
    // Allocar buffers
    input_buffer = malloc(data->block_info->compressed_size);
    output_buffer = malloc(data->block_info->original_size);
    
    if (!input_buffer || !output_buffer) {
        fprintf(stderr, "Error: No se pudo allocar memoria en hilo %d\n", data->thread_id);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Leer bloque comprimido desde el archivo
    fseek(input_fp, data->block_info->offset, SEEK_SET);
    size_t bytes_read = fread(input_buffer, 1, data->block_info->compressed_size, input_fp);
    if (bytes_read != data->block_info->compressed_size) {
        fprintf(stderr, "Error: No se pudo leer el bloque comprimido %d en hilo %d\n", data->block_id, data->thread_id);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Descomprimir el bloque
    decompressed_size = data->block_info->original_size;
    result = uncompress(output_buffer, &decompressed_size, input_buffer, data->block_info->compressed_size);
    
    if (result != Z_OK || decompressed_size != data->block_info->original_size) {
        fprintf(stderr, "Error: Fallo en descompresión del bloque %d en hilo %d (código: %d)\n", data->block_id, data->thread_id, result);
        *data->error_flag = 1;
        goto cleanup;
    }
    
    // Escribir bloque descomprimido al archivo de salida (con mutex)
    pthread_mutex_lock(data->output_mutex);
    
    // Calcular posición en el archivo de salida
    uint64_t output_offset = (uint64_t)data->block_id * data->block_size;
    fseek(data->output_fp, output_offset, SEEK_SET);
    
    // Escribir datos descomprimidos
    size_t written = fwrite(output_buffer, 1, decompressed_size, data->output_fp);
    if (written != decompressed_size) {
        fprintf(stderr, "Error: No se pudo escribir el bloque descomprimido %d\n", data->block_id);
        *data->error_flag = 1;
    } else {
        printf("✅ Bloque %d descomprimido: %d -> %d bytes\n", 
               data->block_id, data->block_info->compressed_size, (int)decompressed_size);
    }
    
    pthread_mutex_unlock(data->output_mutex);
    
cleanup:
    if (input_fp) fclose(input_fp);
    if (input_buffer) free(input_buffer);
    if (output_buffer) free(output_buffer);
    return NULL;
}

// Función de descompresión
int decompress_file(const char *input_file, const char *output_file, int threads) {
    FILE *input_fp = NULL, *output_fp = NULL;
    parzip_header_t header;
    block_info_t *block_infos = NULL;
    thread_data_t *thread_data = NULL;
    pthread_t *thread_ids = NULL;
    pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
    int error_flag = 0;
    int result = 0;
    
    printf("🔄 Iniciando descompresión paralela de archivos...\n");
    printf("📦 Archivo comprimido: %s\n", input_file);
    printf("📁 Archivo salida: %s\n", output_file);
    
    // Abrir archivo comprimido
    input_fp = fopen(input_file, "rb");
    if (!input_fp) {
        fprintf(stderr, "Error: No se pudo abrir el archivo comprimido: %s\n", input_file);
        return -1;
    }
    
    // Leer header
    if (read_parzip_header(input_fp, &header) != 0) {
        fprintf(stderr, "Error: No se pudo leer el header del archivo\n");
        result = -1;
        goto cleanup;
    }
    
    // Verificar número mágico
    if (header.magic != MAGIC_NUMBER) {
        fprintf(stderr, "Error: El archivo no es un archivo .pz válido (magic: 0x%lx)\n", header.magic);
        result = -1;
        goto cleanup;
    }
    
    printf("📊 Archivo original: %ld bytes\n", header.original_size);
    printf("🧩 Bloques: %d (tamaño: %d bytes)\n", header.num_blocks, header.block_size);
    printf("⚙️ Nivel compresión original: %d\n", header.compression_level);
    printf("🧵 Hilos: %d\n", threads);
    
    // Allocar memoria para información de bloques
    block_infos = calloc(header.num_blocks, sizeof(block_info_t));
    thread_data = calloc(threads, sizeof(thread_data_t));
    thread_ids = calloc(threads, sizeof(pthread_t));
    
    if (!block_infos || !thread_data || !thread_ids) {
        fprintf(stderr, "Error: No se pudo allocar memoria\n");
        result = -1;
        goto cleanup;
    }
    
    // Leer información de bloques
    for (uint32_t i = 0; i < header.num_blocks; i++) {
        if (read_parzip_block_info(input_fp, &block_infos[i]) != 0) {
            fprintf(stderr, "Error: No se pudo leer información del bloque %d\n", i);
            result = -1;
            goto cleanup;
        }
    }
    
    // Crear archivo de salida
    output_fp = fopen(output_file, "wb");
    if (!output_fp) {
        fprintf(stderr, "Error: No se pudo crear el archivo de salida: %s\n", output_file);
        result = -1;
        goto cleanup;
    }
    
    // Pre-allocar el archivo de salida al tamaño completo
    if (fseek(output_fp, header.original_size - 1, SEEK_SET) != 0 || fputc(0, output_fp) == EOF) {
        fprintf(stderr, "Error: No se pudo pre-allocar el archivo de salida\n");
        result = -1;
        goto cleanup;
    }
    rewind(output_fp);
    
    printf("\n🚀 Iniciando descompresión paralela...\n");
    
    // Procesar bloques con hilos
    uint32_t blocks_processed = 0;
    
    while (blocks_processed < header.num_blocks && !error_flag) {
        int active_threads = 0;
        
        // Lanzar hilos para procesar bloques
        for (int t = 0; t < threads && blocks_processed + t < header.num_blocks; t++) {
            uint32_t block_id = blocks_processed + t;
            
            thread_data[t].thread_id = t;
            thread_data[t].input_file = input_file;
            thread_data[t].output_file = output_file;
            thread_data[t].block_id = block_id;
            thread_data[t].block_size = header.block_size;
            thread_data[t].compression_level = header.compression_level;
            thread_data[t].output_mutex = &output_mutex;
            thread_data[t].output_fp = output_fp;
            thread_data[t].block_info = &block_infos[block_id];
            thread_data[t].error_flag = &error_flag;
            
            if (pthread_create(&thread_ids[t], NULL, decompress_block_thread, &thread_data[t]) != 0) {
                fprintf(stderr, "Error: No se pudo crear el hilo %d\n", t);
                error_flag = 1;
                break;
            }
            active_threads++;
        }
        
        // Esperar que terminen todos los hilos
        for (int t = 0; t < active_threads; t++) {
            pthread_join(thread_ids[t], NULL);
        }
        
        blocks_processed += active_threads;
    }
    
    if (error_flag) {
        fprintf(stderr, "❌ Error durante la descompresión\n");
        result = -1;
        goto cleanup;
    }
    
    // Truncar el archivo al tamaño exacto (en caso de que el último bloque sea menor)
    if (ftruncate(fileno(output_fp), header.original_size) != 0) {
        fprintf(stderr, "Advertencia: No se pudo truncar el archivo al tamaño exacto\n");
    }
    
    printf("\n✅ Descompresión completada exitosamente!\n");
    printf("📦 Archivo comprimido: %s\n", input_file);
    printf("📁 Archivo recuperado: %s (%ld bytes)\n", output_file, header.original_size);
    
cleanup:
    if (input_fp) fclose(input_fp);
    if (output_fp) fclose(output_fp);
    if (block_infos) free(block_infos);
    if (thread_data) free(thread_data);
    if (thread_ids) free(thread_ids);
    pthread_mutex_destroy(&output_mutex);
    
    return result;
}
