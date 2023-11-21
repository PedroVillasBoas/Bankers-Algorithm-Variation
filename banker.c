#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 50

// Declaração das Funções
int bankerAlgorithm(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources);
int checkSafety(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, int *safeSequence);
void requestResources(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, int customerID, int *requestedResources, FILE *outputFile);
void releaseResources(int **currentAllocation, int **remainingNeed, int *availableResources, int customerID, int numberOfResources, int *resourcesToRelease, FILE *outputFile);
void printAllMatrices(FILE *filePointer, int rows, int cols);
int readCustomerMaximumDemand(const char *filename, int numberOfCustomers, int numberOfResources);
int processBankerCommands(const char *filename, int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, FILE *outputFile);
int countNumberOfCustomers(const char *filename);
int** allocate2DMatrix(int rows, int cols);
void free2DMatrix(int **matrix, int rows);

// Variáveis Globais
int **maximumDemand;     // Matriz de demanda máxima
int **currentAllocation; // Matriz de alocação atual
int **remainingNeed;     // Matriz de necessidade restante
int cmdLineResources;    // Número de recursos passados na linha de comando

int main(int argc, char *argv[]) 
{
    int *availableResources;          // Recursos disponíveis 
    int numberOfCustomers;            // Número de clientes que vão estar presentes no arquivo customer.txt
    int numberOfResources = argc - 1; // O primeiro argumento é o nome do programa, logo não conta
    cmdLineResources = argc - 1;     // Pega a quantidade de recursos passados na linha de comando pra fazer verificação de erro

    // Aloca memória para recursos disponíveis
    availableResources = (int *)malloc(numberOfResources * sizeof(int));
    for (int i = 0; i < numberOfResources; i++) 
    {
        availableResources[i] = atoi(argv[i + 1]); // atoi vai converter o argumento para inteiro
    }

    // Lendo o número de clientes
    numberOfCustomers = countNumberOfCustomers("customer.txt");
    if (numberOfCustomers == 0) 
    {
        printf("Fail to read customer.txt\n");
        free(availableResources);
        return 1;
    }

    // Alocando memória para as matrizes
    currentAllocation = allocate2DMatrix(numberOfCustomers, numberOfResources); // Aloca a matriz de alocação atual
    maximumDemand = allocate2DMatrix(numberOfCustomers, numberOfResources);     // Aloca a matriz de demanda máxima
    remainingNeed = allocate2DMatrix(numberOfCustomers, numberOfResources);     // Aloca a matriz de necessidade restante

    // Lendo as demandas máximas dos clientes
    if (!readCustomerMaximumDemand("customer.txt", numberOfCustomers, numberOfResources)) 
    {
        printf("Incompatibility between customer.txt and command line\n");
        goto cleanup;
    }

    // Calcula a necessidade restante de cada cliente
    for (int i = 0; i < numberOfCustomers; i++) 
    {
        for (int j = 0; j < numberOfResources; j++) 
        {
            remainingNeed[i][j] = maximumDemand[i][j] - currentAllocation[i][j]; // Necessidade restante = Demanda máxima - Alocação atual
        }
    }

    // Abre o arquivo de saída
    FILE *outputFile = fopen("result.txt", "w");
    if (!outputFile) 
    {
        printf("Error: Unable to open result.txt for writing\n");
        goto cleanup;
    }

    // Executa os comando do arquivo commands.txt
    if (!processBankerCommands("commands.txt", currentAllocation, remainingNeed, availableResources, numberOfCustomers, numberOfResources, outputFile)) 
    {
        printf("Incompatibility between commands.txt and command line\n");
        fclose(outputFile);
        goto cleanup;
    }

    // Fecha o arquivo
    fclose(outputFile);

// Libera a memória alocada em caso de erro
cleanup:
    free2DMatrix(currentAllocation, numberOfCustomers);
    free2DMatrix(maximumDemand, numberOfCustomers);
    free2DMatrix(remainingNeed, numberOfCustomers);
    free(availableResources);
    return 0;
}

// Lê a demanda máxima dos clientes do arquivo customer.txt
int readCustomerMaximumDemand(const char *filename, int numberOfCustomers, int numberOfResources) 
{
    // Abre o arquivo
    FILE *file = fopen(filename, "r");
    if (!file)
    {
      return 0;  
    } 

    char line[100];          // Tamanho máximo de uma linha do arquivo
    int currentCustomer = 0; // Rastreia o número de clientes processados

    // Lê cada linha do arquivo até chegar no final do arquivo ou até processar o numero de clientes 
    while (fgets(line, sizeof(line), file) != NULL && currentCustomer < numberOfCustomers) 
    {
        int maxRequests[numberOfResources]; // Vetor para armazenar a demanda máxima de cada cliente
        char *token = strtok(line, ",");    // Divide a linha em tokens separados por vírgula

        // Lê cada token e converte para inteiro
        for (int i = 0; i < numberOfResources; i++) 
        {
            // Se o token for nulo, o arquivo está mal formatado ou não tem o número de recursos esperado 
            // || Se o número de recursos passados na linha de comando for maior que o número de recursos esperado
            if (token == NULL || i >= cmdLineResources)
            {
                fclose(file);
                return 0;
            }
            maxRequests[i] = atoi(token);                       // Converte o token para inteiro (O pedido máximo de um recurso para um cliente)
            token = strtok(NULL, ",");                          // Pega o próximo token
            maximumDemand[currentCustomer][i] = maxRequests[i]; // Armazena o pedido máximo de um recurso para um cliente
        }
        currentCustomer++; // Incrementa o número de clientes processados, indicando que o cliente atual foi processado com sucesso
    }

    fclose(file);
    return currentCustomer == numberOfCustomers; // Retorna 1 se o número de clientes processados for igual ao número de clientes esperados e 0 caso contrário
}

// Processa os comandos do arquivo commands.txt, lidando com a alocação e liberação de recursos baseado nos comandos presentes no arquivo
int processBankerCommands(const char *filename, int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, FILE *outputFile) 
{
    // Abre o arquivo
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return 0;
    } 

    char line[MAX_COMMAND_LENGTH];                  // Tamanho máximo de uma linha do arquivo
    while (fgets(line, sizeof(line), file) != NULL) // Lê cada linha do arquivo até chegar no final do arquivo
    {
        char command[3];                  // Guarda o comando a ser executado (RQ ou RL)
        int customerID;                   // Guarda o ID do cliente a ser alocado ou liberado
        int resources[numberOfResources]; // Guarda os recursos a serem alocados ou liberados

        // Se a linha for igual a *, imprime as matrizes e os recursos disponíveis
        if (strcmp(line, "*\n") == 0)     
        {
            printAllMatrices(outputFile, numberOfCustomers, numberOfResources); // Imprime as matrizes no arquivo
            fprintf(outputFile, "AVAILABLE ");                                  
            for (int i = 0; i < numberOfResources; i++) // Imprime os recursos disponíveis no arquivo
            {
                fprintf(outputFile, "%d ", availableResources[i]);
            }
            fprintf(outputFile, "\n");
            continue;
        }

        if (sscanf(line, "%2s %d", command, &customerID) < 2) // Lê o comando e o ID do cliente, se não conseguir, o arquivo está mal formatado
        {
            fclose(file);
            return 0;
        }

        char *token = strtok(line, " "); // Divide a linha em tokens separados por espaço
        token = strtok(NULL, " ");       // Pula o token do comando
        token = strtok(NULL, " ");       // Pula o token do customerID

        // Lê os recursos a serem alocados ou liberados
        for (int i = 0; i < numberOfResources && token != NULL; i++) 
        {
            if (i >= cmdLineResources) // Se o número de recursos passados na linha de comando for maior que o número de recursos esperado
            {
                fclose(file);
                return 0;
            }
            resources[i] = atoi(token); // E guarda no vetor de recursos como inteiro
            token = strtok(NULL, " ");  // Pega o próximo token
        }

        
        if (strcmp(command, "RQ") == 0) // Executa o comando RQ
        {
            requestResources(currentAllocation, remainingNeed, availableResources, numberOfCustomers, numberOfResources, customerID, resources, outputFile);
        } 
        else if (strcmp(command, "RL") == 0) // Executa o comando RL
        {
            releaseResources(currentAllocation, remainingNeed, availableResources, customerID, numberOfResources, resources, outputFile);
        } 
        else // Se o comando não for RQ ou RL, fecha o arquivo e retorna 0 (arquivo mal formatado)
        {
            fclose(file);
            return 0;
        }
    }

    fclose(file); // Fecha o arquivo e retorna 1 (sucesso)
    return 1;
}

// Processa recursos solicitados por um cliente e printa no arquivo se o pedido foi aceito ou negado e o motivo
void requestResources(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, int customerID, int *requestedResources, FILE *outputFile) 
{
    // Checa se o recurso solicitado é maior que a necessidade restante do cliente
    for (int i = 0; i < numberOfResources; i++) 
    {
        // Se for, o recurso solicitado é maior que a necessidade restante do cliente, logo o pedido é negado
        if (requestedResources[i] > remainingNeed[customerID][i]) 
        {
            fprintf(outputFile, "The customer %d request ", customerID);
            for (int j = 0; j < numberOfResources; j++) 
            {
                fprintf(outputFile, "%d ", requestedResources[j]);
            }
            fprintf(outputFile, "was denied because exceed its maximum need\n");
            return;
        }
    }

    // Checa se o recurso solicitado é maior que o recurso disponível
    for (int i = 0; i < numberOfResources; i++) 
    {
        // Se for, o recurso solicitado é maior que o recurso disponível, logo o pedido é negado e printa no arquivo
        if (requestedResources[i] > availableResources[i]) 
        {
            fprintf(outputFile, "The resources ");
            for (int j = 0; j < numberOfResources; j++) 
            {
                fprintf(outputFile, "%d ", availableResources[j]);
            }
            fprintf(outputFile, "are not enough to customer %d request ", customerID);
            for (int j = 0; j < numberOfResources; j++) 
            {
                fprintf(outputFile, "%d ", requestedResources[j]);
            }
            fprintf(outputFile, "\n");
            return;
        }
    }

    int tempAvailable[numberOfResources]; // Vetor temporário para armazenar os recursos disponíveis
    memcpy(tempAvailable, availableResources, numberOfResources * sizeof(int)); // Copia os recursos disponíveis para o vetor temporário

    // Checa se o estado é seguro
    for (int i = 0; i < numberOfResources; i++) 
    {
        availableResources[i] -= requestedResources[i];            // Subtrai os recursos solicitados dos recursos disponíveis
        currentAllocation[customerID][i] += requestedResources[i]; // Adiciona os recursos solicitados na alocação atual
        remainingNeed[customerID][i] -= requestedResources[i];     // Subtrai os recursos solicitados da necessidade restante
    }

    // Primeiro checa se o novo estado é seguro
    if (!checkSafety(currentAllocation, remainingNeed, availableResources, numberOfCustomers, numberOfResources, NULL)) 
    {
        // Se não for, reverte as mudanças feitas e printa no arquivo que o pedido foi negado
        for (int i = 0; i < numberOfResources; i++) 
        {
            availableResources[i] = tempAvailable[i];                  // Recupera os recursos disponíveis
            currentAllocation[customerID][i] -= requestedResources[i]; // Recupera a alocação atual
            remainingNeed[customerID][i] += requestedResources[i];     // Recupera a necessidade restante
        }
        fprintf(outputFile, "The customer %d request ", customerID);
        for (int j = 0; j < numberOfResources; j++) 
        {
            fprintf(outputFile, "%d ", requestedResources[j]);
        }
        fprintf(outputFile, "was denied because result in an unsafe state\n");
        return;
    }

    // Se chegou até aqui significa que o estado é seguro, printa no arquivo que o pedido foi aceito
    fprintf(outputFile, "Allocate to customer %d the resources ", customerID);
    for (int i = 0; i < numberOfResources; i++) 
    {
        fprintf(outputFile, "%d ", requestedResources[i]);
    }
    fprintf(outputFile, "\n");
}

// Processa recursos liberados por um cliente e printa no arquivo se o pedido foi aceito ou negado e o motivo
void releaseResources(int **currentAllocation, int **remainingNeed, int *availableResources, int customerID, int numberOfResources, int *resourcesToRelease, FILE *outputFile) 
{
    // Checa se o recurso liberado é maior que a alocação atual do cliente
    for (int i = 0; i < numberOfResources; i++) 
    {
        // Se for, o recurso liberado é maior que a alocação atual do cliente, logo o pedido é negado
        if (resourcesToRelease[i] > currentAllocation[customerID][i]) 
        {
            fprintf(outputFile, "The customer %d released ", customerID);
            for (int j = 0; j < numberOfResources; j++) 
            {
                fprintf(outputFile, "%d ", resourcesToRelease[j]);
            }
            fprintf(outputFile, "was denied because exceed its maximum allocation\n");
            return;
        }
    }

    // Libera os recursos e printa no arquivo que o pedido foi aceito
    for (int i = 0; i < numberOfResources; i++) 
    {
        availableResources[i] += resourcesToRelease[i];            // Recupera os recursos disponíveis
        currentAllocation[customerID][i] -= resourcesToRelease[i]; // Recupera a alocação atual
        remainingNeed[customerID][i] += resourcesToRelease[i];     // Recupera a necessidade restante
    }

    // Se chegou até aqui, significa que o pedido foi aceito, printa no arquivo que o pedido foi aceito
    fprintf(outputFile, "Release from customer %d the resources ", customerID);
    for (int i = 0; i < numberOfResources; i++) 
    {
        fprintf(outputFile, "%d ", resourcesToRelease[i]);
    }
    fprintf(outputFile, "\n");
}

// Imprime as matrizes no arquivo de acordo com o numero de clientes(rows / linhas) e recursos(cols / colunas)
void printAllMatrices(FILE *filePointer, int rows, int cols) 
{
    fprintf(filePointer, "MAXIMUM | ALLOCATION | NEED\n");
    // MAXIMUM | ALLOCATION | NEED
    for (int i = 0; i < rows; i++) 
    {
        // MAXIMUM
        for (int j = 0; j < cols; j++) 
        {
            fprintf(filePointer, "%d", maximumDemand[i][j]);
            if (j == cols - 1) 
            {
                fprintf(filePointer, "   | ");  // Adiciona três espaços após o último número na coluna MAXIMUM
            } 
            else 
            {
                fprintf(filePointer, " ");
            }
        }

        // ALLOCATION
        for (int j = 0; j < cols; j++) 
        {
            fprintf(filePointer, "%d", currentAllocation[i][j]);
            if (j == cols - 1) 
            {
                fprintf(filePointer, "      | ");  // Adiciona seis espaços após o último número na coluna ALLOCATION
            } 
            else 
            {
                fprintf(filePointer, " ");
            }
        }

        // NEED
        for (int j = 0; j < cols; j++) 
        {
            fprintf(filePointer, "%d ", remainingNeed[i][j]);  // Espaçamento de um espaço após o último número na coluna NEED
        }
        fprintf(filePointer, "\n");
    }
}

// Banker's Algorithm para checar se o estado é seguro baseado na alocação atual, necessidade restante e recursos disponíveis
int bankerAlgorithm(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources) 
{
    // Vetor para armazenar a sequência segura dos clientes(processos) | A sequencia segura é a ordem em que os processos podem ser executados sem causar deadlock
    int safeSequence[numberOfCustomers]; 
    return checkSafety(currentAllocation, remainingNeed, availableResources, numberOfCustomers, numberOfResources, safeSequence); // Retorna 1 se o estado for seguro e 0 caso contrário
}

// Checa se o estado é seguro (O estado é seguro se existe uma sequência segura) | O verdadeiro Banker's Algorithm
int checkSafety(int **currentAllocation, int **remainingNeed, int *availableResources, int numberOfCustomers, int numberOfResources, int *safeSequence) 
{
    int finished[numberOfCustomers]; // Vetor para armazenar se a NEED do cliente foi satisfeita ou não
    int work[numberOfResources];     // Vetor que copia os recursos disponíveis, representando os recursos disponíveis que podem ser usados

    memcpy(work, availableResources, numberOfResources * sizeof(int)); // Copia os recursos disponíveis para o vetor work
    memset(finished, 0, numberOfCustomers * sizeof(int));              // Inicializa o vetor finished com 0

    // Checa cada cliente até que todos os clientes tenham sido processados
    for (int k = 0; k < numberOfCustomers; k++) 
    {
        // Checa o recurso NEED do cliente pode ser satisfeito com os recursos disponíveis (work)
        for (int i = 0; i < numberOfCustomers; i++) 
        {
            if (!finished[i]) // Se a NEED do cliente não foi satisfeita
            {
                int j;
                for (j = 0; j < numberOfResources; j++)     // Checa se o recurso NEED do cliente pode ser satisfeito com os recursos disponíveis (work)
                {
                    if (remainingNeed[i][j] > work[j])      // Se o recurso NEED do cliente não pode ser satisfeito com os recursos disponíveis (work)
                    {
                        break;                              // Sai do loop e vai para o próximo cliente
                    }
                }
                if (j == numberOfResources)                 // Se o recurso NEED do cliente pode ser satisfeito com os recursos disponíveis (work)
                {
                    for (j = 0; j < numberOfResources; j++) // Adiciona os recursos alocados pelo cliente aos recursos disponíveis (work)
                    {
                        work[j] += currentAllocation[i][j]; // Adiciona temporariamente os recursos alocados pelo cliente aos recursos disponíveis (work)
                    }
                    finished[i] = 1;                        // Marca o cliente como processado
                    if (safeSequence != NULL)
                    {
                        safeSequence[k] = i;                // Adiciona o cliente a sequência segura
                    } 
                    break;                                  // Sai do loop e vai para o próximo cliente
                }
            }
        }
    }

    // Checa se todos os clientes foram processados
    for (int i = 0; i < numberOfCustomers; i++) 
    {
        if (!finished[i]) 
        {
            return 0; // Não é seguro
        }
    }
    return 1; // Pode dale que é seguro
}

// Conta o número de clientes no arquivo customer.txt
int countNumberOfCustomers(const char *filename) 
{
    // Abre o arquivo
    FILE *file = fopen(filename, "r");
    if (!file) 
    {
        return 0;
    }

    int customerCount = 0; // Contador de clientes
    char buffer[1024];     // Tamanho máximo de uma linha do arquivo

    // Lê cada linha do arquivo até chegar no final do arquivo
    while (fgets(buffer, sizeof(buffer), file)) 
    {
        customerCount++;   // Incrementa o contador de clientes
    }

    fclose(file);          // Fecha o arquivo
    return customerCount;  // Retorna o número de clientes
}

// Aloca memória para uma matriz 2D
int** allocate2DMatrix(int rows, int cols) 
{
    int **matrix = (int **)malloc(rows * sizeof(int *)); // Aloca memória para a matriz

    // Aloca memória para cada linha da matriz
    for (int i = 0; i < rows; i++) 
    {
        matrix[i] = (int *)malloc(cols * sizeof(int)); // Aloca memória para cada coluna da matriz
    }
    return matrix; // Retorna a matriz alocada
}

// Libera a memória alocada para uma matriz 2D
void free2DMatrix(int **matrix, int rows) 
{
    // Libera a memória alocada para cada linha da matriz
    for (int i = 0; i < rows; i++) 
    {
        free(matrix[i]); // Libera a memória alocada para cada coluna da matriz
    }
    free(matrix); // Libera a memória alocada para a matriz
}

// ver a parte que se colocar menos de 3 comandos na linha de comando, o programa não deve executar
// ver a parte sobre a identação do print da matriz no arquivo result.txt em relação ao MAXIMUM, ALLOCATION e NEED
// ver a parte sobre o MAX_COMMAND_LENGTH , no caso a gente nao tem um tamanho maximo de comando, mas o programa tem que funcionar com qualquer tamanho de comando