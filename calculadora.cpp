#include <iostream>  // Para entrada/salida de consola (cout, cin)
#include <vector>    // Para usar std::vector
#include <string>    // Para usar std::string
#include <cmath>     // Para usar log2 y pow
#include <sstream>   // Para usar std::istringstream para dividir cadenas
#include <limits>    // Para numeric_limits, útil en la validación de entrada
#include <cstdint>   // NECESARIO para uint32_t y otros tipos enteros de ancho fijo
#include <algorithm> // Para std::sort y std::transform
#include <iomanip>   // Para std::setw, std::left, std::right para formato de salida
#include <fstream>   // Para std::ofstream para escribir en archivos

// Usar el espacio de nombres estándar para simplificar el código.
// En proyectos más grandes, se prefiere calificar con std::
using namespace std;

// Forward declaration for intToIp, as it's used by other functions early
string intToIp(uint32_t ip);
string uint32_tToBinaryString(uint32_t n);

// Estructura para almacenar los detalles de cada subred.
struct Subred {
    string direccionRed;
    string mascaraDecimal;
    string mascaraBinaria; // Nueva: para la representación binaria
    int cidr;
    string hostRangeStart;
    string hostRangeEnd;
    string broadcast;
    int hostsUtilizables;
    int requestedHosts; // Para mantener el recuento original de hosts solicitados
};

/**
 * @brief Divide una cadena de texto en un vector de cadenas usando un delimitador.
 * @param s La cadena a dividir.
 * @param delim El carácter delimitador.
 * @return Un vector de cadenas.
 */
vector<string> split(const string& s, char delim) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * @brief Convierte una dirección IP en formato dotted decimal (ej. "192.168.1.1") a un entero sin signo de 32 bits.
 * @param ip La dirección IP en formato de cadena.
 * @return La dirección IP como un entero de 32 bits. Retorna 0 si hay un error de formato (excepto para "0.0.0.0").
 */
uint32_t ipToInt(const string& ip) {
    vector<string> octetos = split(ip, '.');
    if (octetos.size() != 4) {
        return 0; // Formato inválido (no 4 octetos)
    }
    uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        try {
            int octet_val = stoi(octetos[i]);
            if (octet_val < 0 || octet_val > 255) {
                return 0; // Octeto fuera de rango
            }
            result |= static_cast<uint32_t>(octet_val) << (24 - 8 * i);
        } catch (const invalid_argument& e) {
            return 0; // No es un número
        } catch (const out_of_range& e) {
            return 0; // Número fuera de rango
        }
    }
    return result;
}

/**
 * @brief Convierte un entero sin signo de 32 bits a una dirección IP en formato dotted decimal.
 * @param ip La dirección IP como un entero de 32 bits.
 * @return La dirección IP en formato de cadena.
 */
string intToIp(uint32_t ip) {
    return to_string((ip >> 24) & 0xFF) + "." +
           to_string((ip >> 16) & 0xFF) + "." +
           to_string((ip >> 8) & 0xFF) + "." +
           to_string(ip & 0xFF);
}

/**
 * @brief Convierte un entero sin signo de 32 bits a una cadena binaria con puntos cada 8 bits.
 * @param n El entero de 32 bits.
 * @return La cadena binaria formateada.
 */
string uint32_tToBinaryString(uint32_t n) {
    string binaryString;
    for (int i = 31; i >= 0; --i) {
        binaryString += ((n >> i) & 1) ? '1' : '0';
        if (i % 8 == 0 && i != 0) {
            binaryString += '.';
        }
    }
    return binaryString;
}

/**
 * @brief Convierte una máscara decimal (ej. "255.255.255.0") a un prefijo CIDR (ej. 24).
 * @param maskDecimal La máscara en formato de cadena dotted decimal.
 * @return El prefijo CIDR. Retorna -1 si la máscara no es válida.
 */
int maskToCidr(const string& maskDecimal) {
    uint32_t maskInt = ipToInt(maskDecimal);
    if (maskInt == 0 && maskDecimal != "0.0.0.0") { 
        return -1; // Error en la conversión IP a Int para la máscara
    }

    int cidr = 0;
    bool sawZero = false;
    for (int i = 31; i >= 0; --i) { // Recorre los bits de izquierda a derecha (MSB a LSB)
        if ((maskInt >> i) & 1) { // Si el bit actual es 1
            if (sawZero) {
                return -1; // Máscara inválida (un 1 después de un 0, ej. 255.0.255.0)
            }
            cidr++;
        } else { // Si el bit actual es 0
            sawZero = true;
        }
    }
    return cidr;
}


/**
 * @brief Convierte un prefijo CIDR (ej. 24) a una máscara de subred en formato dotted decimal (ej. "255.255.255.0").
 * @param cidr El prefijo CIDR.
 * @return La máscara de subred en formato de cadena.
 */
string cidrToMask(int cidr) {
    uint32_t mask = (cidr == 0) ? 0 : (~static_cast<uint32_t>(0) << (32 - cidr));
    return intToIp(mask);
}

/**
 * @brief Calcula el prefijo CIDR más pequeño que puede contener un número dado de hosts.
 * @param num_hosts El número de hosts deseado.
 * @return El prefijo CIDR correspondiente. Retorna -1 si no es posible (ej. num_hosts muy grande).
 */
int hostsToCidr(int num_hosts) {
    if (num_hosts < 0) return -1; // Número de hosts negativo no válido

    if (num_hosts == 0) return 32; // /32 tiene 1 IP, 0 hosts utilizables
    
    int required_addresses = num_hosts + 2;

    int bits_for_addresses = static_cast<int>(ceil(log2(required_addresses)));

    int cidr = 32 - bits_for_addresses;

    if (cidr < 0) return 0;   // Si es menor que 0, significa una subred muy grande, usa /0
    if (cidr > 32) return 32; // Esto no debería ocurrir con la lógica, pero como seguridad

    return cidr;
}

/**
 * @brief Imprime el encabezado de la tabla de subredes, para consola o CSV.
 * @param os Flujo de salida.
 * @param is_csv Verdadero si se imprime en CSV, falso para consola.
 */
void printSubnetHeader(ostream& os, bool is_csv) {
    if (is_csv) {
        os << "Numero,Red,CIDR,MascaraDecimal,MascaraBinaria,RangoInicio,RangoFin,Broadcast,HostsDisponibles,HostsSolicitados\n";
    } else {
        const int COL_WIDTH_NUM = 5;
        const int COL_WIDTH_NETWORK = 18;
        const int COL_WIDTH_CIDR = 8;
        const int COL_WIDTH_MASK_DEC = 18;
        const int COL_WIDTH_MASK_BIN = 37;
        const int COL_WIDTH_RANGE_START = 18;
        const int COL_WIDTH_RANGE_END = 18;
        const int COL_WIDTH_BROADCAST = 18;
        const int COL_WIDTH_HOSTS = 10;
        const int COL_WIDTH_REQ_HOSTS = 12;

        os << left << setw(COL_WIDTH_NUM) << "#"
           << setw(COL_WIDTH_NETWORK) << "Red"
           << setw(COL_WIDTH_CIDR) << "CIDR"
           << setw(COL_WIDTH_MASK_DEC) << "Máscara (Dec)"
           << setw(COL_WIDTH_MASK_BIN) << "Máscara (Bin)"
           << setw(COL_WIDTH_RANGE_START) << "Rango Inicio"
           << setw(COL_WIDTH_RANGE_END) << "Rango Fin"
           << setw(COL_WIDTH_BROADCAST) << "Broadcast"
           << setw(COL_WIDTH_HOSTS) << "Hosts Disp."
           << setw(COL_WIDTH_REQ_HOSTS) << "Hosts Sol." << endl;
        os << string(COL_WIDTH_NUM + COL_WIDTH_NETWORK + COL_WIDTH_CIDR + COL_WIDTH_MASK_DEC + COL_WIDTH_MASK_BIN +
                     COL_WIDTH_RANGE_START + COL_WIDTH_RANGE_END + COL_WIDTH_BROADCAST + COL_WIDTH_HOSTS + COL_WIDTH_REQ_HOSTS, '-') << endl;
    }
}

/**
 * @brief Imprime los detalles de una única subred, para consola o CSV.
 * @param os Flujo de salida.
 * @param subnet El objeto Subred que contiene los detalles.
 * @param is_csv Verdadero si se imprime en CSV, falso para consola.
 * @param counter El número de la subred.
 */
void printSubnetRow(ostream& os, const Subred& subnet, bool is_csv, int counter) {
    if (is_csv) {
        os << counter << ","
           << subnet.direccionRed << ","
           << "/" << subnet.cidr << ","
           << subnet.mascaraDecimal << ","
           << subnet.mascaraBinaria << ","
           << subnet.hostRangeStart << ","
           << subnet.hostRangeEnd << ","
           << subnet.broadcast << ","
           << subnet.hostsUtilizables << ","
           << subnet.requestedHosts << "\n";
    } else {
        const int COL_WIDTH_NUM = 5;
        const int COL_WIDTH_NETWORK = 18;
        const int COL_WIDTH_CIDR = 8;
        const int COL_WIDTH_MASK_DEC = 18;
        const int COL_WIDTH_MASK_BIN = 37;
        const int COL_WIDTH_RANGE_START = 18;
        const int COL_WIDTH_RANGE_END = 18;
        const int COL_WIDTH_BROADCAST = 18;
        const int COL_WIDTH_HOSTS = 10;
        const int COL_WIDTH_REQ_HOSTS = 12;

        os << left << setw(COL_WIDTH_NUM) << counter
           << setw(COL_WIDTH_NETWORK) << subnet.direccionRed
           << setw(COL_WIDTH_CIDR) << "/" + to_string(subnet.cidr)
           << setw(COL_WIDTH_MASK_DEC) << subnet.mascaraDecimal
           << setw(COL_WIDTH_MASK_BIN) << subnet.mascaraBinaria
           << setw(COL_WIDTH_RANGE_START) << subnet.hostRangeStart
           << setw(COL_WIDTH_RANGE_END) << subnet.hostRangeEnd
           << setw(COL_WIDTH_BROADCAST) << subnet.broadcast
           << setw(COL_WIDTH_HOSTS) << subnet.hostsUtilizables
           << setw(COL_WIDTH_REQ_HOSTS) << subnet.requestedHosts << endl;
    }
}

/**
 * @brief Calcula y muestra/exporta las subredes solicitadas.
 * Asigna subredes de mayor tamaño a menor tamaño para optimizar el espacio.
 * Los resultados se imprimen en una tabla formateada y pueden exportarse a un archivo.
 * @param os Flujo de salida (ej. cout o un ofstream).
 * @param ipBaseStr La dirección IP base de la red (como string para mensajes de error).
 * @param cidrBase El prefijo CIDR de la red base.
 * @param requestedHostCounts Un vector de números de hosts solicitados para las nuevas subredes.
 * @param is_csv_output Verdadero si la salida debe estar en formato CSV.
 */
void calcularSubredes(ostream& os, string ipBaseStr, int cidrBase, vector<int> requestedHostCounts, bool is_csv_output) {
    uint32_t ipNumericaBase = ipToInt(ipBaseStr);
    uint32_t mascaraBaseNumerica = (~static_cast<uint32_t>(0) << (32 - cidrBase));
    
    uint32_t originalNetworkAddress = ipNumericaBase & mascaraBaseNumerica;
    uint32_t originalBroadcastAddress = originalNetworkAddress | (~mascaraBaseNumerica);
    uint32_t totalOriginalIps = originalBroadcastAddress - originalNetworkAddress + 1;

    if (originalNetworkAddress != ipNumericaBase) {
        os << "Error: La dirección IP de entrada '" << ipBaseStr << "' no es una dirección de red válida para la máscara /" << cidrBase << ".\n";
        os << "La dirección de red correcta para esta IP y máscara sería: " << intToIp(originalNetworkAddress) << endl;
        return;
    }
    
    if (!is_csv_output) { // Solo imprime esta información en consola, no en CSV
        os << "\n--- Resultados de Subneteo ---\n";
        os << "Red Original: " << intToIp(originalNetworkAddress) << "/" << cidrBase 
           << " (Broadcast: " << intToIp(originalBroadcastAddress) << ", IPs Totales: " 
           << totalOriginalIps << ")\n\n";
    }

    vector<pair<int, int>> subnetsToAllocateInfo; // par<CIDR, hosts_solicitados_originales>
    uint32_t totalRequestedIpSpace = 0; // Para la validación previa de capacidad

    for (int hosts : requestedHostCounts) {
        int cidr = hostsToCidr(hosts);
        if (cidr != -1) { // -1 indica un número de hosts inválido
            uint32_t subnetSize = static_cast<uint32_t>(pow(2, 32 - cidr));
            totalRequestedIpSpace += subnetSize;
            subnetsToAllocateInfo.push_back({cidr, hosts});
        } else {
            os << "Advertencia: El número de hosts solicitado (" << hosts << ") es inválido o demasiado grande. Ignorando.\n";
        }
    }

    // Validación previa: Comprobar si el total de IPs solicitadas excede la capacidad de la red original
    if (totalRequestedIpSpace > totalOriginalIps) {
        os << "\nError: La suma de IPs requeridas por las subredes solicitadas (" << totalRequestedIpSpace 
           << ") excede la capacidad total de la red original (" << totalOriginalIps << ").\n";
        os << "No se pueden asignar estas subredes. Por favor, revise sus solicitudes de hosts o use una red base más grande.\n";
        return;
    }

    // Ordena las subredes a asignar. Queremos asignar las subredes más grandes primero.
    // Un CIDR más pequeño significa una subred más grande.
    // Por lo tanto, ordenamos por CIDR en orden ascendente.
    sort(subnetsToAllocateInfo.begin(), subnetsToAllocateInfo.end(), 
         [](const pair<int, int>& a, const pair<int, int>& b) {
             return a.first < b.first; // Ordena por CIDR de menor a mayor
         });

    uint32_t currentAllocationIp = originalNetworkAddress;
    int subnetCounter = 0;
    
    printSubnetHeader(os, is_csv_output); // Imprime el encabezado de la tabla/CSV

    for (const auto& subnetInfo : subnetsToAllocateInfo) {
        int requestedCidr = subnetInfo.first;
        int originalHostsRequested = subnetInfo.second;

        // Si el CIDR solicitado es mayor que 32 o menor que 0 (casos inválidos)
        if (requestedCidr > 32 || requestedCidr < 0) { 
             Subred currentSubnet; // Crea una subred ficticia para imprimir la advertencia
             currentSubnet.direccionRed = "INVALIDO";
             currentSubnet.cidr = requestedCidr;
             currentSubnet.mascaraDecimal = "INVALIDO";
             currentSubnet.mascaraBinaria = "INVALIDO";
             currentSubnet.hostRangeStart = "N/A";
             currentSubnet.hostRangeEnd = "N/A";
             currentSubnet.broadcast = "N/A";
             currentSubnet.hostsUtilizables = 0;
             currentSubnet.requestedHosts = originalHostsRequested;
             
             subnetCounter++;
             if (!is_csv_output) os << "Advertencia: "; // Prefijo de advertencia solo en consola
             printSubnetRow(os, currentSubnet, is_csv_output, subnetCounter);
             if (!is_csv_output) os << "  (Subred solicitada para " << originalHostsRequested << " hosts (/" << requestedCidr << ") es inviable o inválida. Saltando.)\n";
             continue;
        }

        uint32_t requestedSubnetSize = static_cast<uint32_t>(pow(2, 32 - requestedCidr));
        uint32_t potentialSubnetBroadcast = currentAllocationIp + requestedSubnetSize - 1;
        
        // Verifica si la subred solicitada puede ser asignada en el espacio actual
        // y si su dirección de broadcast no excede la de la red original.
        if (potentialSubnetBroadcast > originalBroadcastAddress || currentAllocationIp > originalBroadcastAddress) {
            Subred currentSubnet; // Crea una subred ficticia para imprimir la advertencia
            currentSubnet.direccionRed = "SIN ESPACIO";
            currentSubnet.cidr = requestedCidr;
            currentSubnet.mascaraDecimal = "N/A";
            currentSubnet.mascaraBinaria = "N/A";
            currentSubnet.hostRangeStart = "N/A";
            currentSubnet.hostRangeEnd = "N/A";
            currentSubnet.broadcast = "N/A";
            currentSubnet.hostsUtilizables = 0;
            currentSubnet.requestedHosts = originalHostsRequested;
            
            subnetCounter++;
            if (!is_csv_output) os << "Advertencia: "; // Prefijo de advertencia solo en consola
            printSubnetRow(os, currentSubnet, is_csv_output, subnetCounter);
            if (!is_csv_output) os << "  (Subred para " << originalHostsRequested << " hosts (/" << requestedCidr 
                 << ") NO CABE. IPs restantes: " << (originalBroadcastAddress - currentAllocationIp + 1) << ".)\n";
            continue; 
        }

        subnetCounter++;
        int hostsPorSubredCalculado = (requestedSubnetSize > 2) ? (requestedSubnetSize - 2) : 0;

        Subred currentSubnet;
        currentSubnet.direccionRed = intToIp(currentAllocationIp);
        currentSubnet.cidr = requestedCidr;
        currentSubnet.mascaraDecimal = cidrToMask(requestedCidr);
        currentSubnet.mascaraBinaria = uint32_tToBinaryString(ipToInt(currentSubnet.mascaraDecimal));
        currentSubnet.hostsUtilizables = hostsPorSubredCalculado;
        currentSubnet.requestedHosts = originalHostsRequested;

        if (hostsPorSubredCalculado > 0) {
            currentSubnet.hostRangeStart = intToIp(currentAllocationIp + 1);
            currentSubnet.hostRangeEnd = intToIp(potentialSubnetBroadcast - 1);
            currentSubnet.broadcast = intToIp(potentialSubnetBroadcast);
        } else {
            currentSubnet.hostRangeStart = "N/A";
            currentSubnet.hostRangeEnd = "N/A";
            currentSubnet.broadcast = intToIp(potentialSubnetBroadcast);
        }
        
        printSubnetRow(os, currentSubnet, is_csv_output, subnetCounter);
        
        currentAllocationIp += requestedSubnetSize; // Avanza al siguiente bloque disponible
    }
    
    if (!is_csv_output) { // Solo imprime esta información en consola, no en CSV
        os << endl;
        if (currentAllocationIp <= originalBroadcastAddress) {
            os << "Espacio remanente sin utilizar: " << intToIp(currentAllocationIp) << " - " << intToIp(originalBroadcastAddress) << endl;
            os << "Total de IPs remanentes: " << (originalBroadcastAddress - currentAllocationIp + 1) << endl;
        } else {
            os << "Toda la red ha sido utilizada o las solicitudes excedieron su capacidad.\n";
        }
        os << "-------------------------------------------\n";
    }
}

int main() {
    cout << "Bienvenido a la Calculadora de Subredes!\n\n";

    string entradaRed;
    cout << "Ingrese la dirección IP de la RED y la máscara (ej. 192.168.0.0/24 O 192.168.0.0 - 255.255.255.0): ";
    getline(cin, entradaRed);

    // Eliminar espacios al inicio/final de la cadena de entrada
    entradaRed.erase(0, entradaRed.find_first_not_of(" \t\n\r\f\v"));
    entradaRed.erase(entradaRed.find_last_not_of(" \t\n\r\f\v") + 1);

    string ipBaseStr;
    int cidrBase = -1; // Valor predeterminado para CIDR inválido

    // Intentar analizar la entrada como formato IP/CIDR primero
    size_t slash_pos = entradaRed.find('/');
    if (slash_pos != string::npos) {
        ipBaseStr = entradaRed.substr(0, slash_pos);
        string cidrStr = entradaRed.substr(slash_pos + 1);
        try {
            cidrBase = stoi(cidrStr);
            if (cidrBase < 0 || cidrBase > 32) {
                cout << "Error: El prefijo CIDR debe estar entre 0 y 32.\n";
                #ifdef _WIN32
                    system("pause"); 
                #endif
                return 1;
            }
        } catch (const invalid_argument& e) {
            cout << "Error: El prefijo CIDR no es un número válido.\n";
            #ifdef _WIN32
                system("pause"); 
            #endif
            return 1;
        } catch (const out_of_range& e) {
            cout << "Error: El prefijo CIDR está fuera de rango.\n";
            #ifdef _WIN32
                system("pause"); 
            #endif
            return 1;
        }
    } else { // Si no es IP/CIDR, intenta el formato IP - Máscara decimal
        size_t dash_pos = entradaRed.find(" - ");
        if (dash_pos == string::npos) {
            cout << "Error: Formato de entrada de red inválido. Use 'IP/CIDR' o 'IP - Máscara Decimal'.\n";
            #ifdef _WIN32
                system("pause"); 
            #endif
            return 1;
        }
        ipBaseStr = entradaRed.substr(0, dash_pos);
        string maskDecimalStr = entradaRed.substr(dash_pos + 3); // +3 para saltar " - "
        cidrBase = maskToCidr(maskDecimalStr);
        if (cidrBase == -1) {
            cout << "Error: La máscara de subred ('" << maskDecimalStr << "') no es válida o no es una máscara binaria continua.\n";
            #ifdef _WIN32
                system("pause"); 
            #endif
            return 1;
        }
    }

    // Validar el formato de la dirección IP base por sí misma, independientemente del estilo de entrada
    uint32_t ipBaseNum = ipToInt(ipBaseStr);
    if (ipBaseNum == 0 && ipBaseStr != "0.0.0.0") { 
        cout << "Error: La dirección IP de base ('" << ipBaseStr << "') no es válida o está fuera de rango.\n";
        #ifdef _WIN32
            system("pause"); 
        #endif
        return 1;
    }

    int numSubnetsToDefine;
    cout << "\nIngrese la cantidad de subredes que desea definir: ";
    // Bucle para validación de entrada numérica para la cantidad de subredes
    while (!(cin >> numSubnetsToDefine) || numSubnetsToDefine <= 0) {
        cout << "Entrada inválida. Por favor, ingrese un número entero positivo para la cantidad de subredes: ";
        cin.clear(); 
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    }
    // Consumir el resto de la línea después de leer el entero para que el siguiente getline funcione correctamente
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

    vector<int> requestedHostCounts;
    for (int i = 0; i < numSubnetsToDefine; ++i) {
        int hosts;
        cout << "Ingrese el número de hosts para la subred " << (i + 1) << ": ";
        while (!(cin >> hosts) || hosts < 0) { // Los hosts pueden ser 0 para una /32
            cout << "Entrada inválida. Por favor, ingrese un número entero no negativo para los hosts: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        requestedHostCounts.push_back(hosts);
        // Consumir el resto de la línea después de leer el entero
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    }

    // Calcular y mostrar en consola
    calcularSubredes(cout, ipBaseStr, cidrBase, requestedHostCounts, false);

    string exportOption;
    cout << "\n¿Desea exportar los resultados a un archivo? (s/n): ";
    getline(cin, exportOption);
    // Convertir a minúsculas para una comparación insensible a mayúsculas/minúsculas
    transform(exportOption.begin(), exportOption.end(), exportOption.begin(), ::tolower);

    if (exportOption == "s" || exportOption == "si") {
        string filename;
        cout << "Ingrese el nombre del archivo (ej. resultados.txt o resultados.csv): ";
        getline(cin, filename);

        // Determinar si la exportación debe ser CSV
        bool is_csv = (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".csv");

        ofstream outFile(filename);
        if (outFile.is_open()) {
            calcularSubredes(outFile, ipBaseStr, cidrBase, requestedHostCounts, is_csv);
            outFile.close();
            cout << "Resultados exportados exitosamente a '" << filename << "'.\n";
        } else {
            cout << "Error: No se pudo abrir el archivo para escribir. Verifique los permisos o la ruta.\n";
        }
    }

    cout << "\n--- Fin de la Calculadora de Subredes ---\n";
    #ifdef _WIN32
        system("pause"); 
    #endif

    return 0;
}


