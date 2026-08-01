#include<cmath>
#include<iostream>
#include<algorithm>
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<cstring>
#include<math.h>
#include<fstream>
#include<limits.h>

#include "heuristic.hpp"
#include "EVRP.hpp"

using namespace std;

solution *best_sol = nullptr;   //see heuristic.hpp for the solution structure
bool primeira_execucao;
static int **populacao = nullptr;
static int contador;

static double *fitness_individuos = nullptr;
static int contador_estagnacao;
static double ultimo_best;

/*initialize the structure of your heuristic in this function*/
void initialize_heuristic(){

    // primeiro limpar os vetores dinamicos
    if (populacao != nullptr) {
        for (int i = 0 ; i < POP_SIZE; i++)
            delete[] populacao[i];
        delete[] populacao;
    }

    if (fitness_individuos != nullptr)
        delete[] fitness_individuos;

    if (best_sol != nullptr) {
        delete[] best_sol->tour;
        delete best_sol;
    }
    
    // depois instanciar os parametros referentes a nova run 
    best_sol = new solution;
    best_sol->tour = new int[NUM_OF_CUSTOMERS+2];
    best_sol->id = 1;
    best_sol->steps = 0;
    best_sol->tour_length = INT_MAX;
    
    primeira_execucao = true;
    populacao = new int*[POP_SIZE];
    fitness_individuos = new double[POP_SIZE];
    contador = -1;

    // parametros relacionados ao teste de reinicio qndo ha estagnacao
    contador_estagnacao = 0;
    ultimo_best = 1e10;
}


// /*implement your heuristic in this function*/
// void run_heuristic(){
  
//   /*generate a random solution for the random heuristic*/
//   int i,help, object, tot_assigned =0;
//   int *r;
//   double energy_temp = 0.0; 
//   double capacity_temp = 0.0;
//   int from, to, temp;
//   int charging_station;
  
//   r = new int[NUM_OF_CUSTOMERS+1];
//   //set indexes of objects
//   for(i = 1; i <= NUM_OF_CUSTOMERS; i++){
//     r[i-1]=i;

//   }
//   //randomly change indexes of objects
//   for(i = 0; i <= NUM_OF_CUSTOMERS; i++){
//     object = (int) ((rand()/(RAND_MAX+1.0)) * (double)(NUM_OF_CUSTOMERS-tot_assigned));
//     help = r[i];
//     r[i]=r[i+object];
//     r[i+object]=help;
//     tot_assigned++;
//   }

//   best_sol->steps = 0;
//   best_sol->tour_length = INT_MAX;
  
//   best_sol->tour[0] = DEPOT;
//   best_sol->steps++;

//   i = 0;
//   while(i < NUM_OF_CUSTOMERS) {
//     from = best_sol->tour[best_sol->steps-1];
//     to = r[i];
//     if((capacity_temp + get_customer_demand(to)) <= MAX_CAPACITY && energy_temp+get_energy_consumption(from,to) <= BATTERY_CAPACITY){
//         capacity_temp  += get_customer_demand(to);
//         energy_temp += get_energy_consumption(from,to);
//         best_sol->tour[best_sol->steps] = to;
//         best_sol->steps++;
//         i++;
//     } else if ((capacity_temp + get_customer_demand(to)) > MAX_CAPACITY){
//         capacity_temp = 0.0;
//         energy_temp = 0.0;
//         best_sol->tour[best_sol->steps] = DEPOT;
//         best_sol->steps++;
//     } else if (energy_temp+get_energy_consumption(from,to) > BATTERY_CAPACITY){
//        charging_station = rand() % (ACTUAL_PROBLEM_SIZE-NUM_OF_CUSTOMERS-1)+NUM_OF_CUSTOMERS+1;
//        if(is_charging_station(charging_station)==true){
//           energy_temp = 0.0;
//           best_sol->tour[best_sol->steps] =  charging_station;
//           best_sol->steps++;
//         }
//     } else {
//         capacity_temp = 0.0;
//         energy_temp = 0.0;
//         best_sol->tour[best_sol->steps] =  DEPOT;
//         best_sol->steps++;
//     }
//   }
 
//   //close EVRP tour to return back to the depot
//    if(best_sol->tour[best_sol->steps-1]!=DEPOT){
//      best_sol->tour[best_sol->steps] = DEPOT;
//      best_sol->steps++;
//    }

//   best_sol->tour_length = fitness_evaluation(best_sol->tour, best_sol->steps);


//   //free memory
//   delete[] r;
// }


void preenche_solucao_candidata(int *solucao_candidata) {
    int total_preenchido = 0, help = 0, object = 0;
    //set indexes of objects
    for(int i = 1; i <= NUM_OF_CUSTOMERS; i++){
      solucao_candidata[i-1] = i;
    }
    //randomly change indexes of objects
    for(int i = 0; i < NUM_OF_CUSTOMERS; i++){
        object = (int) ((rand()/(RAND_MAX+1.0)) * (double)(NUM_OF_CUSTOMERS-total_preenchido));
        help = solucao_candidata[i];
        solucao_candidata[i] = solucao_candidata[i+object];
        solucao_candidata[i+object] = help;
        total_preenchido++;
    }
}

void inicializa_populacao(int **populacao) {
  // para cada solução candidata (lista com os nodes) da população (lista de listas)
  for (int i = 0; i < POP_SIZE; i++) {
      // preencher a lista interna (solucao candidata) com os nós de maneira aleatória
      int *solucao_candidata = new int[NUM_OF_CUSTOMERS];
      preenche_solucao_candidata(solucao_candidata);
      populacao[i] = solucao_candidata;
  }
}

// double calcular_solucao_candidata(int *individuo) {
//     // solution *solucao = new solution;
//     int steps, tour_length, *tour = new int[NUM_OF_CUSTOMERS+2];

//     steps = 0;
//     tour_length = INT_MAX;
//     tour[0] = DEPOT;
//     steps++;

//     int i = 0, from = 0, to = 0;
//     while(i < NUM_OF_CUSTOMERS) {
//       from = tour[steps-1];
//       to = individuo[i];
//       tour[steps] = to;
//       steps++;
//       i++;
//     }
  
//     //close EVRP tour to return back to the depot
//     if(tour[steps-1]!=DEPOT){
//       tour[steps] = DEPOT;
//       steps++;
//     }

//     // solucao->tour_length = fitness_evaluation(solucao->tour, solucao->steps);

//     // return solucao->tour_length;

//     double fitness = fitness_evaluation(tour, steps);
//     delete[] tour;
//     return fitness;
// }

void armazenar_solucao_candidata(int **populacao) {
    for (int idx = 0; idx < POP_SIZE; idx++) {    
        // solution *solucao = new solution;
        int steps, tour_length, *tour = new int[NUM_OF_CUSTOMERS+2];

        steps = 0;
        tour_length = INT_MAX;
        tour[0] = DEPOT;
        steps++;

        int i = 0, from = 0, to = 0;
        while(i < NUM_OF_CUSTOMERS) {
        from = tour[steps-1];
        to = populacao[idx][i];
        tour[steps] = to;
        steps++;
        i++;
        }
    
        //close EVRP tour to return back to the depot
        if(tour[steps-1]!=DEPOT){
        tour[steps] = DEPOT;
        steps++;
        }

        // solucao->tour_length = fitness_evaluation(solucao->tour, solucao->steps);

        // return solucao->tour_length;

        double fitness = fitness_evaluation(tour, steps);
        delete[] tour;
        
        fitness_individuos[idx] = fitness;
    }
}

void atualizar_distancia_individuo(int idx, int *individuo) {
    int steps, tour_length, *tour = new int[NUM_OF_CUSTOMERS+2];

    steps = 0;
    tour_length = INT_MAX;
    tour[0] = DEPOT;
    steps++;

    int i = 0, from = 0, to = 0;
    while(i < NUM_OF_CUSTOMERS) {
        from = tour[steps-1];
        to = individuo[i];
        tour[steps] = to;
        steps++;
        i++;
    }

    //close EVRP tour to return back to the depot
    if(tour[steps-1]!=DEPOT){
    tour[steps] = DEPOT;
    steps++;
    }

    double fitness = fitness_evaluation(tour, steps);
    delete[] tour;
    
    fitness_individuos[idx] = fitness;
}

int* copia(int *pai) {
    int *filho = new int[NUM_OF_CUSTOMERS];
    for (int i = 0; i < NUM_OF_CUSTOMERS; i++) {
        filho[i] = pai[i];
    }
    return filho;
}

// int* selecao_torneio(int **populacao) {
//     // sortear K_TORNEIO individuos da populacao e escolher o melhor
//     int *idx = new int[K_TORNEIO];
//     double *func_obj = new double[K_TORNEIO];
//     for (int i = 0; i < K_TORNEIO; i++) {
//         idx[i] = rand() % POP_SIZE;
//         func_obj[i] = calcular_solucao_candidata(populacao[idx[i]]);
//     }
//     int idx_vencedor_torneio = 0;
//     for (int i = 0; i < K_TORNEIO - 1; i++) {
//         if (func_obj[i+1] < func_obj[idx_vencedor_torneio])
//             idx_vencedor_torneio = i+1;
//     }
//     int *vencedor_torneio = populacao[idx[idx_vencedor_torneio]];

//     delete[] idx;
//     delete[] func_obj;

//     return vencedor_torneio;
// }

int* selecao_torneio(int **populacao) {
    // sortear K_TORNEIO individuos da populacao e escolher o melhor
    int *idx = new int[K_TORNEIO];
    // double *func_obj = new double[K_TORNEIO];
    for (int i = 0; i < K_TORNEIO; i++) {
        idx[i] = rand() % POP_SIZE;
        // func_obj[i] = fitness_individuos(idx[i]);
    }
    int idx_vencedor_torneio = 0;
    for (int i = 0; i < K_TORNEIO - 1; i++) {
        if (fitness_individuos[idx[i+1]] < fitness_individuos[idx[idx_vencedor_torneio]])
            idx_vencedor_torneio = i+1;
    }
    int *vencedor_torneio = populacao[idx[idx_vencedor_torneio]];

    delete[] idx;
    // delete[] func_obj;

    return vencedor_torneio;
}

void mutacao_swap(int *filho) {
    int pos1, pos2;  
    pos1 = rand() % NUM_OF_CUSTOMERS;
    do {
        pos2 = rand() % NUM_OF_CUSTOMERS;
    } while (pos2 == pos1);
    int aux = filho[pos1];
    filho[pos1] = filho[pos2];
    filho[pos2] = aux;
}

void mutacao_inversao(int *filho) {
    int pos1, pos2;  
    pos1 = rand() % NUM_OF_CUSTOMERS;
    do {
        pos2 = rand() % NUM_OF_CUSTOMERS;
    } while (pos2 == pos1);
    if (pos2 < pos1)
        swap(pos1, pos2);
    while (pos1 < pos2) {
        int aux = filho[pos1];
        filho[pos1] = filho[pos2];
        filho[pos2] = aux;
        pos1++;
        pos2--;
    }

}

void crossover_PMX(int *pai1, int *pai2, int *filho) {
    for (int i = 0; i < NUM_OF_CUSTOMERS; i++)
        filho[i] = -1;
    // escolher aleatoriamente intervalo de pai1
    int inicio, fim;
    inicio = rand() % NUM_OF_CUSTOMERS;
    do {
        fim = rand() % NUM_OF_CUSTOMERS;
    } while (fim == inicio);
    if (fim < inicio)
        swap(fim, inicio);
    // copiar esse intervalo para o filho
    for (int i = inicio; i <= fim; i++) {
        filho[i] = pai1[i];
    }
    // iniciando no primeiro ponto do intervalo, verifique os elementos do pai2 que nao estao dentre os ja copiados
    for (int i2 = inicio; i2 <= fim; i2++) {
        bool elemento_ja_existe = false;
        for (int i1 = inicio; i1 <= fim; i1++) {
            if (pai2[i2] == filho[i1]) {
                elemento_ja_existe = true;
                break;
            }
        }
        if (!elemento_ja_existe) {
            int pos = i2;
            int elemento = pai1[pos];
            while (true) {
                int posicao_no_pai2 = -1;
                for (int j = 0; j < NUM_OF_CUSTOMERS; j++) {
                    if (pai2[j] == elemento) {
                        posicao_no_pai2 = j;
                        break;
                    }
                }
                if (posicao_no_pai2 < inicio || posicao_no_pai2 > fim) {
                    filho[posicao_no_pai2] = pai2[i2];
                    break;
                }
                else {
                    pos = posicao_no_pai2;
                    elemento = pai1[pos];
                }
            }
        }
    }
    for (int i = 0; i < NUM_OF_CUSTOMERS; i++) {
        if (filho[i] == -1)
            filho[i] = pai2[i];
    }
    // para cada elemento i dentre estes, veja no filho qual elemento j de pai1 foi copiado em seu lugar

    // copie i na posicao ocupada por j em pai2

    // se a posicao k ocupada por j em pai2 ja estiver preenchida no filho, entao coloque i na posicao ocupada por k

    // finalmente o restante do filho pode ser preenchido com os elementos de pai2

    // 
}

void crossover_ordenado(int *pai1, int *pai2, int *filho) {
    for (int i = 0; i < NUM_OF_CUSTOMERS; i++)
        filho[i] = -1;
    // escolher aleatoriamente intervalo de pai1
    int inicio, fim;
    inicio = rand() % NUM_OF_CUSTOMERS;
    do {
        fim = rand() % NUM_OF_CUSTOMERS;
    } while (fim == inicio);
    if (fim < inicio)
        swap(fim, inicio);
    bool *preenchido = new bool[NUM_OF_CUSTOMERS+1];
    for (int i = 0; i < NUM_OF_CUSTOMERS+1; i++)
        preenchido[i] = false;
    // copiar esse intervalo para o filho
    for (int i = inicio; i <= fim; i++) {
        filho[i] = pai1[i];
        preenchido[pai1[i]] = true;
    }
    int idx = (fim+1) % NUM_OF_CUSTOMERS;
    // preenche o filho com o restante dos genes do pai2 (voltando ao 1o elemento depois do ultimo)
    for (int i = 0; i < NUM_OF_CUSTOMERS; i++) {
        int val = pai2[(fim+1+i) % NUM_OF_CUSTOMERS];
        if (!preenchido[val]) {
            filho[idx] = val;
            preenchido[val] = true;
            idx = (idx+1) % NUM_OF_CUSTOMERS;
        }
    }
    delete[] preenchido;
}

void reinicia_populacao_estagnacao() {
    // apaga o que existe na populacao atual
    for (int i = 0 ; i < POP_SIZE; i++)
        delete[] populacao[i];
    // chama a funcao que inicia a populacao
    inicializa_populacao(populacao);
    // avaliar essa populacao
    armazenar_solucao_candidata(populacao);
}

void run_heuristic(ofstream &arquivo_run, ofstream &arquivo_evo_caminho){
    contador++;
                                            // printf("contador: %d\t",contador);
    int **nova_populacao = nullptr;
    // 1) inicializar a populacao (para a primeira iteracao)
    if (primeira_execucao == true) {
        // agora inicializa a população (de maneira randômica mesmo)
        inicializa_populacao(populacao);
        // agora avalia o fitness de cada individuo
        armazenar_solucao_candidata(populacao);
                                            // printf("---------------------DEPOIS---------------------");
                                            // if (contador == 0)
                                            //     for (int j = 0; j < POP_SIZE; j++) {
                                            //         printf("\n pop %d", j);
                                            //         for (int i = 0; i < NUM_OF_CUSTOMERS; i++)
                                            //             printf(" - %d - ", populacao[j][i]);
                                            //         printf("\n");
                                            //     }
                                            // printf("-------------------FIM------------------");
                                            // for (int i = 0; i < POP_SIZE; i++) {
                                            //     printf("pop: %d\n", i+1);
                                            //     for (int j = 0; j < NUM_OF_CUSTOMERS; j++)
                                            //         printf("%d ", populacao[i][j]);
                                            //     printf("FIM \n");
                                            // }
    } 
    // utiliza os operadores de algoritmos genéticos
    else {
        // para cada individuo da populacao antiga
        nova_populacao = new int*[POP_SIZE];
        armazenar_solucao_candidata(populacao);
        for (int i = 0; i < POP_SIZE; i+=2) {
            // 4) seleção: torneio
            int *pai1, *pai2, *filho1 = nullptr, *filho2 = nullptr;
            pai1 = selecao_torneio(populacao);
            pai2 = selecao_torneio(populacao);
            // avalia se terá ou nao recombinacao
            double prob_crossover = (double)rand() / RAND_MAX;
            if (prob_crossover < PROB_CROSSOVER) {
                // 5) crossover: PMX ou ordenado
                filho1 = new int[NUM_OF_CUSTOMERS];
                filho2 = new int[NUM_OF_CUSTOMERS];
                // crossover_PMX(pai1, pai2, filho1);
                // crossover_PMX(pai2, pai1, filho2);
                crossover_ordenado(pai1, pai2, filho1);
                crossover_ordenado(pai2, pai1, filho2);
            }
            else {
                filho1 = copia(pai1);
                filho2 = copia(pai2);
            }
            // 6) mutação: swap
            double prob_mutacao1 = (double)rand() / RAND_MAX, prob_mutacao2 = (double)rand() / RAND_MAX;
            if (prob_mutacao1 < (double) PROB_MUTACAO /*1.0 / POP_SIZE*/) {
                // mutacao_swap(filho1);
                mutacao_inversao(filho1);
            }
            if (prob_mutacao2 < (double) PROB_MUTACAO /*1.0 / POP_SIZE*/) {
                // mutacao_swap(filho2);
                mutacao_inversao(filho2);
            }
            nova_populacao[i] = filho1;
            if (i+1 < POP_SIZE) {
                nova_populacao[i+1] = filho2;
            }
            else {
                delete[] filho2;
            }
        }

    }
    // 2) avaliar fitness (de cada solucao candidata e obter a melhor)
    // para cada solucao candidata
    // double *distancia_solucao_candidata = new double[POP_SIZE];
    // int idx_melhor_solucao = 0;
    // distancia_solucao_candidata[0] = calcular_solucao_candidata(populacao[0]);
    // for (int i = 0; i < POP_SIZE - 1; i++) {
    //     // avaliar o fitness
    //     distancia_solucao_candidata[i+1] = calcular_solucao_candidata(populacao[i+1]);
    //     // delimitar a melhor
    //     if (distancia_solucao_candidata[i+1] < distancia_solucao_candidata[idx_melhor_solucao])
    //         idx_melhor_solucao = i+1;
    // }

    // double *distancia_solucao_candidata = new double[POP_SIZE];
    int idx_melhor_solucao = 0;
    // distancia_solucao_candidata[0] = fitness_individuos[0];
    for (int i = 0; i < POP_SIZE - 1; i++) {
        // avaliar o fitness
        // distancia_solucao_candidata[i+1] = fitness_individuos[i+1];
        // delimitar a melhor
        if (fitness_individuos[i+1] < fitness_individuos[idx_melhor_solucao])
            idx_melhor_solucao = i+1;
    }
    
    // 8) substituição por elitismo (se não for a primeira iteração)
    if (primeira_execucao != true) {
        // escolher o  melhor individuo da geracao anterior para a proxima geracao
        delete[] nova_populacao[idx_melhor_solucao];
        nova_populacao[idx_melhor_solucao] = populacao[idx_melhor_solucao];
        
        // substituindo o restante dos individuos na populacao
        for (int i = 0; i < POP_SIZE; i++) {
            if (i != idx_melhor_solucao)
                delete[] populacao[i];
            populacao[i] = nova_populacao[i];
        }
    }

    // TRABALHO 2: BUSCA LOCAL 2-OPT NO MELHOR INDIVIDUO
    if (FL_OPT_2) {
        int *individuo = populacao[idx_melhor_solucao];
        bool melhorou = true;
        while (melhorou) {
            melhorou = false;
            for (int i = 0; i < NUM_OF_CUSTOMERS - 1; i++) {
                for (int j = i + 1; j < NUM_OF_CUSTOMERS; j++) {
                    // clientes
                    int A = (i == 0) ? DEPOT : individuo[i - 1];
                    int B = individuo[i]; //from
                    int C = individuo[j]; //to
                    int D = (j == NUM_OF_CUSTOMERS - 1) ? DEPOT : individuo[j + 1];
                    // se distancia diminuiu, faz a troca
                    if (get_distance(A, C) + get_distance(B, D) + 1e-9 < get_distance(A, B) + get_distance(C, D)) {
                        int from = i, to = j;
                        while (from < to) {
                            int aux = individuo[from];
                            individuo[from] = individuo[to];
                            individuo[to] = aux;
                            from++; to--;
                        }
                        melhorou = true;
                    }
                }
            }
        }
        // atualiza a distancia do melhor individuo
        atualizar_distancia_individuo(idx_melhor_solucao, individuo);
    }



    // avaliar a populacao apos a substituicao
    // armazenar_solucao_candidata(populacao); // vou tirar pq tava comendo muitas avaliacoes

    // atualizar a best_sol para a main ler
    if (fitness_individuos[idx_melhor_solucao] < best_sol->tour_length) {
        best_sol->tour_length = fitness_individuos[idx_melhor_solucao];
        best_sol->steps = 0;
        best_sol->tour[best_sol->steps] = DEPOT;
        best_sol->steps++;
        for (int i = 0; i < NUM_OF_CUSTOMERS; i++) {
            best_sol->tour[best_sol->steps] = populacao[idx_melhor_solucao][i];
            best_sol->steps++;
        }
        if (best_sol->tour[best_sol->steps - 1] != DEPOT) {
            best_sol->tour[best_sol->steps] = DEPOT;
            best_sol->steps++;
        }
        // tentando guardar a evolucao das rotas em um arquivo separado para visualizacao
        if(arquivo_evo_caminho.is_open()) {
            arquivo_evo_caminho << "ger=" << contador << ";len=" << best_sol->tour_length << ";seq=";
            for (int i = 0; i < best_sol->steps; i++) {
                arquivo_evo_caminho << best_sol->tour[i];
                if (i < best_sol->steps - 1)
                    arquivo_evo_caminho << "-";
            }
            arquivo_evo_caminho << "\n";
        }
    }

                                            // if (contador % 75000 == 0) {
                                            //     printf("contador: %d\n",contador);
                                            //     printf("best solution leght: %.2lf\n", best_sol->tour_length);
                                            //     printf("\tsteps: %d\n", best_sol->steps);
                                            //     printf("\tpath: \n");
                                            //     for (int i = 0; i < NUM_OF_CUSTOMERS; i++) {
                                            //         printf("%d -> ",best_sol->tour[i]);
                                            //     }
                                            //     printf("\n");
                                            // }

    // vou escrever a evolucao do melhor resultado por populacao pra acompanhar num grafico se preciso ajustar algo
    if(arquivo_run.is_open()){
        arquivo_run << "pop=" << contador << "," << "best=" << best_sol->tour_length << "\n";
    }

    // X) criterio de parada (está na main)

    // teste de reiniciar a populacao caso haja estagnacao
    if (FL_ESTAGNACAO) {
        // se houve melhora, continua evoluindo
        if (best_sol->tour_length + 1e-9 < ultimo_best) {
            ultimo_best = best_sol->tour_length;
            contador_estagnacao = 0;
        }
        // contar numero de avaliacoes sem evolucao
        else {
            contador_estagnacao++;
        }
        // se passar o limite de avaliacoes de estagnacao, reiniciar
        if (contador_estagnacao >= LIMITE_ESTAGNACAO) {
            // printf("\n----------- limite estagnou ------------\n");
            reinicia_populacao_estagnacao();
            contador_estagnacao = 0;
        }
    }

    // ajusta a flag de primeira geracao, apos seu fim
    if (primeira_execucao == true) {
        primeira_execucao = false;
    }

                                            // salvar na best solution
                                            // best_sol->tour = new int[NUM_OF_CUSTOMERS+1000];
                                            // best_sol->id = 1;
                                            // best_sol->steps = 0;
                                            // best_sol->tour_length = INT_MAX;

    // libera memoria
    // delete[] distancia_solucao_candidata;
    if (nova_populacao != nullptr) {
        delete[] nova_populacao;
    }
}


/*free memory structures*/
void free_heuristic(){
    delete[] best_sol->tour;
    delete best_sol;
    for (int i = 0; i < POP_SIZE; i++){
        delete[] populacao[i];
    }
    delete[] populacao;
    delete[] fitness_individuos;
}

