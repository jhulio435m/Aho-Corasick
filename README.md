# Aho-Corasick

Este proyecto implementa un buscador de patrones basado en el algoritmo de Aho-Corasick. Permite analizar un texto para encontrar todas las ocurrencias de una lista de patrones.

## Compilación

Compile el programa con `g++` ejecutando:

```bash
g++ -std=c++17 PatternMatcher.cpp ui.cpp main.cpp -o proyecto
```

## Ejecución

Una vez compilado, ejecute el binario resultante:

```bash
./proyecto
```

Se mostrará un menú interactivo donde podrá:

- Cargar patrones desde un archivo o ingresarlos manualmente.
- Cargar el texto a analizar desde un archivo o introducirlo por consola.
- Configurar opciones de búsqueda (modo verboso, sensibilidad a mayúsculas y tamaño de contexto).
- Ejecutar la búsqueda de patrones y visualizar los resultados.
- Generar un resumen estadístico de las coincidencias encontradas.
- Exportar los resultados detallados a un archivo HTML.

