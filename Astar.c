#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define PI 3.141592
//Earth radius (in km) for the haversine function:
#define R_t 6371


typedef struct {
    unsigned long id;
    //Length of node's name (this variable was not in the delivery's pdf but we will need it later on):         
    unsigned short namelen;     
    char* name;                 
    double lat, lon;            
    unsigned short nsucc;       
    unsigned long *successors;  
} node;


typedef char Queue;
enum whichQueue {NONE, OPEN, CLOSED};

typedef struct {
    double g, h;                
    unsigned long parent;       
    Queue whq;                  
} AStarStatus;


//Structure for a node in the OPEN list
typedef struct OL_node {
    double f;
    unsigned long index;
    struct OL_node* next;
} OL_node;


void ExitError(const char *miss, int errcode) {
    fprintf (stderr, "\nERROR: %s.\nStopping...\n\n", miss); exit(errcode);
}


unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes)
{
    // we know that the nodes where numrically ordered by id, so we can do a binary search.
    unsigned long l = 0, r = nnodes - 1, m;
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (nodes[m].id == id) return m;
        if (nodes[m].id < id)
            l = m + 1;
        else
            r = m - 1;
    }

    // id not found, we return nnodes+1
    return nnodes+1;
}


/*The following functions were built following the ones given in the delivery's pdf, the subject's notes 
and random info from the internet.*/

//Function that pops out the first element of the open list i.e. the one with least weight.
void pop (unsigned long target, AStarStatus* PathData, OL_node* OPEN) {
    OL_node* TEMP = OPEN;
    OL_node* PREV = NULL;
    while (TEMP != NULL && TEMP->index != target) {
        PREV = TEMP;
        TEMP = TEMP->next;
    }
    PREV->next = TEMP->next;
    free(TEMP);
}

//Function that pushes a node into the open list taking into account its weight!
void push (unsigned long index, AStarStatus* PathData, OL_node* OPEN, node* nodes, unsigned long src_index, unsigned long dest_index) {
    (PathData + index)->whq = 1;
    OL_node* TEMP = OPEN;                                                                 
    OL_node* new_node = NULL;                                                             
    if ((new_node = (OL_node*) malloc(sizeof(OL_node))) == NULL) ExitError("when allocating memory for a new node in the OPEN list", 1);
    new_node->index = index;                                                                
    new_node->f = PathData[index].g + PathData[index].h;
    new_node->next = NULL;                                                                  
    while ( (TEMP->next != NULL) && ((TEMP->next)->f <= new_node->f ) ) TEMP = TEMP->next;  
    if (TEMP->next == NULL) TEMP->next = new_node;                                          
    else if ((TEMP->next)->f > new_node->f) {                                               
        new_node->next = TEMP->next;
        TEMP->next = new_node;
    }
}

//We have chosen the havershine distance to be our heuristic function:
double haversine (node u, node v) {
    double diff_lat = (u.lat - v.lat) * PI / 180.f;
    double diff_lon = (u.lon - v.lon) * PI / 180.f;
    double a = pow(sin(diff_lat/2), 2) + cos(u.lat * PI / 180.f) * cos(v.lat * PI / 180.f) * pow(sin(diff_lon/2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    double d = R_t * c;
    return d;
}


//Function that allows us to write the final path to a txt file with some nodes' info:
void output_txt(node* nodes, unsigned long* path, unsigned long length, AStarStatus* info, char* name) {
    char ending[257] = "_SROutput";
    char buffer[11];
    strcat(ending, ".txt");
    strcpy(strrchr(name, '.'), ending);
    FILE *fout;
    if ((fout = fopen (name, "w+")) == NULL) ExitError("the output data file cannot be created", 2);

    fprintf(fout, "# Distance from %lu to %lu: %.6f meters.\n", nodes[path[0]].id, nodes[path[length-1]].id, info[path[length-1]].g*1000);
    fprintf(fout, "# Optimal path:\n");
    unsigned long i;
    for (i = 0; i < length; i++) {
        fprintf(fout, "Id = %lu | %.6f | %.6f | Dist = %.6f\n", nodes[path[i]].id, nodes[path[i]].lat, nodes[path[i]].lon, info[path[i]].g);
    }
    
    fclose(fout);
}


//A* algorithm as a function:
void AStar (node* nodes, unsigned long source, unsigned long dest, unsigned long nnodes, char* name) {
    
    unsigned long source_index = (unsigned long)searchNode(source, nodes, nnodes);           
    unsigned long dest_index   = (unsigned long)searchNode(dest, nodes, nnodes);            

    AStarStatus* PathData = NULL;
    if ((PathData = (AStarStatus*) malloc(nnodes*sizeof(AStarStatus))) == NULL) ExitError("when allocating memory for the PathData vector", 3);
    unsigned long i;
    for (i = 0; i < nnodes; i++) {
        PathData[i].whq = 0;                                                                  
        PathData[i].g = INFINITY;                                                             
    }
    PathData[source_index].whq = 1;                                                           
    PathData[source_index].g = 0;                                                             
    PathData[source_index].h = haversine( nodes[source_index], nodes[dest_index]);            

    struct OL_node* OPEN = NULL;                                                         
    if ((OPEN = (OL_node*) malloc(sizeof(OL_node))) == NULL) ExitError("when allocating memory for the OPEN list", 4);
    struct OL_node* AUX = NULL;
    OPEN->index = source_index;
    OPEN->f = PathData[source_index].g + PathData[source_index].h;
    OPEN->next = NULL;
    unsigned long cur_index;                    
    unsigned short succ_count;                  
    unsigned long succ_index;                   
    double successor_current_cost;              
    double w;                                   
    unsigned long expanded_nodes_counter = 0;   
    clock_t start, end;                         
    double cpu_time_used;
    start = clock();                            
    while (OPEN != NULL) {
        cur_index = OPEN->index;
        expanded_nodes_counter += 1;
        if (cur_index == dest_index) {                                              
            printf("DESTINATION REACHED! Check SROutput.txt file.\n");
            break;
        }
        for (succ_count = 0; succ_count < nodes[cur_index].nsucc; succ_count++) {   
            succ_index = (nodes[cur_index].successors)[succ_count];                 
            w = haversine( nodes[cur_index], nodes[succ_index] );                   
            successor_current_cost = PathData[cur_index].g + w;                    
            if ( PathData[succ_index].whq == 1 ) {
                if ( PathData[succ_index].g <= successor_current_cost ) continue;   
                else pop(succ_index, PathData, OPEN);                  
            }
            else if ( PathData[succ_index].whq == 2 ) continue;
            else PathData[succ_index].h = haversine( nodes[succ_index], nodes[dest_index] ); 
            
            PathData[succ_index].g = successor_current_cost;                       
            PathData[succ_index].parent = cur_index;                                
            push(succ_index, PathData, OPEN, nodes, source_index, dest_index);
        }
        PathData[cur_index].whq = 2;                                               
        AUX = OPEN->next;   free(OPEN);     OPEN = AUX;                                                                                                                      
    }
    end = clock();                                                                      
    if (OPEN == NULL) ExitError("OPEN list is empty before reaching destination", 5);  

    while (OPEN != NULL) {                                                              
        AUX = OPEN->next;   free(OPEN);     OPEN = AUX;
    }
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;                          
    printf("Optimal distance: %.6f meters.\n", PathData[dest_index].g*1000);
    printf("A* time elapsed: %.6f seconds.\n", cpu_time_used);

    cur_index = dest_index;
    unsigned long path_len = 0;
    while (cur_index != source_index) {
        path_len += 1;
        cur_index = PathData[cur_index].parent;
    }
    path_len += 1;

    unsigned long* path = NULL;
    if ((path = (unsigned long*) malloc(path_len*sizeof(unsigned long))) == NULL) ExitError("when allocating memory for the path vector", 6);
    path += path_len - 1;
    cur_index = dest_index;
    while (cur_index != source_index) { 
        *path = cur_index;
        cur_index = PathData[cur_index].parent;
        path -= 1;
    }
    if (cur_index == source_index) *path = cur_index;
    
    output_txt(nodes, path, path_len, PathData, name);
    
    free(path);
    free(PathData);
}



int main (int argc, char *argv[]) {
    
    FILE *fin;
    if ((fin = fopen (argv[1], "rb")) == NULL)
        ExitError("Please pass a binary file as an argument", 7);
       
    unsigned long nnodes;
    unsigned long nsucc;
    unsigned long totnamelen;
    if( fread(&nnodes, sizeof(unsigned long), 1, fin) +
        fread(&nsucc, sizeof(unsigned long), 1, fin)
         + fread(&totnamelen, sizeof(unsigned long), 1, fin) != 3 )
            ExitError("when reading the header of the binary data file", 8);
    
    node* nodes;
    if ((nodes = (node*) malloc(nnodes*sizeof(node))) == NULL)
        ExitError("when allocating memory for the nodes vector", 9);
    
    unsigned long* allsuccessors;
    if ((allsuccessors = (unsigned long*) malloc(nsucc*sizeof(unsigned long))) == NULL)
        ExitError("when allocating memory for the edges vector", 10);
    
    char* allnames = NULL;
    if((allnames = (char*) malloc((totnamelen)*sizeof(char))) == NULL) ExitError("when allocating memory for allnames vector", 11);
    char* allnames_aux = allnames; 
    
    if( fread(nodes, sizeof(node), nnodes, fin) != nnodes )
        ExitError("when reading nodes from the binary data file", 12);
    
    if( fread(allsuccessors, sizeof(unsigned long), nsucc, fin) != nsucc)
        ExitError("when reading sucessors from the binary data file", 13);

    if( fread(allnames, sizeof(char), totnamelen, fin) != totnamelen)
        ExitError("when reading names from the binary data file", 14);
    
    fclose(fin);
    
//We store all names and all successors of all nodes in two respective vectors as recommended in the pdf:
    unsigned long i;
    for (i = 0; i < nnodes; i++) {
        if(nodes[i].nsucc) {
            nodes[i].successors = allsuccessors;
            allsuccessors += nodes[i].nsucc;
        }
        if ((nodes[i].name = (char*) malloc(nodes[i].namelen + 1)) == NULL) ExitError("when allocating memory for a node name", 15);
        strncpy(nodes[i].name, allnames, nodes[i].namelen);
        memset(nodes[i].name + nodes[i].namelen, '\0', 1);
        allnames += nodes[i].namelen;
        
    }
    free(allnames_aux);
    
    unsigned long source = 240949599;             //SOURCE NODE'S ID
    unsigned long dest = 195977239;               //DESTINATION NODE'S ID
    
    
    AStar(nodes, source, dest, nnodes, argv[1]);
    

    for (i = 0; i < nnodes; i++) free(nodes[i].name);
    free(nodes);
    allsuccessors -= nsucc;     
    free(allsuccessors);
            
    return 0;
}