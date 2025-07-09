# ParZip - Compresor de Archivos Paralelo

Una implementación en C de un compresor de archivos que utiliza programación paralela con pthreads para dividir archivos en bloques y comprimirlos usando múltiples hilos.

## 🎯 Características Principales

### ✅ Funcionalidades Implementadas
- 🗜️ **Compresión paralela** con múltiples hilos de ejecución
- 📦 **Algoritmo zlib** con niveles de compresión configurables (0-9)
- 🧩 **División en bloques** de tamaño configurable (1KB - 16MB)
- ⚙️ **Configuración automática** basada en número de CPUs disponibles
- 🛡️ **Validación de argumentos** y manejo robusto de errores
- 📊 **Estadísticas detalladas** de compresión con progreso visual
- 📁 **Formato .pz** con header y metadatos para verificación

### 🎯 Arquitectura del Proyecto

**Archivos principales:**
- `main.c` - Interfaz de línea de comandos y manejo de argumentos
- `compressor.c` - Motor de compresión/descompresión paralela
- `compressor.h` - Definiciones y estructuras principales
- `utils.c` - Funciones auxiliares y de validación
- `utils.h` - Headers de utilidades
- `Makefile` - Script de compilación con múltiples targets

## 🚀 Instalación y Uso

### Requisitos
- GCC con soporte para C99
- Biblioteca zlib (`sudo apt-get install zlib1g-dev`)
- Biblioteca pthread (incluida en sistemas Unix/Linux)

### Compilación
```bash
make                 # Compilar el proyecto
make test           # Compilar y ejecutar pruebas
make install        # Instalar en el sistema (requiere sudo)
```

### Uso

**Compresión:**
```bash
./parzip -c archivo.txt archivo.pz
./parzip -c -t 8 -b 32768 -l 9 video.mp4 video.pz
```

**Descompresión:**
```bash
./parzip -d archivo.pz archivo_recuperado.txt
```

**Opciones disponibles:**
- `-c, --compress` - Modo compresión
- `-d, --decompress` - Modo descompresión
- `-t, --threads N` - Número de hilos (por defecto: CPUs disponibles)
- `-b, --block-size N` - Tamaño de bloque en bytes (por defecto: 64KB)
- `-l, --level N` - Nivel de compresión 0-9 (por defecto: 6)

## 🔧 Detalles Técnicos

### Formato de Archivo .pz
```
[Header: parzip_header_t]
[Tabla de bloques: block_info_t[]]
[Datos comprimidos de bloques]
```

### 📹 Video de Explicación
**Link del video:** [Próximamente]

### Algoritmo de Compresión
1. **División**: El archivo se divide en bloques de tamaño fijo
2. **Procesamiento paralelo**: Cada hilo toma un bloque y lo comprime con zlib
3. **Sincronización**: Mutex para escritura segura al archivo de salida
4. **Ensamblaje**: Los bloques comprimidos se organizan secuencialmente

### Estructuras Principales
- `parzip_header_t` - Header con metadatos del archivo
- `block_info_t` - Información de cada bloque comprimido
- `thread_data_t` - Datos pasados a cada hilo de trabajo

## 📊 Rendimiento

El compresor aprovecha múltiples cores para procesar archivos grandes de forma eficiente:
- **Paralelización**: N hilos procesan N bloques simultáneamente
- **Balanceamiento**: División equitativa de trabajo entre hilos
- **Optimización**: Configuración automática basada en hardware disponible

## 🧪 Pruebas

```bash
make test    # Ejecuta suite completa de pruebas
make clean   # Limpia archivos generados
make help    # Muestra comandos disponibles
```

## 📝 Desarrollo

Este proyecto fue desarrollado como trabajo final para la asignatura de Sistemas Operativos, implementando conceptos de:
- Programación paralela con pthreads
- Sincronización y exclusión mutua
- Manejo de archivos y E/O
- Compresión de datos con zlib
- Interfaz de línea de comandos

## Autores
- Juan Sebastian Loaiza Mazo
- Sulay Gisela Martínez Barreto

---

**Versión:** 1.0.0  
**Lenguaje:** C (C99)  
**Dependencias:** zlib, pthread  
**Licencia:** MIT
