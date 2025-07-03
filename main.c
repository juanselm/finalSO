#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "compressor.h"
#include "utils.h"

void print_usage(const char *program_name) {
    printf("🗂️ ParZip - Compresor de Archivos Paralelo\n");
    printf("═══════════════════════════════════════════\n\n");
    printf("COMPRESIÓN:\n");
    printf("  %s -c [-t threads] [-b block_size] [-l level] <archivo_entrada> <archivo_salida.pz>\n\n", program_name);
    printf("DESCOMPRESIÓN:\n");
    printf("  %s -d [-t threads] <archivo_comprimido.pz> <archivo_salida>\n\n", program_name);
    printf("OPCIONES:\n");
    printf("  -c, --compress          Comprimir archivo\n");
    printf("  -d, --decompress        Descomprimir archivo\n");
    printf("  -t, --threads N         Número de hilos (por defecto: CPUs disponibles)\n");
    printf("  -b, --block-size N      Tamaño de bloque en bytes (por defecto: 64KB)\n");
    printf("  -l, --level N           Nivel de compresión 0-9 (por defecto: 6)\n");
    printf("  -h, --help              Mostrar esta ayuda\n");
    printf("  -v, --version           Mostrar versión\n\n");
    printf("EJEMPLOS:\n");
    printf("  %s -c archivo.txt archivo.pz\n", program_name);
    printf("  %s -c -t 8 -b 32768 -l 9 video.mp4 video.pz\n", program_name);
    printf("  %s -d archivo.pz archivo_recuperado.txt\n", program_name);
}

void print_version() {
    printf("ParZip v1.0.0 - Compresor de Archivos Paralelo\n");
    printf("Desarrollado para Sistemas Operativos - Universidad de Antioquia\n");
    printf("Basado en zlib con pthread para procesamiento paralelo\n");
}

void print_banner() {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    🗂️  PARZIP v1.0.0                         ║\n");
    printf("║              Compresor de Archivos Paralelo                 ║\n");
    printf("║                                                              ║\n");
    printf("║  📋 Funcionalidades:                                        ║\n");
    printf("║    ✅ Compresión paralela con múltiples hilos              ║\n");
    printf("║    ✅ División automática en bloques configurables         ║\n");
    printf("║    ✅ Algoritmo zlib con niveles de compresión 0-9         ║\n");
    printf("║    ✅ Formato .pz con header y metadatos                   ║\n");
    printf("║    ✅ Configuración automática basada en CPUs              ║\n");
    printf("║    ✅ Progreso visual y estadísticas detalladas           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

int main(int argc, char *argv[]) {
    // Variables por defecto
    int compress_mode = 0;
    int decompress_mode = 0;
    int threads = get_cpu_count();
    int block_size = DEFAULT_BLOCK_SIZE;
    int compression_level = Z_DEFAULT_COMPRESSION;
    char *input_file = NULL;
    char *output_file = NULL;
    
    // Definir opciones largas
    static struct option long_options[] = {
        {"compress",     no_argument,       0, 'c'},
        {"decompress",   no_argument,       0, 'd'},
        {"threads",      required_argument, 0, 't'},
        {"block-size",   required_argument, 0, 'b'},
        {"level",        required_argument, 0, 'l'},
        {"help",         no_argument,       0, 'h'},
        {"version",      no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    // Si no hay argumentos, mostrar ayuda
    if (argc == 1) {
        print_banner();
        print_usage(argv[0]);
        return 1;
    }
    
    // Procesar argumentos
    while ((c = getopt_long(argc, argv, "cdt:b:l:hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                compress_mode = 1;
                break;
            case 'd':
                decompress_mode = 1;
                break;
            case 't':
                threads = atoi(optarg);
                if (validate_threads(threads) != 0) {
                    return 1;
                }
                break;
            case 'b':
                block_size = atoi(optarg);
                if (validate_block_size(block_size) != 0) {
                    return 1;
                }
                break;
            case 'l':
                compression_level = atoi(optarg);
                if (validate_compression_level(compression_level) != 0) {
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case '?':
                fprintf(stderr, "Opción desconocida. Use -h para ayuda.\n");
                return 1;
            default:
                abort();
        }
    }
    
    // Verificar que se especificó modo de operación
    if (!compress_mode && !decompress_mode) {
        fprintf(stderr, "Error: Debe especificar -c (comprimir) o -d (descomprimir)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (compress_mode && decompress_mode) {
        fprintf(stderr, "Error: No puede especificar -c y -d al mismo tiempo\n");
        return 1;
    }
    
    // Verificar argumentos restantes (archivos)
    if (optind + 2 != argc) {
        fprintf(stderr, "Error: Debe especificar archivo de entrada y archivo de salida\n");
        print_usage(argv[0]);
        return 1;
    }
    
    input_file = argv[optind];
    output_file = argv[optind + 1];
    
    // Verificar que el archivo de entrada existe
    if (!file_exists(input_file)) {
        fprintf(stderr, "Error: El archivo de entrada '%s' no existe\n", input_file);
        return 1;
    }
    
    // Verificar que el archivo de salida no existe (para evitar sobrescribir)
    if (file_exists(output_file)) {
        printf("⚠️  El archivo de salida '%s' ya existe. ¿Sobrescribir? (s/N): ", output_file);
        char response;
        if (scanf(" %c", &response) != 1) {
            fprintf(stderr, "Error leyendo respuesta\n");
            return 1;
        }
        if (response != 's' && response != 'S') {
            printf("Operación cancelada.\n");
            return 0;
        }
    }
    
    print_banner();
    
    // Ejecutar operación
    int result;
    if (compress_mode) {
        result = compress_file(input_file, output_file, threads, block_size, compression_level);
    } else {
        result = decompress_file(input_file, output_file, threads);
    }
    
    if (result == 0) {
        printf("\n🎉 Operación completada exitosamente!\n");
    } else {
        printf("\n❌ La operación falló con código de error: %d\n", result);
    }
    
    return result;
}
