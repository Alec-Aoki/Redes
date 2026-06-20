#include "orquestrador.h"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
using namespace std;

/*
Ponto de entrada do processo Orquestrador

Esse processo nao substitui sensor e atuador, ele apenas inicia cada
um deles como um processo filho de verdade (com PID proprio no SO, via
fork e exec) e concentra a saida de todos em um unico terminal, com
cada linha prefixada pelo id do dispositivo de origem

uso
  ./orquestrador -> sobe a lista default
  ./orquestrador sensor:TERMO:01 atuador:AQUEC:01 -> sobe so o que foi pedido
*/

// Guarda os PIDs dos filhos abertos, usado pra encerrar todos de uma vez ao sair
static vector<pid_t> pidsFilhos;

// Handler de SIGINT (Ctrl C), garante que os processos filhos sao encerrados junto com o orquestrador
static void tratarSinalEncerramento(int sinal) {
    (void)sinal;
    for (pid_t pid : pidsFilhos) {
        kill(pid, SIGTERM);
    }
    _exit(0);
}

/*
Da fork e exec de um dispositivo (sensor ou atuador), redirecionando a
saida do processo filho pra dentro de um pipe
Retorna o descritor de leitura do pipe (usado pelo orquestrador pra
ler a saida do filho) e preenche pidSaida com o PID do processo criado
*/
static int iniciarProcessoFilho(const orquestrador::ConfigDisp& configuracao, pid_t& pidSaida) {
    int descritoresPipe[2];
    if (pipe(descritoresPipe) < 0) {
        cerr << "Erro ao criar pipe para " << configuracao.tipoBinario << "\n";
        pidSaida = -1;
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        cerr << "Erro ao dar fork para " << configuracao.tipoBinario << "\n";
        pidSaida = -1;
        return -1;
    }

    if (pid == 0) {
        // Processo filho: redireciona stdout e stderr pro lado de escrita do pipe
        close(descritoresPipe[0]); // O filho nao le do pipe, so escreve
        dup2(descritoresPipe[1], STDOUT_FILENO);
        dup2(descritoresPipe[1], STDERR_FILENO);
        close(descritoresPipe[1]);

        // O binario do dispositivo fica na mesma pasta bin do orquestrador
        string caminhoBinario = "./bin/" + configuracao.tipoBinario;

        /*
        execv substitui a imagem do processo filho pelo binario do
        dispositivo, mas mantem o mesmo PID, ou seja, esse processo
        continua sendo o mesmo processo do SO que o fork criou
        */
        char* argumentosExecv[4];
        argumentosExecv[0] = const_cast<char*>(caminhoBinario.c_str());
        argumentosExecv[1] = const_cast<char*>(configuracao.subtipo.c_str());
        argumentosExecv[2] = const_cast<char*>(configuracao.numero.c_str());
        argumentosExecv[3] = nullptr;

        execv(caminhoBinario.c_str(), argumentosExecv);

        // Se execv retornar, algo deu errado (binario nao encontrado, por exemplo)
        cerr << "Erro ao executar " << caminhoBinario << "\n";
        _exit(1);
    }

    // Processo pai (orquestrador): fecha o lado de escrita, que so o filho usa
    close(descritoresPipe[1]);
    pidSaida = pid;
    return descritoresPipe[0];
}

/*
Le continuamente do pipe de um processo filho e imprime cada linha no
terminal do orquestrador, prefixada com o id do dispositivo
Roda em uma thread dedicada por dispositivo, ja que a leitura de cada
pipe eh bloqueante
*/
static void encaminharSaidaFilho(int descritorLeitura, const string& prefixo) {
    char bufferLinha[1024];
    string linhaAcumulada;

    char caractereAtual;
    while (read(descritorLeitura, &caractereAtual, 1) > 0) {
        if (caractereAtual == '\n') {
            cout << "[" << prefixo << "] " << linhaAcumulada << "\n";
            linhaAcumulada.clear();
        } else {
            linhaAcumulada += caractereAtual;
        }
    }

    // Imprime qualquer resto de linha que nao terminou com \n antes do pipe fechar
    if (!linhaAcumulada.empty()) {
        cout << "[" << prefixo << "] " << linhaAcumulada << "\n";
    }

    (void)bufferLinha; // Mantido apenas para clareza do tamanho de buffer pretendido, leitura eh byte a byte
    close(descritorLeitura);
}

int main(int argumentosTotais, char* argumentos[]) {
    cout.setf(ios::unitbuf);

    vector<orquestrador::ConfigDisp> dispositivos;

    if (argumentosTotais == 1) {
        cout << "Nenhum argumento informado, usando lista default de dispositivos\n";
        dispositivos = orquestrador::listaDispoDef();
    } else {
        dispositivos = orquestrador::parsearArgs(argumentosTotais, argumentos);
        if (dispositivos.empty()) {
            cerr << "Nenhum dispositivo valido informado, encerrando\n";
            return 1;
        }
    }

    // Registra o handler de Ctrl+C pra encerrar os filhos junto com o orquestrador
    signal(SIGINT, tratarSinalEncerramento);

    vector<thread> threadsLeitura;

    for (auto const& configuracao : dispositivos) {
        pid_t pidFilho = -1;
        int descritorLeitura = iniciarProcessoFilho(configuracao, pidFilho);

        if (pidFilho < 0) {
            continue;
        }

        pidsFilhos.push_back(pidFilho);

        string prefixo = configuracao.tipoBinario + "-" + configuracao.subtipo + "-" + configuracao.numero;
        threadsLeitura.push_back(thread(encaminharSaidaFilho, descritorLeitura, prefixo));

        cout << "Processo " << prefixo << " iniciado com PID " << pidFilho << "\n";
    }

    cout << "Todos os dispositivos foram iniciados, pressione Ctrl+C para encerrar tudo\n";

    // Espera todas as threads de leitura terminarem, o que so acontece quando os processos filhos encerram
    for (auto& threadLeitura : threadsLeitura) {
        threadLeitura.join();
    }

    // Espera os processos filhos serem finalizados de fato pelo SO, evitando processos zumbis
    for (pid_t pid : pidsFilhos) {
        waitpid(pid, nullptr, 0);
    }

    return 0;
}
