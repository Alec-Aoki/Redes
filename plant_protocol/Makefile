# Makefile do projeto Estufa Inteligente, protocolo PLANT
# compila os quatro binarios do sistema: gerenciador, sensor, atuador e cliente
#
# uso
#   make           compila todos os binarios
#   make limpar    remove os binarios e objetos gerados

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# arquivos comuns usados por todos os processos
FONTES_COMUNS = common/plant_message.cpp common/socket_utils.cpp

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

orquestrador: orquestrador/main.cpp orquestrador/orquestrador.cpp common/device_ids.h
	$(CXX) $(CXXFLAGS) -o bin/orquestrador orquestrador/main.cpp orquestrador/orquestrador.cpp

limpar:
	rm -rf bin

.PHONY: todos limpar
