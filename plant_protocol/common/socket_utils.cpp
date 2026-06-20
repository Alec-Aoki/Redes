#include "socket_utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

namespace plant {

int criarSocketServidor(int porta) {
    int descritorSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (descritorSocket < 0) {
        cerr << "Erro ao criar socket do servidor: " << strerror(errno) << "\n";
        return -1;
    }

    /* SO_REUSEADDR evita o erro "Address already in use" quando o
    gerenciador eh reiniciado rapidamente apos um encerramento, pois
    o SO mantem a porta em estado de espera por um curto periodo */
    int valorReutilizar = 1;
    setsockopt(descritorSocket, SOL_SOCKET, SO_REUSEADDR, &valorReutilizar, sizeof(valorReutilizar));

    sockaddr_in enderecoServidor;
    memset(&enderecoServidor, 0, sizeof(enderecoServidor));
    enderecoServidor.sin_family = AF_INET;
    enderecoServidor.sin_addr.s_addr = INADDR_ANY; // Aceita conexoes de qualquer interface
    enderecoServidor.sin_port = htons(porta);

    if (bind(descritorSocket, reinterpret_cast<sockaddr*>(&enderecoServidor), sizeof(enderecoServidor)) < 0) {
        cerr << "Erro ao fazer bind na porta " << porta << ": " << strerror(errno) << "\n";
        close(descritorSocket);
        return -1;
    }

    /* O segundo argumento de listen() define o tamanho da fila de
    conexoes pendentes. 16 deve ser suficiente pro numero de dispositivos
    esperados */
    if (listen(descritorSocket, 16) < 0) {
        cerr << "Erro ao colocar socket em modo de escuta: " << strerror(errno) << "\n";
        close(descritorSocket);
        return -1;
    }

    return descritorSocket;
}

int conectarAoServidor(const string& endereco, int porta) {
    int descritorSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (descritorSocket < 0) {
        cerr << "Erro ao criar socket de cliente: " << strerror(errno) << "\n";
        return -1;
    }

    sockaddr_in enderecoServidor;
    memset(&enderecoServidor, 0, sizeof(enderecoServidor));
    enderecoServidor.sin_family = AF_INET;
    enderecoServidor.sin_port = htons(porta);

    if (inet_pton(AF_INET, endereco.c_str(), &enderecoServidor.sin_addr) <= 0) {
        cerr << "Endereco IP invalido: " << endereco << "\n";
        close(descritorSocket);
        return -1;
    }

    if (connect(descritorSocket, reinterpret_cast<sockaddr*>(&enderecoServidor), sizeof(enderecoServidor)) < 0) {
        cerr << "Erro ao conectar ao gerenciador em " << endereco << ":" << porta << ": " << strerror(errno) << "\n";
        close(descritorSocket);
        return -1;
    }

    return descritorSocket;
}

bool enviarTexto(int descritorSocket, const string& texto) {
    /* send() pode enviar apenas parte dos bytes solicitados quando o
    buffer do socket esta cheio. Por isso, repetimos o envio em loop
    ate que todos os bytes do texto tenham sido efetivamente enviados */
    size_t totalEnviado = 0;
    size_t totalAEnviar = texto.size();

    while (totalEnviado < totalAEnviar) {
        ssize_t bytesEnviadosAgora = send(descritorSocket, texto.c_str() + totalEnviado, totalAEnviar - totalEnviado, 0);
        if (bytesEnviadosAgora <= 0) {
            cerr << "Erro ao enviar dados pelo socket: " << strerror(errno) << "\n";
            return false;
        }
        totalEnviado += static_cast<size_t>(bytesEnviadosAgora);
    }

    return true;
}

/* Le do socket ate encontrar a sequencia \n\n, que delimita o fim do
header. Retorna o texto do header (sem a linha em branco final) por
meio de headerLido, e qlqr byte de payload que tenha sido lido
"a mais" no mesmo recv() eh devolvido em payloadAdiantado, pra nao
ser perdido. Retorna false se a conexao foi encerrada antes de um
header completo ser recebido */
static bool lerHeaderBruto(int descritorSocket, string& headerLido, string& payloadAdiantado) {
    string bufferAcumulado;
    char bufferTemporario[512];

    while (true) {
        size_t posicaoSeparador = bufferAcumulado.find("\n\n");
        if (posicaoSeparador != string::npos) {
            headerLido = bufferAcumulado.substr(0, posicaoSeparador);
            payloadAdiantado = bufferAcumulado.substr(posicaoSeparador + 2);
            return true;
        }

        ssize_t bytesRecebidos = recv(descritorSocket, bufferTemporario, sizeof(bufferTemporario), 0);
        if (bytesRecebidos <= 0) {
            // 0 significa que o peer fechou a conexao normalmente
            // Valor negativo indica erro de leitura
            return false;
        }
        bufferAcumulado.append(bufferTemporario, static_cast<size_t>(bytesRecebidos));
    }
}

bool receberMensagem(int descritorSocket, Mensagem& mensagemSaida) {
    string headerBruto;
    string payloadAdiantado;

    if (!lerHeaderBruto(descritorSocket, headerBruto, payloadAdiantado)) {
        return false;
    }

    if (!parsearHeader(headerBruto, mensagemSaida)) {
        return false;
    }

    /* Eh possivel que o ultimo recv() durante a leitura do header ja
    tenha trazido parte (ou todo) o payload junto, ja que o TCP nao
    respeita fronteiras de mensagem.
    payloadAdiantado contem esses bytes que vieram "de bonus" e precisam
    ser contabilizados antes de pedirmos mais dados ao socket */
    string payloadAcumulado = payloadAdiantado;

    while (static_cast<int>(payloadAcumulado.size()) < mensagemSaida.tamanhoPayload) {
        char bufferTemporario[512];
        ssize_t bytesRecebidos = recv(descritorSocket, bufferTemporario, sizeof(bufferTemporario), 0);
        if (bytesRecebidos <= 0) {
            return false;
        }
        payloadAcumulado.append(bufferTemporario, static_cast<size_t>(bytesRecebidos));
    }

    /* Pode haver bytes excedentes pertencentes a proxima mensagem caso
    o peer tenha enviado multiplas mensagens em sequencia rapida.
    Pra manter esse modulo simples, descartamos o excedente aqui
    Isso eh seguro nesse protocolo pq cada mensagem espera uma
    resposta antes da proxima ser enviada (exceto o streaming de
    DATA, que sempre eh lido em uma chamada dedicada de receberMensagem
    por leitura, sem mensagens adicionais aguardando no buffer) */
    mensagemSaida.payloadBruto = payloadAcumulado.substr(0, mensagemSaida.tamanhoPayload);

    return true;
}

void fecharSocket(int descritorSocket) {
    close(descritorSocket);
}

}