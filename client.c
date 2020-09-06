#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <locale.h>
#include <math.h>
#include <unistd.h>
#define portaserver 5000
#define portaclient1 4500
#define portaclient2 4000

//Gustavo Souza Kaneshiro - 2017004125 - Trabalho 2 de Redes
int main(){
  setlocale(LC_ALL, "Portuguese_Brazil");

  //inicia os sockets no windows
  WSADATA wsa;
  if(WSAStartup(MAKEWORD(2, 0), &wsa) < 0){
    printf("Error WSAStartup\n");
    return -1;
  }

  FILE *arquivo;

  int socket1, length,  n, tambuff = 0, error_found = 0, error_count = 0;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buffer[1024], buffer2[1024];
  char nome_arq[1000];
  boolean flag = 0;

  //recebe o nome do arquivo a ser enviado
  printf("Digite o nome do arquivo:\n");
  scanf("%s", &nome_arq);



  if( access(nome_arq, F_OK) == -1){
    printf("Erro ao encontrar o arquivo\n");
    printf("Ctrl+C para sair da aplicação\n");
    while(1){
    }
  }
  else{
    printf("Arquivo encontrado\nAguardando resposta do servidor\n");
  }

  arquivo = fopen(nome_arq, "rb");
  //inicia o socket
  socket1 = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket1 < 0) {
    printf("Error socket\n");
    return -1;
  }

  memset(&server, 0, sizeof(server));

  //armazena as informações do servidor
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(5000);

  length = sizeof(server);

  //coloca o buffer de string com o nome do arquivo a ser enviado para o servidor
  memset(buffer,'\0', 1024);
  buffer[0] = '1';
  buffer[1] = '1';
  strcpy(&buffer[2], nome_arq);

  n = 0;
  n = sendto(socket1, buffer, strlen(buffer), 0, (const struct sockaddr*)&server, length);

  //espera a resposta do servidor para iniciar a Transferência dos pacotes
  while(1){
    n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
      if(buffer[0] != '\0' ){
        printf("Resposta recebida do servidor\n");
        break;
      }
    }
  }

  //espera a resposta do servidor dizendo que o outro cliente está pronto para receber o arquivo
  printf("Esperando o outro client\n");
  memset(buffer, '\0', 1024);
  while(1){
    n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
      if(strcmp(buffer, "ok") == 0){
        printf("OK\n");
        break;
      }
    }
  }

  //verifica o tamanho do arquivo a ser enviado para mostrar visualmente para o ususario
  fseek(arquivo,0,SEEK_END);
  long int total = ftell(arquivo)/1000;
  fseek(arquivo,0,SEEK_SET);
  int count = 0;
  while(1){
    memset(buffer, '\0', 1024);
    //le parte do arquivo e coloca em um buffer de string para enviá-lo em pacotes
    tambuff = fread(&buffer[24],1, 1000, arquivo);
    //verifica se é o último pacote a ser enviado
    if(tambuff < 1000){
      buffer[0] = '1';
      flag = 1;
    }
    else{
      buffer[0] = '0';
    }


    //função de checksum simples para enviar junto do pacote nas primeiras posições do buffer
    int i;
    long long int somatot = 0;
    for(i = 24; i < 1024; i++){
      somatot += (int) buffer[i];
    }
    somatot = (int)fabs(somatot);
    while(somatot < 1000){
      somatot = somatot * 10;
      if(somatot == 0)
        somatot = 1000;
    }
    itoa(somatot, &buffer[1], 10);

    //inicio de envio do pacote
    printf("Enviando pacote %d de %d\n", count, total);
    n = sendto(socket1, buffer, 1024, 0, (const struct sockaddr*)&server, length);
    if(n < 0){
      //encontra erro no envio e marca o flag de erro
      printf("Erro enviando o pacote numero %d\n",count);
      error_found = 1;
      error_count++;
    }
    else{
    //loop para esperar a resposta do servidor
    while(1){
      memset(buffer2,'\0',1024);
      n = recvfrom(socket1, buffer2, 1024, 0, (struct sockaddr*)&server, &length);
      if(n > 0){
      if(strcmp(buffer2, "ok") == 0){
        //recebe a mensagem do servidor de que o pacote está ok e continua a Transferência
        printf("Pacote %d enviado\n",count);
        memset(buffer2, '\0', 1024);
        count++;
        error_found = 0;
        error_count = 0;
        break;
      }
      else if(strcmp(buffer2, "notok") == 0){
        //recebe mensagem do servidor que deu erro e aumenta o contador de erro
        printf("Erro no pacote %d\n", count);
        error_found = 1;
        error_count++;
      }
    }
  }
}
  //se algum erro for encontrado ao enviar um pacote, o programa entra em um loop para tentar enviar o pacote novamente
  //e se der erro 10 vezes o programa terminará para não ficar em um loop infinito no envio de um pacote defeituoso
  while(error_found == 1 && error_count < 10){

    n = sendto(socket1, buffer, 1024, 0, (const struct sockaddr*)&server, length);
    if(n < 0){
      //encontra erro no envio e marca o flag de erro
      printf("Erro enviando o pacote numero %d\n",count);
      error_found = 1;
      error_count++;
    }
    else{
    while(1){
    memset(buffer2,'\0',1024);
    n = recvfrom(socket1, buffer2, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
    if(strcmp(buffer2, "ok") == 0){
      printf("Pacote %d enviado\n",count);
      memset(buffer2, '\0', 1024);
      count++;
      error_found = 0;
      error_count = 0;
      break;
    }
    else if(strcmp(buffer2, "notok") == 0){
      printf("Erro no pacote %d\n", count);
      error_found = 1;
      error_count++;
    }
  }
}
}
  }

    //para a Transferência se o programa detectou erro 10 ou mais vezes em uma única Transferência
    if(error_count >= 10){
      printf("Timeout no temporizador\n");
      break;
    }

    //para a Transferência quando o programa atinge o último pacote a ser enviado
    if(flag){
      break;
    }
  }

  fclose(arquivo);
  int close = 0;

  //Verifica se o programa terminou por causa de algum erro no envio ou devido ao termino correto
  if(error_count < 10)
  printf("Envio de Arquivo Concluido\nCtrl+C para fechar a aplicação\n");
  else
  printf("Envio de Arquivo Concluido com Erro\nCtrl+C para fechar a aplicação\n");

  while(1){
      scanf("%d", close);
      if(close == 1){
        break;
      }
  }

  return 0;
}
