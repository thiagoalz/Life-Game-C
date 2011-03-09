#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/**
 * Jogo da Vida
 * ============
 *
 * Autor: Thiago Alvarenga Lechuga(RA: 079699)
 *
 * Descrição: Implementação baseada em pthreads para o jogo da vida.  São
 * utilizadas duas matrizes (table representando o tabuleiro atual para o jogo,
 * e old_table sendo uma referência ao tabuleiro anterior) representando o
 * tabuleiro do jogo. O tabuleiro pode ter tamanhos arbitrários (não existe
 * restrição de tamanho).  O número de threads também não possui limitantes. O
 * tabuleiro será dividido entre as threads tendo uma célula como unidade
 * mínima da divisão. Apenas uma das threads é responsável pela impressão do
 * tabuleiro a cada geração. Um vetor de variáveis que
 * indicam se o tabuleiro pode ser impresso ou se a próxima geração já pode ser
 * produzida é utilizado para manter a impressão em sincronia.
 *
 * Nota: Algumas funções são marcadas com a tag DEBUG, o que indica que elas
 * não estão sendo utilizadas no programa, mas que serviram para verificar a
 * corretude dos algoritmos.
 */

// constantes relativas às tabelas do jogo
#define ROWS    20        // quantidade de linhas na tabela
#define COLUMNS  20        // quantidade de colunas na tabela

// representação do tabuleiro do jogo
volatile char **table;    // próxima geração
volatile char **oldTable; // antiga geração

// representação de células na impressão do tabuleiro
char vazio = '_';         // caractere que representa um espaço sem célula viva
char vivo  = '#';         // caractere que representa um espaço com célula viva

// --------------------------------------------------------------------------------------------------------

// constantes relativas às threads e sincronização entre elas
#define THREADS  5  // quantidade de threads em execução (não inclui a thread responsável pela impressão)
#define WAIT     0  // threads esperam para gerar próxima tabela (a tabela atual pode não ter sido impressa)
#define GO_AHEAD 1  // threads podem gerar próxima tabela
#define END      2  // threads param de processar

// referência para as threads do programa
pthread_t threads[THREADS];

// variáveis utilizadas para sincronização impressão/geração da próxima tabela
volatile int nextGeneration[THREADS];

// --------------------------------------------------------------------------------------------------------

// constantes relativas a tempo
#define MIN_TIME 1  //segundos para gerar a próxima tabela (mantém a thread impressora em dormir para economia de cpu)

// constantes relativas a modelos para inicialização do tabuleiro
#define RANDOM   0  //inicializa tabuleiro randomicamente
#define BLINKER  1  //inicializa tabuleiro com um blinker (necessário ser ao menos 3x3)
#define BLOCO    2  //inicializa tabuleiro com bloco (necessário ser ao menos 2x2)
#define SAPO     3  //inicializa tabuleiro com sapo (necessário ser ao menos 4x2)
#define LWSS     4  //inicializa tabuleiro com nave espacial (necessário ser ao menos 5x4)
#define GLIDER   5  //inicializa tabuleiro com nave espacial (necessário ser ao menos 3x3)
// tipo de inicialização do tabuleiro
int initTable = RANDOM;

// --------------------------------------Funções-----------------------------------------------------------

// pausa uma thread por 'sec' segundos
void dormir (int sec){
    struct timespec timeOut, remains;

    timeOut.tv_sec = sec;

    nanosleep(&timeOut, &remains);
}

//alocação de memória para os tabuleiros
void alocarTable() {
    int i;
    table = (volatile char**) malloc(sizeof(char*)*ROWS);
    oldTable = (volatile char**) malloc(sizeof(char*)*ROWS);

    for(i = 0; i < ROWS; i++){
        table[i] = (volatile char*) malloc(sizeof(char)*COLUMNS);
        oldTable[i] = (volatile char*) malloc(sizeof(char)*COLUMNS);
    }
}

//liberação de memória ocupado pelos tabuleiros
void liberarTable() {
    int i;

    for(i = 0; i < ROWS; i++){
        free((char *)table[i]);
        free((char *)oldTable[i]);
    }

    free(table);
    free(oldTable);
}

// Tratamento do CTRL+C. Assim evita o problema de memory leak da alocação das
// matrizes.
void sigint() {
    
    int i;
    for( i = 0 ; i < THREADS ; i++ ) {
        // Solicitar que todas as threads sejam finalizadas.
        nextGeneration[i] = END;
    }
    
    // Dar um tempo para as threads processarem o que falta.
    sleep(2);
    liberarTable();
    exit(0);
}

// inicializa matriz randomicamente
void randomico(){
    int i;
    // assume que uma porcentagem máxima das células estarão inicialmente vivas
    int qCelulas = ROWS*COLUMNS*0.25;
    //inicializa semente randômica
    srand ( time(NULL) );
    for(i = 0; i < qCelulas; i++){
        int row = rand() % ROWS;
        int column = rand() % COLUMNS;
        table[row][column] = vivo;
    }
}

// inicializa a matriz com um blinker
void blinker(){
    table[1][0] = vivo;
    table[1][1] = vivo;
    table[1][2] = vivo;
}

// inicializa a matriz com um bloco
void bloco(){
    table[0][0] = vivo;
    table[0][1] = vivo;
    table[1][0] = vivo;
    table[1][1] = vivo;
}

// inicializa a matriz com um sapo
void sapo() {
    /*table[0][1] = vivo;
    table[0][2] = vivo;
    table[0][3] = vivo;
    table[1][0] = vivo;
    table[1][1] = vivo;
    table[1][2] = vivo;*/

    table[8][6] = vivo;
    table[8][7] = vivo;
    table[8][8] = vivo;
    table[9][5] = vivo;
    table[9][6] = vivo;
    table[9][7] = vivo;
}

// inicializa a matriz com uma nave
void nave(){
    table[0+5][1+5] = vivo;
    table[0+5][4+5] = vivo;
    table[1+5][0+5] = vivo;
    table[2+5][0+5] = vivo;
    table[2+5][4+5] = vivo;
    table[3+5][0+5] = vivo;
    table[3+5][1+5] = vivo;
    table[3+5][2+5] = vivo;
    table[3+5][3+5] = vivo;
}

// inicializa a matriz com um glinder
void glider(){
    /*table[0][0] = vivo;
    table[0][1] = vivo;
    table[0][2] = vivo;
    table[1][0] = vivo;
    table[2][1] = vivo;*/

    table[7][6] = vivo;
    table[7][7] = vivo;
    table[7][8] = vivo;
    table[8][6] = vivo;
    table[9][7] = vivo;
}

// inicialização do tabuleiro com algumas células vivas (de acordo com algum
// dos formatos padrão)
void inicializaTable() {
    int i, j;
    // inicializa todo o tabuleiro com vazio
    for(i = 0; i < ROWS; i++)
        for(j = 0; j < COLUMNS; j++){
            table[i][j] = vazio;
            oldTable[i][j] = vazio;
        }

    //preenche o tabuleiro
    switch(initTable){
        case RANDOM: randomico(); break;
        case BLINKER: blinker(); break;
        case BLOCO: bloco(); break;
        case SAPO: sapo(); break;
        case LWSS: nave(); break;
        case GLIDER: glider(); break;
    }
}

// para uma célula localizada na posição ij, retorna a
// quantidade de vizinhos vivos ao redor de ij.
// No geral, uma matriz 3x3 centrada em ij possui 9 ele-
// mentos. Se ij estiver em uma das bordas, entretanto,
// esses vizinhos devem ser considerados "mortos" e não
// serão computados.
int contarVizinhos(int i, int j) {
    int vizinhos = 0;                   // quantidade de vizinhos vivos de ij

    // coordenadas de início e fim para uma matriz centrada em ij
    int sLine = i - 1;
    int eLine = sLine + 3;
    int sCol = j - 1;
    int eCol = sCol + 3;

    // correção da matriz se ij está próximo a uma das bordas da tabela
    if(sLine < 0) sLine ++;
    if(sCol < 0) sCol ++;
    if(eLine > ROWS) eLine --;
    if(eCol > COLUMNS) eCol --;

    // contagem de elementos vivos na matriz
    int k, l;
    for(k = sLine; k < eLine; k++)
        for(l = sCol; l < eCol; l++){
            if(oldTable[k][l] == vivo) vizinhos ++;
        }

    //como toda a matriz centrada em ij é vasculhada, se ij está vivo então ele foi contado tb;
    // mas ij não é vizinho de si mesmo
    if(oldTable[i][j] == vivo) vizinhos --;

    return vizinhos;
}

// DEBUG: imprime a contagem de vizinhos na forma do tabuleiro
void tabelaVizinhos(){
    int i = 0;
    for(i = 0; i < ROWS; i++){
        int j = 0;
        for(j = 0; j < COLUMNS; j++){
            printf("%i ", contarVizinhos(i, j));
        }
        printf("\n");
    }
}

// aplica as regras do jogo da vida em uma célula ij para descobrir seu
// estado na próxima geração
void gerarCelula(int i, int j)
{
    int vizinhos = contarVizinhos(i, j);
    if(oldTable[i][j] == vivo)
        // 1. célula viva com menos que dois vizinhos morre (solidão)
        // 2. célula viva com mais que três vizinhos morre (superlotação)
        if(vizinhos < 2 || vizinhos > 3)
            table[i][j] = vazio;
        // 3. célula viva com dois ou três vizinhos sobrevive
        else
            table[i][j] = vivo;

    else // 4. célula morta com três vizinhos renasce.
        if(vizinhos == 3)
            table[i][j] = vivo;
        else
            table[i][j] = vazio;
}

// tarefa das threads slave: preocupar-se com sua região do tabuleiro
void* threadJob(void* arg) {
    // identificador da thread (possibilita a divisão estática de células entre as threads)
    int id = (int) arg;

    int elementos = ROWS*COLUMNS;                   // total de células no tabuleiro

    // divisão das células entre as threads
    int elementosPorThread = elementos/THREADS;
    int restoElementos = elementos%THREADS;
    // Para o calculo do início e do fim, consideramos que a identificação da
    // thread é uma progressão aritmética começando do zero e de razão '1'(um).
    int inicio = id*elementosPorThread + ((id >= restoElementos)?restoElementos:id);
    int fim = (id+1)*elementosPorThread + ((id + 1 >= restoElementos)?restoElementos:id + 1);

    int initI = inicio/COLUMNS;
    int initJ = inicio%COLUMNS;
    int lastI = fim/COLUMNS;
    int lastJ = fim%COLUMNS;

    while(1){
        // aguarda o momento de gerar a próxima tabela
        while(nextGeneration[id] == WAIT);

        // parar a thread
        if(nextGeneration[id] == END)
            pthread_exit(0);

        // gera cada uma de suas células
        int i, j;
        for(i = initI; i < lastI; i++) {
            for(j = initJ; j < COLUMNS; j++){
                gerarCelula(i, j);
            }
        }

        // nota: para i = lastI, o limite do j é lastJ (não COLUMNS)
        for(j = initJ; j < lastJ; j++){
            gerarCelula(i, j);
        }

        // libera suas células para impressão
        nextGeneration[id] = WAIT;
    }
}

//criação das threads e inicialização da variável para sincronização
void criaThreads() {
    int i;
    for(i = 0; i < THREADS; i++){
        nextGeneration[i] = WAIT; // até aqui, o tabuleiro inicial ainda não foi impresso
        threads[i] = pthread_create(&threads[i], NULL, threadJob, (void *)i);

        // se alguma thread não puder ser criada, sair do programa
        if (threads[i]){
            exit(1);
        }
    }
}

//percorre o tabuleiro, imprimindo-o
void printTable() {
    // limpa a tela
    system ("clear");

    int i, j;
    for(i = 0; i < ROWS; i++){
        for(j = 0; j < COLUMNS; j++)
            printf("%c", table[i][j]);
        printf("\n");
    }
}

// troca as tabelas para a próxima geração e libera as threads
// nota: a geração atual é criada com base na geração antiga.
// Aqui, a geração antiga é descartada e a geração atual pas-
// sa a ser a geração antiga para a nova geração. Para poupar
// tempo, as referências às tabelas são apenas trocadas.
void prepareNextGeneration(){
    volatile char **aux = oldTable;
    oldTable = table;
    table = aux;

    // libera as threads em espera para gerar suas células
    int i;
    for(i = 0; i < THREADS; i++)
        nextGeneration[i] = GO_AHEAD;
}

// bloqueia a thread impressora até que todas as threads já tenham
// finalizado suas células na tabela
void checkNextGeneration(){
    int i;
    for(i = 0; i < THREADS; i++)
        while (nextGeneration[i] == GO_AHEAD);
}

//thread principal: cria o tabuleiro e as threads e imprime o tabuleiro
int main() {
    
    signal(SIGINT, sigint); // tratamento de interrupção crtl+c

    alocarTable();
    inicializaTable();
    criaThreads();

    while(1){
        printTable();
        prepareNextGeneration();
        dormir(MIN_TIME);
        checkNextGeneration();
    }
}
