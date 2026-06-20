#include "log_thread_safe.h"
#include <iostream>
#include <mutex>
using namespace std;

namespace plant {

/*
Mutex estatico, compartilhado entre todas as chamadas de logThreadSafe
dentro do mesmo processo
Cada processo (gerenciador, sensor, atuador, cliente) tem o seu
proprio mutex, ja que mutex nao atravessa processos, mas dentro de um
mesmo processo todas as threads usam essa mesma instancia
*/
static mutex mutexConsole;

void logThreadSafe(const string& linha) {
    lock_guard<mutex> guarda(mutexConsole);
    cout << linha << "\n";
}

}