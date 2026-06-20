#ifndef ACTUATOR_REGISTRY_H
#define ACTUATOR_REGISTRY_H

#include <string>
#include <map>
#include <mutex>
using namespace std;

/*
Esse modulo guarda o socket de cada atuador conectado ao Gerenciador
Eh necessario pq quando um sensor manda uma leitura fora dos limites,
a propria thread que esta atendendo esse sensor precisa conseguir
mandar um CMD pro atuador correto, usando a conexao que o atuador ja
deixou aberta desde o handshake

Alem do socket, esse modulo tambem guarda se o atuador esta ligado ou
desligado no momento
Isso eh necessario pra implementar a histerese:
Sem saber o estado atual do atuador, o Gerenciador mandaria CMD ON a
cada leitura nova enquanto a variavel estiver fora do limite, mesmo o
atuador ja estando ligado desde a leitura anterior
Guardando o estado, a thread do sensor so manda um CMD novo quando
realmente precisa mudar o estado do atuador (de ligado pra desligado
ou vice versa), o que tambem deixa o log de mensagens mais limpo e
fiel ao que de fato esta acontecendo na estufa

Assim como no registro de sensores, o acesso eh protegido por mutex
pq varias threads (atuadores se conectando, sensores disparando CMD)
podem usar esse registro ao mesmo tempo
*/

namespace plant {

// Guarda o socket de comunicacao e o estado atual (ligado ou nao) de um atuador
struct EstadoAtuador {
    int descritorSocket;
    bool ligado;

    EstadoAtuador() : descritorSocket(-1), ligado(false) {}
};

class RegistroAtuadores {
public:
    // Registra o socket de um atuador recem conectado e identificado, comecando desligado
    void registrarAtuador(const string& idAtuador, int descritorSocket);

    // remove um atuador do registro, chamado quando a conexao cai
    void removerAtuador(const string& idAtuador);

    /*
    Busca o socket de um atuador pelo id
    encontrado indica se o atuador estava registrado
    retorna -1 se nao encontrado
    */
    int obterSocket(const string& idAtuador, bool& encontrado);

    // Verifica se o atuador esta ligado atualmente, usado pra decidir se precisa mandar um CMD novo
    bool estaLigado(const string& idAtuador, bool& encontrado);

    // Atualiza o estado ligado ou desligado guardado pra esse atuador
    void definirEstadoLigado(const string& idAtuador, bool ligado);

private:
    map<string, EstadoAtuador> atuadores;
    mutex mutexAcesso;
};

}

#endif