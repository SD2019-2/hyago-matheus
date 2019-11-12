#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <math.h>



#define BUFFERMAX 10  //Máximo de buffers livres
#define DORMINDO 0    //Processo suspendido
#define ACORDADO 1    //Processo em execução
#define ACABADO 0     //O processo está produzindo
#define PROCESSANDO 1 //Inidica que o produtor já acabou


typedef struct FILA{
    int posicao; //posição do buffer
    struct FILA *prox; //próxima posição
}Fila;

Fila *inicio_leitura = NULL, *fim_leitura = NULL, *inicio_escrita = NULL, *fim_escrita = NULL;
//Variaveis da estruta para controlar a entrada saida do buffer


//Variáveis globais

char bufferLimitado[BUFFERMAX];
int buffer_vazio = BUFFERMAX; //Todos os espaços livres
int buffer_cheio = 0;        //Não existe um espaço ocupado
int produz = ACORDADO; 
int consome = ACORDADO;
int status_processo = PROCESSANDO;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //inicializa o mutex que irá controlar o acesso ao buffer
pthread_mutex_t mutex_status_buffer = PTHREAD_MUTEX_INITIALIZER; //inicializa o mutex que irá controlar a espera(dormir/acordar)
pthread_cond_t status_produtor = PTHREAD_MUTEX_INITIALIZER; // Suspende a excução do produtor 
pthread_cond_t status_consumidor = PTHREAD_MUTEX_INITIALIZER; // Suspende a execução do consumidor


//funções
int setFila(int posicao, Fila **comeco, Fila **fim);
int getFila(Fila **comeco, Fila **fim);
void *Consumir();
void *Produzir();


int main(int argc, char *argv[]){ 

    //Variáveis podutora e consumidora (Threads)

    pthread_t tConsumidor, tProdutor;

    //Criando novas threads para os consumidores
    if(pthread_create(&tConsumidor, NULL, Consumir, "Consumidor consumiu : ") != 0){ 
        printf("Erro ao criar a thread!\n");
        return 0;
    }
    if(pthread_create(&tProdutor, NULL, Produzir, "Produtor produziu : ") != 0){
        printf("Erro ao criar a thread!\n");
        return 0;
    }

    //Esperando o termino de processamento das threads para finaliza-la
    if(pthread_join(tConsumidor, NULL) != 0){ 
        printf("Erro ao finalizar a thread!\n");
        return 0;
    }
    if(pthread_join(tProdutor, NULL) != 0){ 
        printf("Erro ao finalizar a thread!\n");
        return 0;
    }

    return 0;

}


void *Produzir(){

    //Produzindo algo
    char str[25] = "abcdefghijklmnopqrstuvyxz";
    int i, pos_buffer;
    //pos_buffer controla a posição do buffer para ser escrita

    for(i = 0; i <= 24;i++){

        //Semáforo para definir status do produtor (dormindo ou acordado)
        pthread_mutex_lock(&mutex_status_buffer);

        //Nenhuma posição disponivel no buffer_vazio e todas as posições ocupadas no buffer_cheio
        if(buffer_vazio == 0 || buffer_cheio == BUFFERMAX){ //se o buffer estiver cheio ele dorme
            
            printf("Produtor : Não tenho trabalho, irei dormir!\n");
            produz = DORMINDO;
            //Só libera o processo ao receber um sinal
            pthread_cond_wait(&status_produtor, &mutex_status_buffer); //entra em modo de espera esperando o sinal
            produz = ACORDADO; //volta a produzir
            printf("\nProdutor : Estão me chamando, vou trabalhar!\n");


        }//Fim do modo de espera


        //Semáforo para definir status do produtor
        pthread_mutex_unlock(&mutex_status_buffer);


        //Semáforo para exclusão mutua para controlar posição do buffer
        pthread_mutex_lock(&mutex);

        buffer_vazio--; //Decrementa espaço livre
        buffer_cheio++; //Incrementa espaço ocupado

        //Recebe posição da fila, se retorno for -1 quer dizer que a fila não inicializou, então ele deve pegar do controle do buffer
        if ((pos_buffer = getFila(&inicio_escrita, &fim_escrita)) == -1){ //pega posição livre do buffer
            pos_buffer = 0;  //inicio da fila
        }            

        //manda para a Fila
        if(setFila(pos_buffer, &inicio_leitura, &fim_leitura) == -1){
            printf("\n\nNão foi possivel inserir na fila!\n");
        }

        //Preenche o buffer
        bufferLimitado[pos_buffer] = str[i];

        printf("\nProdutor : Posicao (%d), Produtor produziu : (%c).\n", pos_buffer ,bufferLimitado[pos_buffer]);

        //Semáforo para definir status do produtor
        pthread_mutex_lock(&mutex_status_buffer);

        if(consome == DORMINDO){ //Se o consumidor estiver dormindo deve acorda-lo pois existe produtos a serem consumidos
            printf("\nProdutor : Consumidor está esperando, pode chama-lo pois existe produto no buffer!!\n");
            sleep(rand()%2);
            pthread_cond_signal(&status_consumidor);
            
        }

        pthread_mutex_unlock(&mutex_status_buffer);
        


        pthread_mutex_unlock(&mutex);

       
        sleep(rand() % 3);/* Processo dorme por tempo aleatório */

    }
    printf("\nProdutor : Hora de ir embora, meu trabalho está acabado!\n");
    status_processo = ACABADO;
}


void *Consumir(){

    int pos;

    for(;;){

        //Criando um semáforo para definir o status do consumidor (dormindo ou acordado)
        pthread_mutex_lock(&mutex_status_buffer);

        //Teste para ver se o buffer está vazio e esperar até o produtor avisar que tem uma nova produção
        if(buffer_cheio == 0){ //o buffer nao esta cheio

            if(status_processo == ACABADO && buffer_vazio == BUFFERMAX){  
                printf("Consumidor : Não tenho mais o que consumir, volto mais tarde. Até mais!\n\n");
                return 0;
            }

            printf("\nConsumidor : Vou esperar enquanto não tem consumo.\n");
            consome = DORMINDO;
            pthread_cond_wait(&status_consumidor, &mutex_status_buffer); //Entra em modo de espera até receber um sinal do produtor que existe novos produtos
            consome = ACORDADO;
            printf("\nConsumidor : Chegou um novo consumo, vou consumir!!\n");

        }

        pthread_mutex_unlock(&mutex_status_buffer);



        //Semáforo para exclusão mutua para controlar posições do buffer
        pthread_mutex_lock(&mutex);

        pos = getFila(&inicio_leitura, &fim_leitura);

        if(pos != ' ' || pos != -1){
            printf("\nConsumidor : Posicao (%d), Consumidor consumiu : (%c).\n", pos ,bufferLimitado[pos]);

            bufferLimitado[pos] = ' '; //retira aquela parte do buffer
            buffer_vazio++; //Incrementa uma posição no buffer livre
            buffer_cheio--; //Decrementa uma posição no buffer ocupado

            //Manda a posição do buffer vazio para a fila
            setFila(pos, &inicio_escrita, &fim_escrita);

            //semáforo para verificar o status do -produtor-
            pthread_mutex_lock(&mutex_status_buffer);

            if(produz == DORMINDO){ //Se estiver dormindo manda um sinal para avisar que tem buffer livre
                printf("Consumidor : Produtor está dormindo, terei que acorda-lo!\n");
                sleep(rand()%2);
                pthread_cond_signal(&status_produtor);

            }

            //liberar o semáforo
            pthread_mutex_unlock(&mutex_status_buffer);
            sleep(rand()%2);


        }
        else printf("\n\nErro na Leitura!!\n\n");

        //libera outro semáforo
        pthread_mutex_unlock(&mutex);
        sleep(rand() % 4);
    }
    


}



int setFila(int posicao, Fila **comeco, Fila **fim){

   Fila *novo;

   if (!(novo = (Fila*)malloc(sizeof(Fila)))){
      printf("\n\nErro na alocação de memoria!\n");
      return 0;
   }

   novo->posicao = posicao;
   novo->prox = NULL;

   (*fim != NULL) ? ((*fim)->prox = novo) : (*comeco = novo);

   *fim = novo;//Ajusta ponteiro fim para ultimo novo

   return 0;
}

int getFila(Fila **comeco, Fila **fim){

   Fila *pt;
   int posicaoBuffer;

   if(*fim != NULL)
   {
      pt = *comeco;
      *comeco = (*comeco)->prox;// Passa próximo elemento para inicio

      if(*comeco == NULL)//Se inicio for nulo, fila acabou
         *fim = NULL;

      posicaoBuffer = pt->posicao;
      free(pt);
      return(posicaoBuffer);// Retorna primeiro elemento
   }

   return (-1);
}