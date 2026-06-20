# Makefile do projeto Estufa Inteligente, protocolo PLANT
# compila os cinco binarios do sistema: gerenciador, sensor, atuador, cliente e orquestrador
#
# uso
#   make           compila todos os binarios
#   make limpar    remove os binarios e objetos gerados

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# arquivos comuns usados por todos os processos
FONTES_COMUNS = common/plant_message.cpp common/socket_utils.cpp common/log_thread_safe.cpp

# arquivos exclusivos do gerenciador
FONTES_GERENCIADOR = gerenciador/main.cpp gerenciador/connection_handler.cpp gerenciador/sensor_registry.cpp gerenciador/actuator_registry.cpp

todos: bin gerenciador sensor atuador cliente orquestrador

bin:
	mkdir -p bin

gerenciador: $(FONTES_GERENCIADOR) $(FONTES_COMUNS)
	$(CXX) $(CXXFLAGS) -o bin/gerenciador $(FONTES_GERENCIADOR) $(FONTES_COMUNS)

sensor: sensor/main.cpp $(FONTES_COMUNS)
	$(CXX) $(CXXFLAGS) -o bin/sensor sensor/main.cpp $(FONTES_COMUNS)

atuador: atuador/main.cpp $(FONTES_COMUNS)
	$(CXX) $(CXXFLAGS) -o bin/atuador atuador/main.cpp $(FONTES_COMUNS)

cliente: cliente/main.cpp $(FONTES_COMUNS)
	$(CXX) $(CXXFLAGS) -o bin/cliente cliente/main.cpp $(FONTES_COMUNS)

orquestrador: orquestrador/main.cpp orquestrador/orquestrador.cpp common/device_ids.h common/log_thread_safe.cpp
	$(CXX) $(CXXFLAGS) -o bin/orquestrador orquestrador/main.cpp orquestrador/orquestrador.cpp common/log_thread_safe.cpp

limpar:
	rm -rf bin

.PHONY: todos limpar