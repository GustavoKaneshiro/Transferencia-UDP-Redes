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

  //inicia os sockets no windows
  WSADATA wsa;
  if(WSAStartup(MAKEWORD(2, 0), &wsa) < 0){
    printf("Error WSAStartup\n");
    return -1;
  }


  int socket_servidor, length, client1len, client2len, auxlen, n, tambuff = 0, error_found = 0, error_count = 0;
  struct sockaddr_in server;
  struct sockaddr_in client1, client2, aux;
  char buffer[1024], buffer2[1024];
  char nome_arq[1000];
  boolean flag = 0;

  //inicia o socket_servidor
  socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket_servidor < 0) {
    printf("Error socket\n");
    return -1;
  }

  //inicia os sockets dos clientes e armazena as informações do servidor
  auxlen = sizeof(aux);
  client2len = sizeof(client2);
  client1len = sizeof(client1);
  length = sizeof(server);

  memset(&client2, 0, sizeof(client2));
  memset(&client1, 0, client1len);
  memset(&server, 0, length);
  memset(&aux, 0, sizeof(aux));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr=inet_addr("127.0.0.1");
  server.sin_port = htons(5000);


  memset(&server.sin_zero, 0 , 8);


  //da bind no socket
  int x = bind(socket_servidor,(struct sockaddr*)&server, length);

  if(x<0){
    printf("Bind error\n");
    return -1;
  }
  printf("Esperando Clientes\n");
  boolean c1 = 0, c2 = 0;

  //espera a resposta dos clientes e manda para um aux quando recebe uma Resposta
  //o aux vai armazenar o endereço do socket no client certo de acordo com o número recebido na primeira posição da mensagem
  while(1){
      n = recvfrom(socket_servidor, buffer, 1024, 0, ( struct sockaddr*)&aux, &auxlen);
      buffer[n] = '\0';
      if(n < 0)
      {
        printf("Error recvfrom\n");
        return -1;
      }
      if(n > 0){
        //recebe o client 1 e armazena o nome do arquivo que ele vai enviar
        if(buffer[0] == '1'){
          client1 = aux;
          if(buffer[1] == '1'){
            strcpy(nome_arq,&buffer[2]);
            printf("Client1 esta preparado\n");
            n = sendto(socket_servidor, "Message Received\n",17,0, (const struct sockaddr*)&client1, client1len);
            c1  = 1;
        }
      }
        else if(buffer[0] == '2'){
          //recebe o client 2
          client2 = aux;
          if(buffer[1] == '1'){
            printf("Client2 esta preparado\n");
            n = sendto(socket_servidor, "Message Received\n", 17,0, (const struct sockaddr*)&client2, sizeof(client2));
            c2 = 1;
          }
        }
    }
      if(c1 && c2){
        //quando ambos clientes estão prontos, o servidor envia o nome do arquivo para o cliente 2
        system("cls");
        printf("Clientes Prontos\n");
        n = sendto(socket_servidor, nome_arq, strlen(nome_arq), 0, (const struct sockaddr*)&client2, sizeof(client2));
        if(n < 0){
          printf("Erro ao mandar o nome do arquivo para o client2\n");
          return -1;
        }
        n = sendto(socket_servidor, "ok", 2, 0, (const struct sockaddr*)&client1, sizeof(client1));
        if(n < 0){
          printf("Erro client1\n");
          return -1;
        }
        break;
      }
  }




  int count = 0;
  //inicio de Transferência dos pacotes
  while(1){
      memset(buffer, '\0', 1024);
      n = recvfrom(socket_servidor, buffer, 1024, 0,(struct sockaddr*)&aux,&auxlen);
      //se recebeu corretamente a mensagem do cliente 1, o servidor vai começar a repassar ela para o cliente 2
      if(n > 0)
      {
      client1 = aux;

      //verifica se é o último pacote do arquivo
      if(buffer[0] == '1'){
        flag = 1;
      }

      printf("Repassando pacote %d\n", count);

      //função simples de checksum
      int i, tamtot = 0;
      for(i = 24; i < 1024; i++){
        tamtot += (int)buffer[i];
      }
      tamtot = (int)fabs(tamtot);
      while(tamtot < 1000)
      {
        tamtot *=10;
          if(tamtot == 0)
            tamtot = 1000;
      }
      char checksum_string[6];
      itoa(tamtot, checksum_string, 10);


      //checa erro no checksum
      if(strcmp(checksum_string, &buffer[1]) != 0){
        error_found = 1;
        error_count++;
        break;
      }
      else{
      //envia o pacote para o client2
      n = sendto(socket_servidor, buffer, 1024, 0, (const struct sockaddr*)&client2, sizeof(client2));

      //verifica se o envio deu certo
      if(n < 0){
        printf("Erro repassando o pacote %d\n", count);
        error_found = 1;
        error_count++;
        break;
      }
      else{
      while(1){
        //espera mensagem do client2
        memset(buffer2, '\0', 1024);
          n = recvfrom(socket_servidor, buffer2, 1024, 0, (struct sockaddr*)&client2, &client2len);
          //verifica erro no recebimento da mensagem do client2
          if(n > 0){
            //verifica se o pacote que o client2 recebeu estava certo
            if(strcmp(buffer2, "ok") == 0){
              printf("Pacote %d repassado\n", count);
              n = sendto(socket_servidor, "ok", 2, 0, (const struct sockaddr*)&client1, sizeof(client1));
              count++;
              error_count = 0;
              error_found = 0;
              break;
            }
            else if(strcmp(buffer2, "notok") == 0){
              printf("Erro no pacote %d", count);
              error_found = 1;
              error_count++;
              break;
            }
          }
      }
    }
  }
  }
  else if(n < 0){
    printf("Erro no Recebimento\n");
    error_count++;
    error_found = 1;
  }

  //loop de erro encontrado, o loop encerra quando envia o pacote com sucesso ou quando
  //o número de erros no mesmo pacote for de 10
  while(error_found == 1 && error_count < 10){

      //envia a mensagem de erro para o client1
      n = sendto(socket_servidor, "notok", 5, 0, (const struct sockaddr*)&client1, sizeof(client1));

      //verifica se a mensagem foi enviada
      if(n > 0){
        //espera receber o pacote novamente do client1
        while(1){
          memset(buffer, '\0', 1024);
          n = recvfrom(socket_servidor, buffer, 1024, 0,(struct sockaddr*)&client1,&client1len);
          if(n > 0){

            printf("Repassando pacote %d\n", count);

            //função simples de checksum
            int i, tamtot = 0;
            for(i = 24; i < 1024; i++){
              tamtot += (int)buffer[i];
            }
            tamtot = (int)fabs(tamtot);
            while(tamtot < 1000)
              tamtot *=10;
            char checksum_string[6];
            itoa(tamtot, checksum_string, 10);

            //checa erro no checksum
            if(strcmp(checksum_string, &buffer[1]) != 0){
              error_count++;
              break;
            }
            else{
              //envia o pacote para o client2
              n = sendto(socket_servidor, buffer, 1024, 0, (const struct sockaddr*)&client2, sizeof(client2));

              //verifica se teve erro no envio do pacote
              if(n < 0){
                printf("Erro repassando o pacote %d\n", count);
                error_found = 1;
                error_count++;
                break;
              }
              else{
                int flag_resposta = 0;
                while(1){
                  //espera uma resposta do client2
                  memset(buffer2, '\0', 1024);
                    n = recvfrom(socket_servidor, buffer2, 1024, 0, (struct sockaddr*)&client2, &client2len);
                    if(n > 0){
                      if(strcmp(buffer2, "ok") == 0){
                        printf("Pacote %d repassado\n", count);
                        n = sendto(socket_servidor, "ok", 2, 0, (const struct sockaddr*)&client1, sizeof(client1));
                        count++;
                        error_count = 0;
                        error_found = 0;
                        flag_resposta = 1;
                        break;
                      }
                      else if(strcmp(buffer2, "notok") == 0){
                        printf("Erro no pacote %d", count);
                        error_found = 1;
                        error_count++;
                        flag_resposta = 1;
                        break;
                      }
                    }
                  }
                  if(flag_resposta == 1)
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
      else{
        error_count++;
      }
    }

    //verifica se o número de erros de um pacote foi maior ou igual a 10 para acabar com o loop de Transferência
    if(error_count >= 10){
      printf("Timeout no temporizador/n");
      break;
    }
    //verifica se o último pacote foi enviado para sair do loop
    if(flag){
      break;
    }
  }

  int close = 0;
  //determina se a aplicação parou por causa de um erro ou por causa do termino de Transferência
  if(error_count == 10)
    printf("Erro no envio de Arquivo\nCtrl+C para fechar a aplicação\n");
  else
    printf("Envio de Arquivo Concluido\nCtrl+C para fechar a aplicação\n");
  while(1){
      scanf("%d", close);
      if(close == 1){
        break;
      }
    }
    return 0;
}
