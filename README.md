# Estufa Inteligente, protocolo PLANT

Este repositório contém a implementação em sockets do protocolo PLANT
(Protocolo para Lavoura, com Atuadores, Nós e Telemetria), especificado
na Entrega 1.

## Integrantes

Dante Brito Lourenço
Juan Henriques Passos
Alec Campos Aoki

## SO e compilador utilizados

Linux nativo, g++ com suporte a C++17.

## Estrutura do projeto

```
plant_protocol/
  common/
    plant_message.h/cpp     construção e parsing das mensagens PLANT
    socket_utils.h/cpp        wrappers de socket TCP em POSIX
    log_thread_safe.h/cpp     escrita de log protegida por mutex
    device_ids.h               constantes do protocolo
  gerenciador/
    main.cpp                    loop de accept, uma thread por conexão
    connection_handler.h/cpp    despacho por tipo de dispositivo e histerese
    sensor_registry.h/cpp       estado dos sensores, protegido por mutex
    actuator_registry.h/cpp     estado dos atuadores, protegido por mutex
  sensor/
    main.cpp                    binário único parametrizado por linha de comando
  atuador/
    main.cpp                    binário único parametrizado por linha de comando
  cliente/
    main.cpp                    menu interativo no terminal
  orquestrador/
    main.cpp                    fork e exec de cada dispositivo
    orquestrador.h/cpp           lista default e parsing de argumentos
  Makefile
```

Cada componente do sistema (Gerenciador, Sensor, Atuador, Cliente) é
um processo separado no sistema operacional, conforme exigido na
especificação do trabalho. O Orquestrador não é um componente da
aplicação; é uma ferramenta auxiliar para facilitar a demonstração, e
está detalhada em sua própria seção mais abaixo.

## Compilando

Na raiz do projeto, basta rodar

```
make todos
```

Isso gera cinco binários dentro da pasta `bin/`:

```
bin/gerenciador
bin/sensor
bin/atuador
bin/cliente
bin/orquestrador
```

Para limpar os binários gerados:

```
make limpar
```

## Rodando o sistema

O Gerenciador atua como servidor passivo e precisa estar de pé antes
de qualquer outro processo se conectar. A partir daí, existem duas
formas de subir o restante do sistema.

### Forma recomendada: com o Orquestrador

```
terminal 1: ./bin/gerenciador
terminal 2: ./bin/orquestrador
terminal 3: ./bin/cliente
```

Sem argumentos, o Orquestrador sobe um exemplar de cada subtipo de
sensor e de atuador da estufa. Mais detalhes na seção "Orquestrador"
abaixo.

### Forma manual: um terminal por processo

Primeiro o Gerenciador:

```
./bin/gerenciador
```

Depois, em terminais separados, os sensores (subtipo e número como
argumentos):

```
./bin/sensor TERMO 01
./bin/sensor UMI 01
./bin/sensor CO2 01
```

Os atuadores, da mesma forma:

```
./bin/atuador AQUEC 01
./bin/atuador RESF 01
./bin/atuador IRR 01
./bin/atuador INJ 01
```

E o cliente, que abre um menu interativo:

```
./bin/cliente
```

Não há ordem obrigatória entre sensores, atuadores e cliente, desde
que o Gerenciador já esteja escutando na porta configurada (8080 por
padrão, ver `common/device_ids.h`).

## O que cada processo faz

**Gerenciador**: aceita conexões de sensores, atuadores e clientes,
cada uma tratada em uma thread separada. Guarda a última leitura de
cada sensor e os limites configurados pelo Cliente. Quando uma leitura
sai dos limites, aplica controle por histerese e manda CMD para o
atuador correspondente, sem repetir o comando se o atuador já estiver
no estado certo.

**Sensor**: conecta no Gerenciador, faz o handshake HELLO/ACK, e entra
em loop mandando DATA a cada 1 segundo, com o valor simulado por uma
pequena variação aleatória em torno da leitura anterior.

**Atuador**: conecta no Gerenciador, faz o handshake, e fica esperando
mensagens CMD, respondendo ACK para confirmar a mudança de estado.

**Cliente**: conecta no Gerenciador e oferece um menu para configurar
limites de um sensor (CONFIG) ou pedir a última leitura de um sensor
específico ou de todos (REQ_DATA).

## Decisões de implementação

Esta seção registra as escolhas de design que não estavam totalmente
fechadas na especificação do protocolo, e o raciocínio por trás de
cada uma.

### Threading no Gerenciador

O Gerenciador usa uma thread por conexão, em vez de um modelo baseado
em `select()` ou `poll()`. Para o número de dispositivos esperado em
uma demonstração da estufa, esse modelo é suficiente e mais simples de
explicar e depurar, ainda que escale pior do que um loop de eventos
único para um número muito grande de conexões simultâneas.

### Simulação dos sensores

Cada sensor mantém um valor interno que começa em um ponto de partida
plausível (22.0 para temperatura, 50.0 para umidade, 600.0 para CO2) e
soma uma variação aleatória pequena, entre -0.5 e 0.5, a cada leitura.
Optamos por essa abordagem ao invés de um padrão senoidal ou leitura de
arquivo externo porque o objetivo da simulação é apenas gerar uma série
de valores plausível, e não modelar com precisão o comportamento físico
de uma estufa real.

### Associação entre sensor e atuador

A relação entre qual atuador deve reagir a qual sensor foi decidida no código do Gerenciador como
uma associação fixa por tipo, e não por instância individual:

* TERMO controla AQUEC (liga abaixo do mínimo) e RESF (liga acima do
máximo).
* UMI controla IRR (liga abaixo do mínimo).
* CO2 controla INJ (liga abaixo do mínimo).

UMI e CO2 não possuem atuador de redução na especificação da estufa,
de modo que IRR e INJ apenas desligam quando a leitura volta a uma
faixa segura, sem que exista um atuador correspondente para o caso de
excesso.

### Controle por histerese

Para evitar que um atuador ligue e desligue repetidamente quando a
leitura oscila perto do limite configurado, foi adotada uma margem de
histerese de 1 unidade: o atuador liga assim que a leitura cruza o
limite, mas só desliga quando a leitura volta a uma distância segura
dentro do limite (não basta voltar a tocar o limite exato). Essa
margem está definida como constante `MARGEM_HISTERESE` em
`connection_handler.cpp`.

Para que o Gerenciador saiba se um CMD novo é realmente necessário, o
`RegistroAtuadores` guarda também o estado ligado ou desligado de cada
atuador, além do socket de conexão. Sem esse estado, o Gerenciador
mandaria CMD ON a cada leitura nova enquanto a variável estivesse fora
do limite, mesmo que o atuador já estivesse ligado desde a leitura
anterior.

### Log protegido por mutex

Como o Gerenciador e o Orquestrador escrevem na saída padrão a partir
de múltiplas threads simultaneamente, chamadas diretas a `std::cout`
podem ter seus caracteres intercalados entre threads diferentes,
produzindo linhas de log corrompidas. Para evitar isso, toda escrita
de log nesses dois processos passa pela função `logThreadSafe`, em
`common/log_thread_safe.cpp`, que protege a escrita com um mutex único
por processo.

### Flush imediato do buffer de saída

Todo processo chama `std::cout.setf(std::ios::unitbuf)` no início do
`main`, forçando o flush do buffer de saída após cada operação de
`cout`. Sem isso, a saída padrão usa buffer completo quando não está
conectada a um terminal interativo (por exemplo, quando redirecionada
para um arquivo), e as mensagens de log só apareceriam quando o buffer
enchesse ou o processo terminasse, o que atrapalharia a observação da
troca de mensagens em tempo real durante testes e demonstrações.

### Parsing de mensagens sobre TCP

Como o TCP entrega um fluxo contínuo de bytes sem preservar fronteiras
de mensagem, o parsing não pode confiar em delimitadores dentro do
payload. Por isso, a leitura de cada mensagem PLANT acontece em duas
etapas: primeiro lê-se byte a byte até encontrar a sequência `\n\n`,
que delimita o fim do header; em seguida, extrai-se o campo TAMANHO
desse header e lê-se exatamente essa quantidade de bytes de payload.
Essa lógica está concentrada em `socket_utils.cpp`, isolada dos
módulos de lógica de aplicação.

## Orquestrador

Abrir um terminal para cada sensor e cada atuador é viável, mas
cansativo na hora de demonstrar o sistema. O Orquestrador resolve esse
problema prático sem violar o requisito de processo separado: ele dá
fork e exec em cada sensor e atuador, de modo que cada um continua
sendo um processo de verdade no sistema operacional, com PID próprio
(visível em `ps aux`), mas a saída de todos eles fica concentrada em
um único terminal, prefixada com o nome do dispositivo de origem.

O Orquestrador não substitui sensor e atuador, apenas os inicia.
Continua sendo necessário que o Gerenciador já esteja rodando, como
qualquer outro dispositivo do sistema.

### Como rodar

Sem nenhum argumento, sobe uma lista fixa com um exemplar de cada
subtipo de sensor e de atuador da estufa:

```
./bin/orquestrador
```

Isso equivale a rodar, em terminais separados:

```
./bin/sensor TERMO 01
./bin/sensor UMI 01
./bin/sensor CO2 01
./bin/atuador AQUEC 01
./bin/atuador RESF 01
./bin/atuador IRR 01
./bin/atuador INJ 01
```

Para controlar exatamente quais dispositivos sobem, é possível passar
um argumento por dispositivo, no formato `tipo:subtipo:numero`:

```
./bin/orquestrador sensor:TERMO:01 atuador:AQUEC:01
```

Para encerrar todos os processos de uma vez, basta um `Ctrl+C` no
terminal onde o Orquestrador está rodando; o sinal é propagado para
todos os processos filhos automaticamente.