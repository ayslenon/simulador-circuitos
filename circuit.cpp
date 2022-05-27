#include <fstream>
#include "circuit.h"

///
/// As strings que definem os tipos de porta
///

// Funcao auxiliar que testa se uma string com nome de porta eh valida
// Caso necessario, converte os caracteres da string para maiusculas
bool validType(std::string& Tipo)
{
  if (Tipo.size()!=2) return false;
  Tipo.at(0) = toupper(Tipo.at(0));
  Tipo.at(1) = toupper(Tipo.at(1));
  if (Tipo=="NT" || Tipo=="AN" || Tipo=="NA" ||
      Tipo=="OR" || Tipo=="NO" ||
      Tipo=="XO" || Tipo=="NX") return true;
  return false;
}

// Funcao auxiliar que retorna um ponteiro que aponta para uma porta alocada dinamicamente
// O tipo da porta alocada depende do parametro string de entrada (AN, OR, etc.)
// Caso o tipo nao seja nenhum dos validos, retorna nullptr
// Pode ser utilizadas nas funcoes: Circuit::setPort, Circuit::digitar e Circuit::ler
ptr_Port allocPort(std::string& Tipo)
{
  if (!validType(Tipo)) return nullptr;

  if (Tipo=="NT") return new Port_NOT;
  if (Tipo=="AN") return new Port_AND;
  if (Tipo=="NA") return new Port_NAND;
  if (Tipo=="OR") return new Port_OR;
  if (Tipo=="NO") return new Port_NOR;
  if (Tipo=="XO") return new Port_XOR;
  if (Tipo=="NX") return new Port_NXOR;

  // Nunca deve chegar aqui...
  return nullptr;
}

///
/// CLASSE CIRCUIT
///

/// ***********************
/// Inicializacao e finalizacao
/// ***********************

// As variaveis do tipo Circuit sao sempre criadas sem nenhum dado
// A definicao do numero de entradas, saidas e ports eh feita ao ler do teclado ou arquivo
// ou ao executar o metodo resize
Circuit::Circuit(){}

// Construtor por copia
// Nin e os vetores id_out e out_circ serao copias dos equivalentes no Circuit C
// O vetor ports terah a mesma dimensao do equivalente no Circuit C
// Serah necessario utilizar a funcao virtual clone para criar copias das portas
Circuit::Circuit(const Circuit& C){
    clear();
    Nin = C.Nin;
    for (unsigned int i = 0; i < C.id_out.size(); i++){
        id_out.push_back(C.id_out[i]);
        out_circ.push_back(C.out_circ[i]);
    }
    for (unsigned int i = 0; i < C.ports.size(); i++){
        ports.push_back(C.ports[i]->clone());
    }
}

// Destrutor: apenas chama a funcao clear()
Circuit::~Circuit() {clear();}

// Limpa todo o conteudo do circuito. Faz Nin <- 0 e
// utiliza o metodo STL clear para limpar os vetores id_out, out_circ e ports
// ATENCAO: antes de dar um clear no vetor ports, tem que liberar (delete) as areas
// de memoria para as quais cada ponteiro desse vetor aponta.
void Circuit::clear(){
    Nin = 0;
    id_out.clear();
    out_circ.clear();
    for(unsigned int i = 0; i < ports.size(); i++){
        if (ports[i]!= NULL) delete ports[i];
    }
    ports.clear();
}

// Operador de atribuicao
// Atribui (faz copia) de Nin e dos vetores id_out e out_circ
// ATENCAO: antes de alterar o vetor ports, tem que liberar (delete) as areas
// de memoria para as quais cada ponteiro desse vetor aponta.
// O vetor ports terah a mesma dimensao do equivalente no Circuit C
// Serah necessario utilizar a funcao virtual clone para criar copias das portas
void Circuit::operator=(const Circuit& C){
    clear();
    Nin = C.Nin;
    for (unsigned int i = 0; i < C.id_out.size(); i++){
        id_out.push_back(C.id_out[i]);
        out_circ.push_back(C.out_circ[i]);
    }
    for (unsigned int i = 0; i < C.ports.size(); i++){
        ports.push_back(C.ports[i]->clone());
    }
}

// Redimensiona o circuito para passar a ter NI entradas, NO saidas e NP ports
// Inicialmente checa os parametros. Caso sejam validos,
// depois de limpar conteudo anterior (clear), altera Nin; os vetores tem as dimensoes
// alteradas (resize) e sao inicializados com valores iniciais neutros ou invalidos:
// id_out[i] <- 0
// out_circ[i] <- UNDEF
// ports[i] <- nullptr
void Circuit::resize(unsigned NI, unsigned NO, unsigned NP){
    if(NI > 0 && NO > 0 && NP > 0){
        clear();
        Nin = NI;
        id_out.resize(NO);
        out_circ.resize(NO);
        ports.resize(NP);
        for (unsigned int i = 0; i < id_out.size(); i++){
            id_out[i] = 0;
            out_circ[i] = bool3S::UNDEF;
        }
        for (unsigned int i = 0; i < ports.size(); i++) ports[i] = nullptr;
    }
}

/// ***********************
/// Funcoes de testagem
/// ***********************

// Retorna true se IdInput eh uma id de entrada do circuito valida (entre -1 e -NInput)
bool Circuit::validIdInput(int IdInput) const
{
  return (IdInput<=-1 && IdInput>=-int(getNumInputs()));
}

// Retorna true se IdOutput eh uma id de saida do circuito valida (entre 1 e NOutput)
bool Circuit::validIdOutput(int IdOutput) const
{
  return (IdOutput>=1 && IdOutput<=int(getNumOutputs()));
}

// Retorna true se IdPort eh uma id de porta do circuito valida (entre 1 e NPort)
bool Circuit::validIdPort(int IdPort) const
{
  return (IdPort>=1 && IdPort<=int(getNumPorts()));
}

// Retorna true se IdOrig eh uma id valida para a origem do sinal de uma entrada de porta ou
// para a origem de uma saida do circuito (podem vir de uma entrada do circuito ou de uma porta)
// validIdOrig == validIdInput OR validIdPort
bool Circuit::validIdOrig(int IdOrig) const
{
  return validIdInput(IdOrig) || validIdPort(IdOrig);
}

// Retorna true se IdPort eh uma id de porta valida (validIdPort) e
// a porta estah definida (estah alocada, ou seja, != nullptr)
bool Circuit::definedPort(int IdPort) const
{
  if (!validIdPort(IdPort)) return false;
  if (ports.at(IdPort-1)==nullptr) return false;
  return true;
}

// Retorna true se IdPort eh uma porta existente (definedPort) e
// todas as entradas da porta com Id de origem valida (usa getId_inPort e validIdOrig)
bool Circuit::validPort(int IdPort) const
{
  if (!definedPort(IdPort)) return false;
  for (unsigned j=0; j<getNumInputsPort(IdPort); j++)
  {
    if (!validIdOrig(getId_inPort(IdPort,j))) return false;
  }
  return true;
}

// Retorna true se o circuito eh valido (estah com todos os dados corretos):
// - numero de entradas, saidas e portas valido (> 0)
// - todas as portas validas (usa validPort)
// - todas as saidas com Id de origem validas (usa getIdOutput e validIdOrig)
// Essa funcao deve ser usada antes de salvar ou simular um circuito
bool Circuit::valid() const
{
  if (getNumInputs()==0) return false;
  if (getNumOutputs()==0) return false;
  if (getNumPorts()==0) return false;
  for (unsigned i=0; i<getNumPorts(); i++)
  {
    if (!validPort(i+1)) return false;
  }
  for (unsigned i=0; i<getNumOutputs(); i++)
  {
    if (!validIdOrig(getIdOutput(i+1))) return false;
  }
  return true;
}

/// ***********************
/// Funcoes de consulta
/// ***********************

// Caracteristicas do circuito

// Retorna o tamanho (size) dos vetores correspondentes:
// inputs, id_out e ports, respectivamente
unsigned Circuit::getNumInputs() const {return Nin;}
unsigned Circuit::getNumOutputs() const {return id_out.size();}
unsigned Circuit::getNumPorts() const {return ports.size();}

// Caracteristicas das saidas do circuito

// Retorna a origem (a id) do sinal de saida cuja id eh IdOutput
// Depois de testar o parametro (validIdOutput), retorna id_out[IdOutput-1]
// ou 0 se parametro invalido
int Circuit::getIdOutput(int IdOutput) const{
    if (validIdOutput(IdOutput)) return id_out[IdOutput-1];
    return 0;
}

// Retorna o valor logico atual da saida cuja id eh IdOutput
// Depois de testar o parametro (validIdOutput), retorna out_circ[IdOutput-1]
// ou bool3S::UNDEF se parametro invalido
bool3S Circuit::getOutput(int IdOutput) const{
    if (validIdOutput(IdOutput)) return out_circ[IdOutput-1];
    return bool3S::UNDEF;
}

// Caracteristicas das portas

// Retorna o nome da porta: AN, NX, etc
// Depois de testar se a porta existe (definedPort),
// retorna ports[IdPort-1]->getName()
// ou "??" se parametro invalido
std::string Circuit::getNamePort(int IdPort) const{
    if (definedPort(IdPort)) return ports[IdPort-1]->getName();
    return "??";
}

// Retorna o numero de entradas da porta
// Depois de testar se a porta existe (definedPort),
// retorna ports[IdPort-1]->getNumInputs()
// ou 0 se parametro invalido
unsigned Circuit::getNumInputsPort(int IdPort) const{
    if (definedPort(IdPort)) return ports[IdPort-1]->getNumInputs();
    return 0;
}

// Retorna a origem (a id) da I-esima entrada da porta cuja id eh IdPort
// Depois de testar se a porta existe (definedPort) e o indice da entrada I,
// retorna ports[IdPort-1]->getId_in(I)
// ou 0 se parametro invalido
int Circuit::getId_inPort(int IdPort, unsigned I) const{
    if (I > getNumInputsPort(IdPort) || getNumInputsPort(IdPort) == 0) return 0;
    return ports[IdPort-1]->getId_in(I);
}

/// ***********************
/// Funcoes de modificacao
/// ***********************

void Circuit::setIdOutput(int IdOut, int IdOrig){
    if (validIdOutput(IdOut) && validIdOrig(IdOrig)) id_out[IdOut-1] = IdOrig;
}

void Circuit::setPort(int IdPort, std::string Tipo, unsigned NIn){
    if (validIdPort(IdPort)){
        if (validType(Tipo)){
            ptr_Port prov = allocPort(Tipo);
            if (prov->validNumInputs(NIn)){
                delete ports[IdPort-1];
                ports[IdPort-1] = prov;
                ports[IdPort-1]->setNumInputs(NIn);
                delete prov;
            }
        }
    }
}

// Altera a origem da I-esima entrada da porta cuja id eh IdPort, que passa a ser "IdOrig"
// Depois de VARIOS testes (definedPort, validIndex, validIdOrig)
// faz: ports[IdPort-1]->setId_in(I,Idorig)
void Circuit::setId_inPort(int IdPort, unsigned I, int IdOrig) const{
    if (definedPort(IdPort)){
        if(ports[IdPort]->validIndex(I)){
            if(validIdOrig(IdOrig))
                ports[IdPort-1]->setId_in(I, IdOrig);
        }
    }
}

/// ***********************
/// E/S de dados
/// ***********************

// Entrada dos dados de um circuito via teclado
// O usuario digita o numero de entradas, saidas e portas
// apos o que, se os valores estiverem corretos (>0), redimensiona o circuito
// Em seguida, para cada porta o usuario digita o tipo (NT,AN,NA,OR,NO,XO,NX) que eh conferido
// Apos criada dinamicamente (new) a porta do tipo correto, chama a
// funcao digitar na porta recem-criada. A porta digitada eh conferida (validPort).
// Em seguida, o usuario digita as ids de todas as saidas, que sao conferidas (validIdOrig).
// Se o usuario digitar um dado invalido, o metodo deve pedir que ele digite novamente
// Deve utilizar o metodo digitar da classe Port
void Circuit::digitar(){
    unsigned int NIn = 0, NOut = 0, NPort = 0;

    std::string PortType = "";
    do{
        std::cout << "Escreva o numero de entradas do circuito: ";
        std::cin >> NIn;
    }while(Nin < 0);
    Nin = NIn;
    do{
        std::cout << "Escreva o numero de saidas do circuito: ";
        std::cin >> NOut;
    }while(NOut < 0);
    id_out.resize(NOut, 0);
    out_circ.resize(NOut, bool3S::UNDEF);
    do{
        std::cout << "Escreva o numero de portas logicas do circuito: ";
        std::cin >> NPort;
    }while(NPort < 0);
    ports.clear();
    for (unsigned int i = 0; i < NPort; i++){
        do{
            std::cout << "Digite o tipo da porta " <<i+1<< "\n";
            std::cout << "NT | AN | NA | OR | NO | XO | NX: ";
            std::cin >> PortType;
            ptr_Port prov = allocPort(PortType);
            if(prov != nullptr){
                ports.push_back(prov);
                ports[i]->digitar();
            }
        }while(!validPort(i+1));
    }
    for (unsigned int i = 0; i < NOut; i++){
        do{
            std::cout <<"Para a saida " << i+1 << " digite a fonte de origem do sinal: ";
            std::cin >> id_out[i];
        }while(!validIdOrig(id_out[i]));
    }
}

// Entrada dos dados de um circuito via arquivo
// Leh do arquivo o cabecalho com o numero de entradas, saidas e portas
// apos o que, se os valores estiverem corretos (>0), redimensiona o circuito
// Em seguida, para cada porta leh e confere a id e o tipo (NT,AN,NA,OR,NO,XO,NX)
// Apos criada dinamicamente (new) a porta do tipo correto, chama a
// funcao ler na porta recem-criada. A porta lida eh conferida (validPort).
// Em seguida, leh as ids de todas as saidas, que sao conferidas (validIdOrig).
// Retorna true se deu tudo OK; false se deu erro.
// Deve utilizar o metodo ler da classe Port
bool Circuit::ler(const std::string& arq){
    std::ifstream arquivo;
    std::string prov;
    unsigned int NIn, NOut, NPort, portID;
    ptr_Port portP;

    try{
        arquivo.open(arq, std::fstream::in);

        arquivo >> prov;
        if (prov == "CIRCUITO"){
            arquivo >> prov;
            if(prov != ":"){
                std::cout << "Separador ':' nao encontrado\n\n";
                std::cout << "\n" << prov << "\n";
                clear();
                return false;
            }
        }
        else if (prov != "CIRCUITO:") {
            std::cout << "Paravra chave CIRCUITO nao encontrada\n\n";
            std::cout << "\n" << prov << "\n";
            clear();
            return false;
        }
        arquivo >> NIn >> NOut >> NPort;
        if (NIn <= 0 || NOut <= 0 || NPort <= 0){
            std::cout << "Numero de entradas | saidas | portas negativo ou zero\n\n";
            clear();
            return false;
        }
        arquivo >> prov;
        if (prov == "PORTAS"){
            arquivo >> prov;
            if(prov != ":"){
                std::cout << "Separador ':' nao encontrado\n\n";
                std::cout << "\n" << prov << "\n";
                clear();
                return false;
            }
        }
        else if (prov != "PORTAS:") {
            std::cout << "Palavra chave PORTAS nao encontrada\n";
            std::cout << "\n" << prov << "\n";
            clear();
            return false;
        }
        resize(NIn, NOut, NPort);

        for (unsigned int i = 0; i < NPort; i++){
            arquivo >> portID;

            if (portID != i+1){
                std::cout << "Id de porta esperado nao encontrado\n";
                std::cout << "\n" << portID << "\n";
                clear();
                return false;
            }
            arquivo >> prov;

            if (prov != ")"){
                std::cout << "Caractere ')' nao encontrado\n";
                std::cout << "\n" << prov << "\n";
                clear();
                return false;
            }
            arquivo >> prov;

            if (!validType(prov)){
                std::cout << "Tipo de porta valido nao encontrado\n\n";
                std::cout << "\n" << prov << "\n";
                clear();
                return false;
            }
            portP = allocPort(prov);
            if (!portP->ler(arquivo)){
                std::cout << "Falha na leitura da porta\n\n";
                clear();
                return false;
            }
            if(!portP->valid()){
                std::cout << "Porta com entradas invalidas\n\n";
                clear();
                return false;
            }
            ports.at(i)=portP;
        }
        for(unsigned int j = 0; j < getNumPorts(); j++){
            for(unsigned int i = 0; i < (ports.at(j)->getNumInputs()); i++){
                if(!validIdOrig(ports[j]->getId_in(i))){
                    std::cout << "Porta de sinal de origem invalido\n\n";
                    clear();
                    return false;
                }
            }
        }
        arquivo >> prov;
        if (prov == "SAIDAS"){
            arquivo >> prov;
            if(prov != ":"){
                std::cout << "Separador ':' nao encontrado\n\n";
                std::cout << "\n" << prov << "\n";
                clear();
                return false;
            }
        }
        else if (prov != "SAIDAS:"){
            std::cout << "Palavra chave SAIDAS nao encontrada\n\n";
            clear();
            return false;
        }
        unsigned int outID, outSignalID;
        for (unsigned int i = 0; i < NOut; i++){
            arquivo >> outID;
            if (outID != i+1){
                std::cout << "Id de saida esperado nao encontrado\n\n";
                clear();
                return false;
            }
            arquivo >> prov;
            if (prov != ")"){
                std::cout << "Caractere ')' nao encontrado\n\n";
                clear();
                return false;
            }
            arquivo >> outSignalID;
            if (!validIdOrig(outSignalID)){
                std::cout << "Sinal de origem invalido\n\n";
                clear();
                return false;
            }
            id_out.at(i) = outSignalID;
        }
        std::cout<<"arquivo lido com sucesso\n\n";
        arquivo.close();
        return true;
    }
    catch(std::ifstream::failure e){
        std::cerr << "erro ao ler arquivo\n\n";
        return false;
    }
}

// Saida dos dados de um circuito (em tela ou arquivo, a mesma funcao serve para os dois)
// Imprime os cabecalhos e os dados do circuito, caso o circuito seja valido
// Deve utilizar os metodos de impressao da classe Port
std::ostream& Circuit::imprimir(std::ostream& O) const{
    O << "CIRCUITO: " << getNumInputs() << ' ' << getNumOutputs() << ' ' << getNumPorts();
    O << "\nPORTAS:";
    for(unsigned int i = 0; i < getNumPorts(); i++){
        O << "\n" << i+1 << ") ";
        ports.at(i)->imprimir(O);
    }
    O << "\nSAIDAS:";
    for(unsigned int i = 0; i < getNumOutputs(); i++){
        O << "\n" << i+1 << ") " << id_out.at(i);
    }
    return O;
}

// Salvar circuito em arquivo, caso o circuito seja valido
// Abre a stream, chama o metodo imprimir e depois fecha a stream
// Retorna true se deu tudo OK; false se deu erro
bool Circuit::salvar(const std::string& arq) const{
    std::ofstream arquivo;
    arquivo.open(arq, std::ofstream::out);
    try{
        imprimir(arquivo);
        arquivo.close();
        return true;
    }
    catch(std::ifstream::failure e){
        std::cerr << "erro ao salvar arquivo\n\n";
        return false;
    }
}

/// ***********************
/// SIMULACAO (funcao principal do circuito)
/// ***********************

// Calcula a saida das portas do circuito para os valores de entrada
// passados como parametro, caso o circuito e a dimensao da entrada sejam
// validos (caso contrario retorna false)
// A entrada eh um vetor de bool3S, com dimensao igual ao numero de entradas
// do circuito.
// Depois de simular todas as portas do circuito, calcula as saidas do
// circuito (out_circ <- ...)
// Retorna true se a simulacao foi OK; false caso deh erro
bool Circuit::simular(const std::vector<bool3S>& in_circ){
    bool tudo_def, alguma_def;
    std::vector<bool3S> in_port;
    // Entradas de uma porta

    // SIMULAÇÃO DAS PORTAS
    for (unsigned int i=0; i<getNumPorts(); i++){
        ports[i]->setOutput(bool3S::UNDEF);
    }

    do {
        tudo_def=true;
        alguma_def=false;

        for(unsigned int i=0; i<getNumPorts(); i++){
            in_port.resize(ports[i]->getNumInputs(), bool3S::UNDEF);
            if(ports[i]->getOutput()==bool3S::UNDEF){
                int id = 0;
                for(unsigned int j=0; j<(ports[i]->getNumInputs());j++){
                    // De onde vem a entrada?
                    id = ports[i]->getId_in(j);
                    // Obtém valor da entrada
                    if(id > 0){
                        // De outra porta
                        in_port[j] = ports[id-1]->getOutput();
                    }
                    else{
                        // De entrada do circuito
                        in_port[j] = in_circ[-id-1];
                    }
                }
                ports[i]->simular(in_port);

                if(ports[i]->getOutput()==bool3S::UNDEF){
                    tudo_def = false;
                }
                else{
                    alguma_def = true;
                }
            }
        }
    }while(!tudo_def && alguma_def);

    // DETERMINAÇÃO DAS SAÍDAS
    for(unsigned int j = 0; j < getNumOutputs(); j++){
        // De onde vem a saída?
        int id = id_out[j];
        // Obtem valor da saída
        if(id > 0){
            // De uma porta
            out_circ[j] = ports[id-1]->getOutput();
        }
        else {
            // De entrada do circuito
            out_circ[j] = in_circ[-id-1];
        }
    }

    return true;
}
