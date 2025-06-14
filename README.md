# Calculadora-de-Subredes
Este proyecto consiste en una calculadora de subredes desarrollada en C++, diseñada para simplificar y optimizar el proceso de segmentación de redes IP. Su principal objetivo es proporcionar una herramienta robusta y fácil de usar, capaz de asistir tanto a estudiantes como a profesionales de redes en la planificación de arquitecturas de redes.

Características Principales

El programa ha sido construido con un enfoque en la flexibilidad, la precisión y la usabilidad, incorporando las siguientes funcionalidades clave:

    Entrada de Red Flexible: Acepta la dirección de red base y su máscara en dos formatos comunes:
        Formato CIDR: Por ejemplo, 192.168.0.0/24.
        Formato Decimal: Por ejemplo, 192.168.0.0 - 255.255.255.0. El programa detecta automáticamente el formato utilizado por el usuario.

    Cálculo por Número de Hosts (VLSM): Permite al usuario especificar la cantidad exacta de hosts necesarios para cada subred individualmente. Esto es fundamental para la implementación de VLSM, ya que el programa calcula el prefijo CIDR más eficiente para cada solicitud, minimizando el desperdicio de direcciones IP.

    Asignación Optimizada: Para garantizar el uso más eficiente del espacio de direcciones IP disponible, el programa ordena automáticamente las subredes solicitadas de mayor a menor tamaño. Esta estrategia de asignación "First Fit" con subredes grandes primero ayuda a prevenir la fragmentación y asegura que las subredes más grandes tengan espacio contiguo.

    Validación de Entradas Robusta: Incluye un sistema exhaustivo de validación para asegurar la integridad de los datos de entrada:
        Verifica que el formato de las direcciones IP y máscaras sea correcto y que los valores de los octetos estén dentro del rango válido (0-255).
        Asegura que la máscara de subred sea una máscara binaria continua.
        Valida que la IP de red base proporcionada sea, de hecho, la dirección de red correspondiente a la máscara.
        Realiza una validación de capacidad total previa, sumando el espacio de IP requerido por todas las subredes solicitadas para confirmar que no excedan la capacidad de la red base antes de intentar la asignación.
        Notifica al usuario si alguna subred individual no puede ser asignada debido a la falta de espacio contiguo o si es una solicitud inviable.

    Formato de Salida Amigable: Los resultados de cada subred asignada se presentan en una tabla bien estructurada y alineada en la consola, facilitando su lectura y análisis. Para cada subred, se muestra la dirección de red, la máscara en CIDR, decimal y binario, el rango de hosts utilizables, la dirección de broadcast y el número de hosts disponibles y solicitados.

    Exportación Inteligente de Resultados: Ofrece la opción de exportar los resultados a un archivo. El programa detecta automáticamente la extensión del archivo proporcionado por el usuario:
        Si el archivo termina en .csv (ej. resultados.csv), los datos se formatean como valores separados por comas, ideales para importar directamente a hojas de cálculo como Excel.
