#include "actuator_registry.h"
using namespace std;

namespace plant {

void RegistroAtuadores::registrarAtuador(const string& idAtuador, int descritorSocket) {
    lock_guard<mutex> guarda(mutexAcesso);
    EstadoAtuador novoEstado;
    novoEstado.descritorSocket = descritorSocket;
    novoEstado.ligado = false; // Todo atuador comeca desligado ao se conectar
    atuadores[idAtuador] = novoEstado;
}

void RegistroAtuadores::removerAtuador(const string& idAtuador) {
    lock_guard<mutex> guarda(mutexAcesso);
    atuadores.erase(idAtuador);
}

int RegistroAtuadores::obterSocket(const string& idAtuador, bool& encontrado) {
    lock_guard<mutex> guarda(mutexAcesso);
    auto iterador = atuadores.find(idAtuador);
    if (iterador == atuadores.end()) {
        encontrado = false;
        return -1;
    }
    encontrado = true;
    return iterador->second.descritorSocket;
}

bool RegistroAtuadores::estaLigado(const string& idAtuador, bool& encontrado) {
    lock_guard<mutex> guarda(mutexAcesso);
    auto iterador = atuadores.find(idAtuador);
    if (iterador == atuadores.end()) {
        encontrado = false;
        return false;
    }
    encontrado = true;
    return iterador->second.ligado;
}

void RegistroAtuadores::definirEstadoLigado(const string& idAtuador, bool ligado) {
    lock_guard<mutex> guarda(mutexAcesso);
    auto iterador = atuadores.find(idAtuador);
    if (iterador != atuadores.end()) {
        iterador->second.ligado = ligado;
    }
}

}