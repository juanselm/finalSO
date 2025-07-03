# Makefile para ParZip - Compresor de Archivos Paralelo
CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread -std=c99
LDFLAGS=-lz -lpthread

# Nombre del ejecutable
TARGET=parzip

# Archivos fuente
SOURCES=main.c compressor.c utils.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=compressor.h utils.h

# Archivos de prueba
TEST_FILE=test_data.txt
COMPRESSED_FILE=test_data.pz
DECOMPRESSED_FILE=test_data_recovered.txt

.PHONY: all clean test install uninstall help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "🔗 Enlazando $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ $(TARGET) compilado exitosamente!"

%.o: %.c $(HEADERS)
	@echo "🔨 Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Crear archivo de prueba
$(TEST_FILE):
	@echo "📝 Creando archivo de prueba..."
	@echo "Este es un archivo de prueba para ParZip." > $(TEST_FILE)
	@echo "Contiene múltiples líneas de texto para probar la compresión." >> $(TEST_FILE)
	@echo "¡La compresión paralela debe funcionar correctamente!" >> $(TEST_FILE)
	@for i in $$(seq 1 100); do echo "Línea de prueba número $$i con datos repetitivos para compresión." >> $(TEST_FILE); done

# Ejecutar pruebas
test: $(TARGET) $(TEST_FILE)
	@echo "🧪 Ejecutando pruebas..."
	@echo "\n📦 Prueba de compresión:"
	./$(TARGET) -c -t 4 -b 1024 $(TEST_FILE) $(COMPRESSED_FILE)
	@echo "\n📊 Comparando tamaños:"
	@ls -lh $(TEST_FILE) $(COMPRESSED_FILE)
	@echo "\n🔄 Prueba de descompresión:"
	./$(TARGET) -d $(COMPRESSED_FILE) $(DECOMPRESSED_FILE)
	@echo "\n✅ Verificando integridad:"
	@if diff $(TEST_FILE) $(DECOMPRESSED_FILE) > /dev/null; then \
		echo "✅ ¡Prueba exitosa! Los archivos son idénticos."; \
	else \
		echo "❌ Error: Los archivos no coinciden."; \
		exit 1; \
	fi

# Prueba rápida solo de compilación
compile-test: $(TARGET)
	@echo "✅ Compilación exitosa"

# Instalar en el sistema (requiere permisos de administrador)
install: $(TARGET)
	@echo "📦 Instalando $(TARGET)..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "✅ $(TARGET) instalado en /usr/local/bin/"

# Desinstalar del sistema
uninstall:
	@echo "🗑️  Desinstalando $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ $(TARGET) desinstalado"

# Mostrar ayuda
help:
	@echo "🗂️ ParZip - Makefile"
	@echo "════════════════════"
	@echo "Comandos disponibles:"
	@echo "  make              - Compilar el proyecto"
	@echo "  make test         - Compilar y ejecutar pruebas"
	@echo "  make compile-test - Solo verificar compilación"
	@echo "  make install      - Instalar en el sistema"
	@echo "  make uninstall    - Desinstalar del sistema"
	@echo "  make clean        - Limpiar archivos generados"
	@echo "  make help         - Mostrar esta ayuda"

# Limpiar archivos generados
clean:
	@echo "🧹 Limpiando archivos..."
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(TEST_FILE) $(COMPRESSED_FILE) $(DECOMPRESSED_FILE)
	@echo "✅ Limpieza completada"

# Información del sistema
info:
	@echo "🖥️  Información del sistema:"
	@echo "Compilador: $(CC) $$($(CC) --version | head -1)"
	@echo "CPUs disponibles: $$(nproc)"
	@echo "Memoria: $$(free -h | grep Mem | awk '{print $$2}')"
	@echo "Sistema: $$(uname -a)"
