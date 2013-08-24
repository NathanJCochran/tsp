#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define LINE_MAX 128
#define WORD_MAX 64
#define MAX_CITIES 32768 

#define SATISFIED 10000
#define START_TEMP (avg_distance/8)
#define DELTA_TEMP (avg_distance/1000000.0)

//STRUCTS:
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
int calc_distances(int max_id);
int calc_distance(city *a, city *b);
int get_distance(int i, int j);
int calc_path_dist(int *path, int len);
void free_distances(void);
int greedy(int *path, int len);
int swap_closest(int *remaining, int num_remaining);
void two_opt(int *path, int len);
void anneal(int *path, int len);
int anneal_accept(int new_dst, int old_dst, double temp);
double change_temp(double old_temp);
void two_opt_swap(int i, int j, int *path);
int two_opt_dist(int old_dist, int i, int j, int *path, int len);

//STATIC VARIABLES:
static city * cities[MAX_CITIES];
static int num_cities;

static int ** distances;
static int avg_distance;

static int best_distance;
static int * best_path;

static int use_anneal = 0;
static int use_combo = 0;
static int use_greedy = 0;
static int verbose = 0;
static int debug = 0;

static int in_file = 0;
static char in_filename[WORD_MAX];
static int out_file = 0;
static char out_filename[WORD_MAX];

int main (int argc, char * argv[]) {
    int * path;
    int dst, max_id;

    //Get command line options:
    get_options(argc, argv);

    //Read input, get list of cities:
    if(verbose)
        printf("Reading input...\n");
    read_input();

    //Print list of cities:
    if(debug)
        print_cities();

    //Initialize static variable to hold paths:
    best_path = malloc((num_cities+1) * sizeof(int));
    path = malloc((num_cities+1) * sizeof(int));

    //Get simple list of city ids into cur_path;
    max_id = get_list_of_cities(path);

    //Get matrix of distances between cities:
    if(verbose)
        printf("Calculating distances...\n");
    avg_distance = calc_distances(max_id);

    //Print distances:
//    if(debug)
//        print_distances();

    //Call greedy algorithm to get a good first approximation:
    if(verbose)
        printf("Calling greedy algorithm...\n");
    dst = greedy(path, num_cities);
    set_best(dst, path);

    //Call two-opt/anneal to make it better:
    if(!use_greedy) {
        if(use_anneal) {
            if(verbose)
                printf("Calling anneal...\n");
            anneal(path, num_cities);
        }
        else if (use_combo) {
            if(verbose)
                printf("Calling combo anneal...\n");
            anneal(path, num_cities);
            copy_array(path, best_path, num_cities);
            two_opt(path, num_cities);
        }
        else{
            if(verbose)
                printf("Calling two-opt...\n");

            two_opt(path, num_cities);
        }
    }

    //Print solution:
    print_solution();
    
    //Free memory allocated for distances matrix:
    free_distances();

    return EXIT_SUCCESS;
}

/*
void brute_force(int start, int distance, int cur, int * remaining, int num_remaining) {
    int i, next, new_dist;
    int temp[num_remaining];

    //Create copy of remaining cities:
    copy_array(temp, remaining, num_remaining);

    //Set (backwards) current path:
    cur_path[num_remaining] = cur;

    //Base case: check if tour distance is best so far:
    if(num_remaining == 0) {
        distance += distances[cur][start];
        if (distance > best_distance) {
            set_best(distance);
        }
        return;
    }

    //Recursive case: 
    for(i=0; i<num_remaining; i++) {
        swap(0, i, temp);
        next = temp[0];
        new_dist = distance + distances[cur][next];
        brute_force(start, new_dist, next, temp+1, num_remaining-1);
    }
}
*/

void get_options(int argc, char ** argv) {
    char opt;

    while((opt = getopt(argc, argv, "acgi:vd")) != -1) {
        switch(opt) {
            case 'a':
                use_anneal = 1;
                break;
            case 'c':
                use_combo = 1;
                break;
            case 'g':
                use_greedy = 1;
                break;
            case 'i':
                in_file = 1;
                out_file = 1;
                strncpy(in_filename, optarg, WORD_MAX);
                snprintf(out_filename, WORD_MAX, "%s%s", in_filename, ".tour");
                break;
            case 'v':
                verbose = 1;
                break;
            case 'd':
                verbose = 1;
                debug = 1;
                break;
            default:
                printf("Usage: %s -[vd]\n", argv[0]);
                printf("\t-v: Verbose mode\n");
                printf("\t-d: Debug mode\n");
                exit(EXIT_SUCCESS);
        }
    }
}

void set_best(int distance, int * path) {
    best_distance = distance;
    copy_array(best_path, path, num_cities);
}

void swap(int i, int j, int * array) {
    int temp;

    temp = array[i];
    array[i] = array[j];
    array[j] = temp;
}

void copy_array(int * to, int * from, int len) {
    int i=0;

    for(i=0; i<len; i++) {
        to[i] = from[i];
    }
}

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

city * read_city(char * line) {
    city * c;

    c = malloc(sizeof(struct city));
    c->id = (int)strtol(line, &line, 10);
    c->x = (int)strtol(line, &line, 10);
    c->y = (int)strtol(line, &line, 10);

    return c;
}

void print_distances() {
    int i, j;
    for(i=0; i<num_cities; i++) {
        for(j=i+1; j<num_cities; j++) {
            print_distance(cities[i]->id, cities[j]->id);
        }
    }
}

void print_distance(int i, int j) {
    printf("Distance between %d and %d: %d\n", i, j, get_distance(i, j));
}

void print_cities(void) {
    int i;
    for(i=0; i<num_cities; i++) {
        print_city(cities[i]);
    }
}

void print_city(city * c) {
    printf("City: %d, X: %d, Y: %d\n", c->id, c->x, c->y);
}

void print_solution(void) {
    int i;
    FILE * f;

    if(out_file)
        f = fopen(out_filename, "w");
    else
        f = stdout;

    if(verbose)
        fprintf(f, "Best distance: %d\nPath:\n", best_distance);
    else
        fprintf(f, "%d\n", best_distance);

    for(i=0; i<num_cities; i++) {
        fprintf(f, "%d\n", best_path[i]);
    }

    if(out_file)
        fclose(f);
}

int calc_distances(int max_id) {
    int i, j, avg;
    unsigned long sum;

    sum=0;
    distances = malloc((max_id+1) * sizeof(int *));

    for(i=0; i<num_cities; i++) {
        distances[cities[i]->id] = malloc((max_id+1) * sizeof(int));

        for(j=i+1; j<num_cities; j++) {
            sum += distances[cities[i]->id][cities[j]->id] = calc_distance(cities[i], cities[j]);
        }
    }
    avg = (sum/pow(max_id, 2));
    return avg;
}

int calc_distance(city * a, city * b) {
    int x_dif, y_dif;
    double distance;

    x_dif = a->x - b->x;
    y_dif = a->y - b->y;

    distance = sqrt(pow((double) x_dif, 2.0) + pow((double) y_dif, 2.0));

    return (int) round(distance);
}

int get_distance(int i, int j) {
    if(i<j)
        return distances[i][j];
    else
        return distances[j][i];
}

int calc_path_dist(int * path, int len) {
    int i, dist;

    dist = 0;
    for(i=0; i<len-1; i++) {
        dist += get_distance(path[i], path[i+1]);
    }
    dist += get_distance(path[i], path[0]);
    return dist;
}

void free_distances(void) {
    int i;

    for(i=0; i<num_cities; i++) {
        free(distances[(cities[i]->id)]);
    }
    free(distances);
}

int greedy(int * path, int len) {
    int i, dst;

    dst = 0;
    for(i=0; i<len-1; i++) {
        dst += swap_closest(&path[i], (len-i));
    }

    dst += get_distance(path[len-1], path[0]);
    return dst;
}


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

int anneal_accept(int new_dst, int old_dst, double temp) {
    double prob, q;

    if(new_dst == old_dst)
        return 0;

    prob = exp(-(new_dst - old_dst)/temp);

    q = rand()/ (double) RAND_MAX;
    if(q < prob)
        return 1;
    else
        return 0;
}

double change_temp(double old_temp) {
    if(old_temp > DELTA_TEMP) {
        return old_temp - DELTA_TEMP;
    }
    return old_temp;
}

void two_opt_swap(int i, int j, int * path) {
    while (i < j) {
        swap(i++, j--, path);
    }
}

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



