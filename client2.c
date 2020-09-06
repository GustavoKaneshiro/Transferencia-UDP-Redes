#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <locale.h>
#include <math.h>
#define portaserver 5000
#define portaclient1 4500
#define portaclient2 4000

//Gustavo Souza Kaneshiro - 2017004125 - Trabalho 2 de Redes
int main(){
  setlocale(LC_ALL, "Portuguese_Brazil");

  FILE *arquivo;
  //inicia os sockets do windows
  WSADATA wsa;
  if(WSAStartup(MAKEWORD(2, 0), &wsa) < 0){
    printf("Error WSAStartup\n");
    return -1;
  }

  int socket1, length,  n, tambuff = 0, error_count = 0, error_found = 0;
  struct sockaddr_in server;
  struct sockaddr_in client2;
  char buffer[1024], buffer2[1024];
  char nome_arq[1000];
  boolean flag = 0;

  //inicia o socket do Client2
  socket1 = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket1 < 0) {
    printf("Error socket\n");
    return -1;
  }

  //inicializa as informações do servidor
  memset(&server, 0, sizeof(server));
  memset(&client2, 0, sizeof(client2));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(5000);

  client2.sin_family = AF_INET;
  client2.sin_addr.s_addr = inet_addr("127.0.0.1");
  client2.sin_port = htons(4000);


  length = sizeof(server);

  //recebe o digito 1 para iniciar a Transferência do arquivo
  printf("Digite 1 para Começar a Transferência:\n");
  int i = 0;
  while(i != 1){
    scanf("%d", &i);
  }

  memset(buffer,'\0', 1024);
  //espera a resposta do servidor para começar a receber o arquivo
printf("Esperando Resposta do Servidor\n");
  n = 0;
  n = sendto(socket1, "21\n", 3, 0, (const struct sockaddr*)&server, length);
  while(1){
    n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
      if(buffer[0] != '\0' ){
        printf("Resposta Recebida do Servidor\n");
        break;
      }
    }
  }


  memset(buffer, '\0', 1024);
  //espera o client1 com o nome do arquivo
  printf("Esperando Nome do Arquivo\n");
  while(1){
    n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
      strcpy(nome_arq,buffer);
      break;
    }
  }

  printf("Nome de Arquivo Recebido\n");

  arquivo = fopen(nome_arq,"wb");
  if(arquivo == NULL){
    printf("Erro de Arquivo\n");
    return -1;
  }


  int count = 0;

  //loop de Transferência de arquivo
  while(1){
    memset(buffer, '\0', 1024);
    //recebe pacote do servidor
    n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
    if(n > 0){
      printf("Pacote %d Recebido\n", count);

      //verifica se é o ultimo pacote da Transferência
      if(buffer[0] == '1'){
        flag = 1;
      }

      //função simples de checksum
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
      char checksum_string[6];
      itoa(somatot, checksum_string, 10);


      if(strcmp(checksum_string, &buffer[1]) != 0){
        printf("Checksum error\n");
        error_found = 1;
        error_count++;
      }
      else{
          if(fwrite(&buffer[24], 1, sizeof(buffer)-24, arquivo) > 1){
            printf("Pacote %d OK\n", count);
            n = sendto(socket1, "ok", 2, 0, (const struct sockaddr*)&server, length);
            if(n < 0){
              while(n < 0 && error_count < 10)
                n = sendto(socket1, "ok", 2, 0, (const struct sockaddr*)&server, length);

                if(error_count >= 10)
                  break;
            }
            count++;
            error_found = 0;
            error_count = 0;
          }
          else{
            error_found = 1;
            error_count++;
          }
      }
      }
    else{
      error_found = 1;
      error_count++;
    }

    //loop de tratamento de erro
    while(error_found == 1 && error_count < 10){
      n = sendto(socket1, "notok", 5, 0, (const struct sockaddr*)&server, length);
      if(n < 0){
        error_count++;
      }
      else{
        while(1){
          memset(buffer, '\0', 1024);
          //recebe pacote do servidor
          n = recvfrom(socket1, buffer, 1024, 0, (struct sockaddr*)&server, &length);
          if(n > 0){
            printf("Pacote %d Recebido\n", count);

            //verifica se é o ultimo pacote da Transferência
            if(buffer[0] == '1'){
              flag = 1;
            }

            //função simples de checksum
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
            char checksum_string[6];
            itoa(somatot, checksum_string, 10);


            //compara o checksum com o cabeçalho da mensagem
            if(strcmp(checksum_string, &buffer[1]) != 0){
              printf("Checksum error\n");
              error_found = 1;
              error_count++;
              break;
            }
            else{
              //verifica se conseguiu escrever a mensagem no arquivo
              if(fwrite(&buffer[24], 1, sizeof(buffer)-24, arquivo) > 1){
                printf("Pacote %d OK\n", count);
                n = sendto(socket1, "ok", 2, 0, (const struct sockaddr*)&server, length);
                if(n < 0){
                  while(n < 0 && error_count < 10)
                    n = sendto(socket1, "ok", 2, 0, (const struct sockaddr*)&server, length);

                    if(error_count >= 10)
                      break;
                }
                count++;
                error_found = 0;
                error_count = 0;
                break;
              }
              else{
                error_found = 1;
                error_count++;
                break;
              }
            }
        }
        else{
          error_count++;
          break;
        }
      }
    }
  }
    //verifica se o último pacote foi recebido ou se o limite máximo de erros foi atingido
    if(error_count >= 10){
      break;
    }
    if(flag){
      break;
    }
  }

  fclose(arquivo);
  int close = 0;

  //avisa se o programa foi encerrado por causa de erro ou por causa de termino
  if(error_count >= 10)
  printf("Envio de Arquivo Cancelado, ERRO\nCtrl+C para fechar a aplicação\n");
  else
  printf("Envio de Arquivo Concluido\nCtrl+C para fechar a aplicação\n");
  while(1){
      scanf("%d", close);
      if(close == 1){
        break;
      }
  return 0;
}
}
