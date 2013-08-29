/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * tsp.c
 * Author: Nathan Cochran
 * Analysis of Algorithms - TSP Project
 * Date: 8/28/2013
 *
 * Includes a variety of algorithms for solving the travelling
 * salesman problem: nearest neighbor, 2-opt, simulated anneal,
 * and a combination of 2-opt and simulated anneal.
 *
 * Use tsp -h for usage information
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>


//CONSTANTS:

//Limits and buffer sizes:
#define LINE_MAX 128
#define WORD_MAX 64
#define MAX_CITIES 32768 

//Control values for anneal/nathans_algorithm:
#define SATISFIED 10000
#define START_TEMP (avg_distance/40.0)
#define END_TEMP (avg_distance/10.0)

//Only for anneal:
#define DELTA_TEMP (.9999)


//STRUCTS:

//Information for a single city, as read in:
typedef struct city {
    int id;
    int x;
    int y;
} city;


//FUNCTION PROTOTYPES:

void get_options(int argc, char **argv);
void set_best(int distance, int *path);
void swap(int i, int j, int *array);
void copy_array(int *to, int *from, int len);
int get_list_of_cities(int *list);
void read_input(void);
city *read_city(char *line);
void print_distances(void);
void print_distance(int i, int j);
void print_cities(void);
void print_city(city *c);
void print_solution(void);
void calc_distances(int max_id);
int calc_distance(city *a, city *b);
int get_distance(int i, int j);
int calc_path_dist(int *path, int len);
void free_distances(void);
void nearest_neighbor(int *path, int len);
int swap_closest(int *remaining, int num_remaining);
void two_opt(int *path, int len);
void nathans_algorithm(int * path, int len);
void anneal(int *path, int len);
int anneal_accept(int new_dst, int old_dst, double temp);
double change_temp(double old_temp);
void two_opt_swap(int i, int j, int *path);
int two_opt_dist(int old_dist, int i, int j, int *path, int len);
void sig_handler(int sig);
void install_sig_handlers(void);
double get_max(double a, double b);


//STATIC VARIABLES:

//The list of cities and their coordinates:
static city * cities[MAX_CITIES];
static int num_cities;

//The matrix of distances between cities
//and the average distance between cities:
static int ** distances;
static int avg_distance;

//The optimal distance/path found thus far
//(printed on a SIGTERM or SIGINT):
static int best_distance;
static int * best_path;

//The command line options chosen:
static int use_anneal = 0;
static int use_nearest_neighbor = 0;
static int use_two_opt = 0;
static int verbose = 0;
static int debug = 0;

//The input and output options/filenames:
static int in_file = 0;
static char in_filename[WORD_MAX];
static int out_file = 0;
static char out_filename[WORD_MAX];

int main (int argc, char * argv[]) {
    int * path;
    int max_id;

    //Install the SIGINT/SIGTERM signal handlers:
    install_sig_handlers();

    //Get command line options:
    get_options(argc, argv);

    //Read input, get list of cities:
    if(verbose)
        printf("Reading input...\n");
    read_input();

    //Initialize variables to hold paths:
    best_path = malloc((num_cities) * sizeof(int));
    path = malloc((num_cities) * sizeof(int));

    //Get simple list of city ids into our working path:
    max_id = get_list_of_cities(path);

    //Get matrix of distances between cities:
    if(verbose)
        printf("Calculating distances...\n");
    calc_distances(max_id);

    //Call nearest_neighbor algorithm to get a good first approximation:
    if(verbose)
        printf("Calling nearest neighbor algorithm...\n");
    nearest_neighbor(path, num_cities);

    //Unless nearest neighbor is being used alone,
    //call another algorithm to improve the answer:
    if(!use_nearest_neighbor) {

        //Simulated Anneal:
        if(use_anneal) {
            if(verbose)
                printf("Calling anneal...\n");
            anneal(path, num_cities);
        }

        //Two-opt:
        else if(use_two_opt) {
            if(verbose)
                printf("Calling two-opt...\n");
            two_opt(path, num_cities);
        }

        //Default: Nathan's Algorithm 
        else {
            if(verbose)
                printf("Calling Nathan's algorithm...\n");
            nathans_algorithm(path, num_cities);
        }
    }

    //Print solution:
    print_solution();
    
    //Free memory allocated for distances matrix:
    free_distances();

    return EXIT_SUCCESS;
}



//NEAREST NEIGHBOR ALGORITHM:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Creates a path by progressively selecting the nearest neighbor to the last city added to the path
 * Param:   int * path -  Contains the list of cities to build the path from.  At completion, contains the newly created path
 * Param:   int len -  The length of the path (the number of cities)
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void nearest_neighbor(int * path, int len) {
    int i, dst;

    dst = 0;
    for(i=0; i<len-1; i++) {
        dst += swap_closest(&path[i], (len-i));
    }

    dst += get_distance(path[len-1], path[0]);
    set_best(dst, path);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Swaps into the second position in the list the city that is closest to the city in the first position in the list
 * Param:   int * remaining -  Pointer to a (section of) a list of cities
 * Param:   int num_remaining -  The number of cities remaining before the end of the list
 * Return:  int -  The distance from the city in the first position in the list to the city in the second position in the list
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int swap_closest(int * remaining, int num_remaining) {
    int i, cur, best;
    
    cur = remaining[0];
    best = 1;

    for(i=2; i<num_remaining; i++) {
        if(get_distance(cur, remaining[i]) < get_distance(cur, remaining[best])) {
            best = i;
        }
    }

    swap(1, best, remaining);
    return get_distance(cur, remaining[1]);
}


//2-OPT ALGORITHM:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Iteratively improves on a given path by swapping 2 edges at a time in order to create a shorter path
 * Param:   int * path -  The path to improve upon
 * Param:   int len -  The length of the path
 * Return:  void -  The improved path is left at the location specified by the path pointer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void two_opt(int * path, int len) {
    int i, j, dist;

    for(i=1; i<len; i++) {
        for(j=i; j<len; j++) {
            dist = two_opt_dist(best_distance, i, j, path, len);
            if(dist < best_distance) {
                if(debug) {
                    printf("Two-opt found new path with distance: %d\n", dist);
                }
                two_opt_swap(i, j, path);
                set_best(dist, path);
                i=1;
                break;
            }
        }
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Performs a 2-opt swap by reversing the section of the path found between indicies i and j
 * Param:   int i -  The first index of the section to reverse
 * Param:   int j -  The last index of the section to reverse
 * Param:   int * path -  The path to perform the swap on
 * Return:  void -  The swapped path is left in the location specified by the path pointer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void two_opt_swap(int i, int j, int * path) {
    while (i < j) {
        swap(i++, j--, path);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Returns the distance of the path that would result from the associated 2-opt swap without actually performing the swap
 * Param:   int old_dist -  The distance of the path being changed
 * Param:   int i -  The first index of the section to reverse
 * Param:   int j -  The last index of the section to reverse
 * Param:   int * path -  The path that the swap would be performed on
 * Param:   int len -  The length of the path
 * Return:  int -  The length of the path that would result from the associated 2-opt swap
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int two_opt_dist(int old_dist, int i, int j, int * path, int len) {
    int new_dist;
    
    if(j == len-1) {
        new_dist = old_dist - (get_distance(path[i-1], path[i]) + get_distance(path[j], path[0]));
        new_dist += get_distance(path[i-1], path[j]) + get_distance(path[i], path[0]);
    }
    else {
        new_dist = old_dist - (get_distance(path[i-1], path[i]) + get_distance(path[j], path[j+1]));
        new_dist += get_distance(path[i-1], path[j]) + get_distance(path[i], path[j+1]);
    }

    return new_dist;
}


//SIMULATED ANNEAL ALGORITHM:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Version of the simulated anneal algorithm
 * Param:   int * path -  The path to perform simulated anneal on
 * Param:   int len -  The length of the path
 * Return:  void -  The resulting path is left in the location specified by the path pointer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void anneal(int * path, int len) {
    int i, j, dst, swp_dst, attempt;
    double temp;

    //Seed random number generator:
    srand(time(NULL));

    //Get the current path's distance:
    dst = calc_path_dist(path, len);

    temp = START_TEMP;
    attempt = 0;
    while(attempt < SATISFIED) {

        //Try a random 2-opt swap:
        i = (rand() % (len-1)) + 1;
        j = i + (rand() % (len-i));
        swp_dst = two_opt_dist(dst, i, j, path, len);

        //If the result is acceptable:
        if(anneal_accept(swp_dst, dst, temp)) {
            if(debug) {
                printf("Anneal: temp: %f, old path: %d, new path : %d", temp, dst, swp_dst);
                if(swp_dst > dst) 
                    printf("\t < escape local optimum");
                printf("\n");
            }

            //Update the path and it's distance:
            two_opt_swap(i, j, path);
            dst = swp_dst;

            //Update the global best dst/path, if necessary:
            if(dst < best_distance)
                set_best(dst, path);

            //Reset attempt counter:
            attempt = 0;
        }

        //If the result is unacceptable:
        else
            attempt++;
            if(debug)
                printf("Decline #%d\n", attempt);

        //Decrease the temperature:
        temp = change_temp(temp);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Accepts or rejects a proposed change from a path with old_dst to a path with new_dst, given temperature
 * Param:   int new_dst -  The distance of the new path
 * Param:   int old_dst -  The distance of the old path
 * Param:   double temp -  The temperature
 * Return:  int -  1 if the move is accepted, 0 if not
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int anneal_accept(int new_dst, int old_dst, double temp) {
    double prob, q;

    if(new_dst == old_dst)
        return 0;

    prob = exp((old_dst - new_dst)/temp);
    q = rand() / (double) RAND_MAX;

    if(q < prob)
        return 1;
    else
        return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Takes a temperature and returns a decreased temperature
 * Param:   double old_temp -  The temperature to decrease
 * Return:  double -  The new, decreased temperature
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double change_temp(double old_temp) {
    if(old_temp > .01) {
        return old_temp*DELTA_TEMP;
    }
    return old_temp;
}


//NATHAN'S ALGORITHM:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * A combination of two_opt and simulated anneal algorithms
 * Param:   int * path -  The path to improve
 * Param:   int len -  The length of the path
 * Return:  void -  The resulting path is left in the location specified by the path pointer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void nathans_algorithm(int * path, int len) {
    int i, j, dst, swp_dst, change, best_change, term_cnt;
    double temp;
    
    //Get the current path's distance:
    dst = calc_path_dist(path, len);

    term_cnt = 0;
    temp = 0.01;

    do {
        change = 0;
        best_change = 0;

        //Iterate through endpoints to be swapped:
        for(i=1; i<len; i++) {
            for(j=i; j<len; j++) {

                //Get the path distance of the tentative two-opt swap
                //(without actually performing the swap):
                swp_dst = two_opt_dist(dst, i, j, path, len);

                //Check to see whether the new distance is acceptable:
                if(anneal_accept(swp_dst, dst, temp)) {

                    //Print debug information:
                    if(debug) {
                        printf("Two-opt2: temp: %f, old path: %d, new path : %d", temp, dst, swp_dst);
                        if(swp_dst > dst) 
                            printf("\t < up");
                        printf("\n");
                    }

                    //Make the swap:
                    two_opt_swap(i, j, path);
                    dst = swp_dst;
                    change = 1;
   
                    //If necessary, update the running best path/distance:
                    if(dst < best_distance) {
                        set_best(dst, path);
                        best_change = 1;
                        term_cnt=0;
                    }
                }
            }
        }

        //Increment the termination counter if there's been no change
        //in the best path throughout the entire last loop:
        if(!best_change) {
            term_cnt++;
        }

        //Boost the temperature if there was no change
        //at all throughout the entire last loop
        //(i.e. it's stuck in a local optimum):
        if(!change) {

            //The closer the algorithm is to termination,
            //the larger the temperature boost (getting desperate):
            
            temp = get_max(START_TEMP,  END_TEMP * ((double)term_cnt/SATISFIED));
        }

        //If there has been some change, put it on ice
        //(this will get down it back down to a local optimum):
        else {
            temp = .01;
        }

    } while(term_cnt<SATISFIED);
}




//UTILITIES:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Sets the static variables which hold the best path found thus far
 * Param:   int distance -  The distance of the new path
 * Param:   int * path -  The new path
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void set_best(int distance, int * path) {
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, NULL);

    if(verbose)
        printf("New best path found: %d\n", distance);
    best_distance = distance;
    copy_array(best_path, path, num_cities);

    sigprocmask(SIG_UNBLOCK, &set, NULL);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Swaps elements at indicies i and j in array
 * Param:   int i -  First index
 * Param:   int j -  Second index
 * Param:   int * array -  The array to perform the swap on
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void swap(int i, int j, int * array) {
    int temp;

    temp = array[i];
    array[i] = array[j];
    array[j] = temp;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copies the elements of one array into another array
 * Param:   int * to -  The array to copy elements to
 * Param:   int * from -  The array to copy elements from
 * Param:   int len -  The length of the array to copy
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void copy_array(int * to, int * from, int len) {
    int i=0;

    for(i=0; i<len; i++) {
        to[i] = from[i];
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Returns the max of the two values passed in
 * Param:   double a -  First element
 * Param:   double b -  Second element
 * Return:  double -  The max of the two elements
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double get_max(double a, double b) {
    if(a>b)
        return a;
    else
        return b;
}


//INPUT/OUTPUT: 


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Gets the command line options and sets the appropriate static variables representing them
 * Param:   int argc -  The length of the argument vector
 * Param:   char ** argv -  The argument vector
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void get_options(int argc, char ** argv) {
    char opt;

    while((opt = getopt(argc, argv, "adf:hntv")) != -1) {
        switch(opt) {
            case 'a':
                use_anneal = 1;
                break;
            case 'n':
                use_nearest_neighbor = 1;
                break;
            case 'f':
                in_file = 1;
                out_file = 1;
                strncpy(in_filename, optarg, WORD_MAX);
                snprintf(out_filename, WORD_MAX, "%s%s", in_filename, ".tour");
                break;
            case 't':
                use_two_opt = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'd':
                verbose = 1;
                debug = 1;
                break;
            case 'h':
            default:
                printf("Usage: %s -[adntv] -[f filename]\n", argv[0]);
                printf("Algorithms:\n");
                printf("\t-Default: Nathan's (honestly the best choice)\n");
                printf("\t-n: Nearest Neighbor (only)\n");
                printf("\t-t: Two-opt\n");
                printf("\t-a: Simulated Anneal\n");
                printf("Display modes:\n");
                printf("\t-v: Verbose (minor progress messages)\n");
                printf("\t-d: Debug (lots of detailed messages)\n");
                printf("Input/Output:\n");
                printf("\t-f: Specify file to use as input/source file\n");
                printf("\t    Note: this will result in a output file named [input file].tour\n");

                exit(EXIT_SUCCESS);
        }
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Gets a list of all cities into the array specfied by list.  Useful for setting up the initial path
 * Param:   int * list -  Location to store the list of cities
 * Return:  int -  The max id of all the cities
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_list_of_cities(int * list) {
    int i, max_id;
    max_id = cities[0]->id;

    for(i=0; i<num_cities; i++) {
        list[i] = cities[i]->id;
        if(list[i] > max_id) {
            max_id = list[i];
        }
    }
    return max_id;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads the input (from stdin or a file, depending on command line options) and stores it in the cities array
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void read_input(void) {
    FILE * f;
    char line[LINE_MAX];
    num_cities = 0;

    f = in_file ? fopen(in_filename, "r") : stdin;

    while(fgets(line, LINE_MAX, f) != NULL) {
        cities[num_cities++] = read_city(line);
        if(num_cities >= MAX_CITIES) {
            printf("Error: too many cities");
            exit(EXIT_SUCCESS);
        }
    }

    if(in_file)
        fclose(f);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Reads a line of input text into a city struct, returning a pointer to the city struct
 * Param:   char * line -  Line of text containing three integers: the city's id, x coordinate, and y coordinate
 * Return:  city * -  Pointer to a struct with the city's information
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
city * read_city(char * line) {
    city * c;

    c = malloc(sizeof(struct city));
    c->id = (int)strtol(line, &line, 10);
    c->x = (int)strtol(line, &line, 10);
    c->y = (int)strtol(line, &line, 10);

    return c;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prints out all the distances between all the cities
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_distances() {
    int i, j;
    for(i=0; i<num_cities; i++) {
        for(j=i+1; j<num_cities; j++) {
            print_distance(cities[i]->id, cities[j]->id);
        }
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prints the distance between the two cities represented by ids i and j
 * Param:   int i -  The id of the first city
 * Param:   int j -  The id of the second city
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_distance(int i, int j) {
    printf("Distance between %d and %d: %d\n", i, j, get_distance(i, j));
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prints a list of all cities and their coordinates
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_cities(void) {
    int i;
    for(i=0; i<num_cities; i++) {
        print_city(cities[i]);
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prints a single city's id and coordinates
 * Param:   city * c -  The city to print to stdout
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_city(city * c) {
    printf("City: %d, X: %d, Y: %d\n", c->id, c->x, c->y);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Prints the solution (to stdout or a file, depending on command line arguments) in standard format
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_solution(void) {
    int i;
    FILE * f;

    if(out_file)
        f = fopen(out_filename, "w");
    else
        f = stdout;

    fprintf(f, "%d\n", best_distance);
    for(i=0; i<num_cities; i++) {
        fprintf(f, "%d\n", best_path[i]);
    }

    if(out_file)
        fclose(f);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Calculates the distances between all cities, storing them in the static distances matrix
 * Param:   int max_id -  The max id of all cities
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void calc_distances(int max_id) {
    int i, j;
    unsigned long sum;

    sum=0;
    distances = malloc((max_id+1) * sizeof(int *));

    for(i=0; i<num_cities; i++) {
        distances[cities[i]->id] = malloc((max_id+1) * sizeof(int));

        for(j=i+1; j<num_cities; j++) {
            sum += distances[cities[i]->id][cities[j]->id] = calc_distance(cities[i], cities[j]);
        }
    }
    avg_distance = (sum/pow(max_id, 2));
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Calculates the distance between the specified cities
 * Param:   city * a -  The first city
 * Param:   city * b -  The second city
 * Return:  int -  The distance, rounded to the nearest integer, between the two cities
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int calc_distance(city * a, city * b) {
    int x_dif, y_dif;
    double distance;

    x_dif = a->x - b->x;
    y_dif = a->y - b->y;

    distance = sqrt(pow((double) x_dif, 2.0) + pow((double) y_dif, 2.0));

    return (int) round(distance);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Returns the distance between cities with ids i and j
 * Param:   int i -  The first city's id
 * Param:   int j -  The second city's id
 * Return:  int -  The distance between the two cities
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int get_distance(int i, int j) {
    if(i<j)
        return distances[i][j];
    else
        return distances[j][i];
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Calculates the distance of the specified path
 * Param:   int * path -  The path whose distance is desired
 * Param:   int len -  The length of the path
 * Return:  int -  The path's distance (including the return to the origin city)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int calc_path_dist(int * path, int len) {
    int i, dist;

    dist = 0;
    for(i=0; i<len-1; i++) {
        dist += get_distance(path[i], path[i+1]);
    }
    dist += get_distance(path[i], path[0]);
    return dist;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Frees the matrix of distances allocated by calc_distances
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void free_distances(void) {
    int i;

    for(i=0; i<num_cities; i++) {
        free(distances[(cities[i]->id)]);
    }
    free(distances);
}


//SIGNAL HANDLERS:


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Signal handler for SIGINT or SIGTERM signals.  Prints solution before exiting
 * Param:   int sig -  The signal received
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void sig_handler(int sig) {
    if(verbose)
        printf("Received signal %d: exiting...\n", sig);
    print_solution();
    exit(EXIT_SUCCESS);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Installs the signal handler for SIGINT and SIGTERM
 * Param:   void
 * Return:  void
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void install_sig_handlers(void) {
    struct sigaction siga;

    siga.sa_handler = sig_handler;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags = 0;

    sigaction(SIGTERM, &siga, NULL);
    sigaction(SIGINT, &siga, NULL);
}


