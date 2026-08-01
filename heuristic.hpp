#include <fstream>

/* parametros do algoritmo genetico elaborado */
#define PROB_CROSSOVER 0.7
#define PROB_MUTACAO 0.7 //aumentei aqui de acordo com a primeira apresentacao e sugestao do heder
#define POP_SIZE 50
#define K_TORNEIO 2

#define FL_ESTAGNACAO true
#define FL_OPT_2 false
#define LIMITE_ESTAGNACAO 1000

struct solution{
  int *tour;	//this is what the fitness_evaluation function in EVRP.hpp will evaluate
  int id;
  double tour_length; //quality of the solution
  int steps; //size of the solution
  //the format of the solution is as follows:
  //*tour:  0 - 5 - 6 - 8 - 0 - 1 - 2 - 3 - 4 - 0 - 7 - 0
  //*steps: 12
  //this solution consists of three routes: 
  //Route 1: 0 - 5 - 6 - 8 - 0
  //Route 2: 0 - 1 - 2 - 3 - 4 - 0
  //Route 3: 0 - 7 - 0
};


extern solution *best_sol;


void initialize_heuristic();
void run_heuristic(std::ofstream &arquivo_run, std::ofstream &arquivo_evo_caminho);



void free_heuristic();